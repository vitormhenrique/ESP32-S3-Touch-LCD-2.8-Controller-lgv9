#include "CRSF_Manager.h"
#include "InputManager.h"
#include "ui/screens/ui_screen_telemetry.h"
#include <cstring>
#include <cmath>
#include <driver/gpio.h>   // For gpio_set_pull_mode
#include <driver/uart.h>   // For RS485 half-duplex mode
#include <esp_rom_gpio.h>  // For inverted GPIO matrix routing
#include <soc/gpio_sig_map.h>  // For U1RTS_OUT_IDX

// Global instance
CRSF_Manager CRSFLink;

//=============================================================================
// Constructor
//=============================================================================

CRSF_Manager::CRSF_Manager()
    : _initialized(false)
    , _serial(Serial1)
    , _crc(0xD5)
    , _frameIntervalUs(1000000 / CRSF_DEFAULT_RATE_HZ)
    , _lastFrameUs(0)
    , _frameRateHz(CRSF_DEFAULT_RATE_HZ)
    , _telemetry{}
    , _telemetryMutex(nullptr)
    , _bootstrapIntervalUs(1000000 / CRSF_BOOTSTRAP_RATE_HZ)
    , _lastBootstrapUs(0)
    , _lastDiagMs(0)
    , _lastModulePacketMs(0)
    , _lastPingMs(0)
    , _linkOk(false)
    , _prevLinkUp(false)
    , _prevAttitudeValid(false)
    , _lastLogMs(0)
    , _lastStatsLogMs(0)
    , _logQueue(nullptr)
{
}

//=============================================================================
// Initialization
//=============================================================================

bool CRSF_Manager::begin() {
    printf("[CRSF] Initializing...\n");
    printf("[CRSF] TX Pin: %d, RX Pin: %d, OE Pin: %d\n",
           CRSF_UART_TX_PIN, CRSF_UART_RX_PIN, CRSF_OE_PIN);
    printf("[CRSF] Baud: %d, Frame rate: %d Hz\n", CRSF_BAUD, CRSF_DEFAULT_RATE_HZ);

    // Clear any stale pad hold from previous experimental firmware/recovery.
    gpio_hold_dis((gpio_num_t)CRSF_OE_PIN);

    // Configure OE pin for active-high buffer control.
    // LOW = hi-Z (RX mode), HIGH = output active (TX mode)
    pinMode(CRSF_OE_PIN, OUTPUT);
    digitalWrite(CRSF_OE_PIN, LOW);  // Default: RX mode
    // Enable internal pull-down to ensure buffer stays tri-stated during boot/glitches
    gpio_set_pull_mode((gpio_num_t)CRSF_OE_PIN, GPIO_PULLDOWN_ONLY);
    printf("[CRSF] OE pin configured: LOW (RX mode), pull-down enabled\n");

    // Initialize UART
    _serial.begin(CRSF_BAUD, SERIAL_8N1, CRSF_UART_RX_PIN, CRSF_UART_TX_PIN);
    printf("[CRSF] Serial1 initialized\n");

#if CRSF_HW_HALF_DUPLEX
    // Hand OE control to the UART peripheral (RS485 half-duplex mode).
    // The driver asserts RTS (pin HIGH) for the exact duration of each
    // transmission and releases it from the TX-done interrupt - immune to
    // task preemption, unlike software GPIO toggling.
    // OE is active-high, so the RTS signal is routed to the pin directly:
    //   transmitting -> OE HIGH (buffer drives bus)
    //   idle         -> OE LOW (buffer tri-state, RX listens)
    uart_set_pin(UART_NUM_1, CRSF_UART_TX_PIN, CRSF_UART_RX_PIN,
                 CRSF_OE_PIN, UART_PIN_NO_CHANGE);
    uart_set_mode(UART_NUM_1, UART_MODE_RS485_HALF_DUPLEX);
    esp_rom_gpio_connect_out_signal(CRSF_OE_PIN, U1RTS_OUT_IDX, false, false);
    printf("[CRSF] Hardware half-duplex enabled (RS485 mode, direct RTS on OE)\n");
#endif

    // Initialize CRSF library
    _crsf.begin(_serial);

    // Create telemetry mutex
    _telemetryMutex = xSemaphoreCreateMutex();
    if (!_telemetryMutex) {
        printf("[CRSF] ERROR: Failed to create mutex\n");
        return false;
    }

    // Log queue: CRSF task -> LVGL thread (see queueLog)
    _logQueue = xQueueCreate(CRSF_LOG_QUEUE_LEN, CRSF_LOG_MSG_MAX);

    _initialized = true;
    printf("[CRSF] Initialized at %d baud, %d Hz frame rate\n",
           CRSF_BAUD, _frameRateHz);
    printf("[CRSF] Half-duplex mode with active-high tri-state buffer\n");

    return true;
}

//=============================================================================
// FreeRTOS Task
//=============================================================================

void CRSF_Manager::taskFunc(void* param) {
    CRSF_Manager* self = static_cast<CRSF_Manager*>(param);

    printf("[CRSF] Task started on core %d\n", xPortGetCoreID());

#if CRSF_RETRY_ENABLED
    // Cold-boot recovery without resetting the MCU: if the module never
    // responds, periodically go silent so the module parser can time out and
    // resync, then start a clean CRSF stream again.
    uint32_t retryDeadlineMs = millis() + CRSF_RETRY_PERIOD_MS;
#endif

    while (true) {
        self->update();

#if CRSF_RETRY_ENABLED
        if (!self->_linkOk && self->_lastModulePacketMs == 0 &&
            millis() > retryDeadlineMs) {
            printf("[CRSF] No module response for %d ms - silent retry cycle\n",
                   CRSF_RETRY_PERIOD_MS);
            self->queueLog("CRSF: retrying link...");

            // Go completely quiet so the module's parser can time out and resync
            vTaskDelay(pdMS_TO_TICKS(CRSF_RETRY_SILENCE_MS));

            // Drop any bytes collected during the silence and start clean
            while (self->_serial.available()) self->_serial.read();
            retryDeadlineMs = millis() + CRSF_RETRY_PERIOD_MS;
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(4)); // ~250Hz polling for responsive serial RX; frame rate is gated internally
    }
}

//=============================================================================
// Main update (called from CRSF task at 50Hz)
//=============================================================================

void CRSF_Manager::update() {
    if (!_initialized) return;

    uint32_t nowUs = micros();
    uint32_t nowMs = millis();

    // Always process incoming first. _linkOk is computed inside
    // processTelemetry() with a post-parse timestamp (see comment there).
    processTelemetry();

    // Keepalive poll: the ELRS module doesn't stream telemetry to the handset
    // unsolicited - the stock Lua script polls it every 1s. Keep polling while
    // linked too, or link detection flaps at the CRSF_LINK_TIMEOUT_MS rhythm.
    // Discovery ping (0x28) until the module is first seen, then the same
    // ELRS status request (0x2D -> 0x2E reply) the Lua script sends.
    if ((nowMs - _lastPingMs) >= CRSF_PING_INTERVAL_MS) {
        _lastPingMs = nowMs;
        if (_lastModulePacketMs == 0) {
            sendDevicePing();
        } else {
            sendLinkStatRequest();
        }
    }

    if (_linkOk && RCInput.isReady()) {
        // Normal operation: send real RC data at full rate
        if ((nowUs - _lastFrameUs) >= _frameIntervalUs) {
            _lastFrameUs = nowUs;

            uint16_t channels[CPACK_NUM_CHANNELS];
            gatherAndPackChannels(channels);
            sendRcChannelsPacked(channels);

            if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                _telemetry.frames_sent++;
                xSemaphoreGive(_telemetryMutex);
            }
        }
    } else {
        // Bootstrap: send center-value RC frames at low rate to wake up TX module.
        // ELRS TX modules will NOT transmit RF or bind unless they receive valid
        // CRSF RC frames from the handset. Without this, we deadlock:
        //   no RC frames → TX module silent → no binding → no link → no RC frames
        if ((nowUs - _lastBootstrapUs) >= _bootstrapIntervalUs) {
            _lastBootstrapUs = nowUs;
            sendBootstrapFrame();

            if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                _telemetry.frames_sent++;
                xSemaphoreGive(_telemetryMutex);
            }
        }

        // Diagnostic logging while waiting for link
        if ((nowMs - _lastDiagMs) >= CRSF_LINK_DIAG_MS) {
            _lastDiagMs = nowMs;
            
            // Get frames sent for diagnostic
            uint32_t framesSent = 0;
            if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                framesSent = _telemetry.frames_sent;
                xSemaphoreGive(_telemetryMutex);
            }
            
            if (_crsf.bytesRead() == 0) {
                printf("[CRSF] No UART bytes from TX module. Sent %lu bootstrap frames. Check wiring/power.\n",
                       (unsigned long)framesSent);
            } else if (_crsf.goodPackets() == 0) {
                printf("[CRSF] UART activity but no valid CRSF packets (bad=%lu, sent=%lu). Check baud/wiring.\n",
                       (unsigned long)_crsf.badPackets(), (unsigned long)framesSent);
            } else {
                printf("[CRSF] Bootstrap: waiting for link (good=%lu, bad=%lu, sent=%lu, inputs_ready=%d)\n",
                       (unsigned long)_crsf.goodPackets(),
                       (unsigned long)_crsf.badPackets(),
                       (unsigned long)framesSent,
                       RCInput.isReady());
            }
        }
    }

    // Generate log messages for state changes
    generateLogs();
}

//=============================================================================
// Channel packing
//=============================================================================

void CRSF_Manager::gatherAndPackChannels(uint16_t channels[CPACK_NUM_CHANNELS]) {
    ChannelPackInputs_t inputs;
    memset(&inputs, 0, sizeof(inputs));

    // Gimbals
    inputs.gimbal[0] = RCInput.getLeftX();
    inputs.gimbal[1] = RCInput.getLeftY();
    inputs.gimbal[2] = RCInput.getRightX();
    inputs.gimbal[3] = RCInput.getRightY();

    // Potentiometers
    inputs.pot[0] = RCInput.getPot(POT_1);
    inputs.pot[1] = RCInput.getPot(POT_2);

    // Encoders
    inputs.encoder[0] = RCInput.getEncoderPosition(ENCODER_1);
    inputs.encoder[1] = RCInput.getEncoderPosition(ENCODER_2);

    // 6 two-position switches (indices 0-5: SW_A,B,C,D,G,H)
    for (int i = 0; i < 6; i++) {
        inputs.switches[i] = RCInput.isSwitchOn(i);
    }
    // Bits 6-7 reserved (SW_E/SW_F now handled as 3-pos toggles)
    inputs.switches[6] = false;
    inputs.switches[7] = false;

    // 4 buttons (BTN_1=idx 6, BTN_2=idx 7, BTN_3=idx 8, BTN_4=idx 9)
    inputs.buttons[0] = RCInput.isSwitchOn(6);   // BTN_1
    inputs.buttons[1] = RCInput.isSwitchOn(7);   // BTN_2
    inputs.buttons[2] = RCInput.isSwitchOn(8);   // BTN_3
    inputs.buttons[3] = RCInput.isSwitchOn(9);   // BTN_4

    // 3-position toggles (SW_E, SW_F)
    inputs.toggles[0] = (uint8_t)RCInput.getToggle3Pos(TOGGLE_3POS_1);
    inputs.toggles[1] = (uint8_t)RCInput.getToggle3Pos(TOGGLE_3POS_2);

    // Nav switches (NAV1: indices 10-14, NAV2: indices 15-19)
    for (int i = 0; i < 5; i++) {
        inputs.nav[0][i] = RCInput.isSwitchOn(10 + i);  // NAV1 U,D,L,R,C
        inputs.nav[1][i] = RCInput.isSwitchOn(15 + i);  // NAV2 U,D,L,R,C
    }

    ChannelPack::packInputs(&inputs, channels);
}

//=============================================================================
// Debug helpers
//=============================================================================

static void printHexFrame(const char* label, const uint8_t* data, size_t len) {
    printf("%s HEX [%d]: ", label, (int)len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

//=============================================================================
// CRSF frame building & sending (half-duplex)
//=============================================================================

// Debug: Print first frame and then periodically
static uint32_t _lastFrameDebugMs = 0;
static uint32_t _frameDebugCount = 0;
#define FRAME_DEBUG_INTERVAL_MS 5000  // Print frame every 5 seconds

void CRSF_Manager::sendRcChannelsPacked(const uint16_t channels[CPACK_NUM_CHANNELS]) {
    // Bit-pack 16 x 11-bit channels into 22-byte payload (LSB-first)
    uint8_t payload[22] = {0};
    uint32_t bitBuffer = 0;
    uint8_t bitCount = 0;
    uint8_t outIndex = 0;

    for (uint8_t i = 0; i < 16; i++) {
        uint16_t v = channels[i] & 0x07FF;
        bitBuffer |= ((uint32_t)v) << bitCount;
        bitCount += 11;

        while (bitCount >= 8) {
            if (outIndex >= sizeof(payload)) break;
            payload[outIndex++] = (uint8_t)(bitBuffer & 0xFF);
            bitBuffer >>= 8;
            bitCount -= 8;
        }
    }

    // Flush remaining bits
    while (outIndex < sizeof(payload)) {
        payload[outIndex++] = (uint8_t)(bitBuffer & 0xFF);
        bitBuffer >>= 8;
    }

    // Build CRSF frame: [addr][len][type][payload][crc]
    uint8_t frame[CRSF_MAX_PACKET_LEN + 4];
    frame[0] = CRSF_ADDRESS_CRSF_TRANSMITTER; // 0xEE
    frame[1] = 22 + 2;                        // payload + type + crc
    frame[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED; // 0x16
    memcpy(&frame[3], payload, 22);
    frame[25] = _crc.calc(&frame[2], 23);      // CRC over type + payload

    size_t frameLen = 26;

    // Debug: Print frame periodically
    uint32_t nowMs = millis();
    bool printDebug = (_frameDebugCount == 0) || 
                      (nowMs - _lastFrameDebugMs >= FRAME_DEBUG_INTERVAL_MS);
    
    if (printDebug) {
        _lastFrameDebugMs = nowMs;
        printf("[CRSF] --- Frame #%lu ---\n", (unsigned long)_frameDebugCount);
        printf("[CRSF] Channels: ");
        for (int i = 0; i < 8; i++) {
            printf("%d ", channels[i]);
        }
        printf("...\n");
        printHexFrame("[CRSF]", frame, frameLen);
    }

    // Half-duplex with active-high hardware buffer:
    // When using a tri-state buffer, we DON'T get echo - the buffer isolates TX from RX.
    // So no need to discard echo like in one-wire mode.
#if CRSF_HW_HALF_DUPLEX
    // UART hardware controls OE (RS485 mode) - just write.
    size_t written = _serial.write(frame, frameLen);
    _serial.flush();           // Wait for TX to finish (keeps frame pacing honest)
#else
    setOeMode(true);           // OE HIGH = buffer enabled, TX drives line
    delayMicroseconds(2);      // Let OE settle (same as working example)

    size_t written = _serial.write(frame, frameLen);
    _serial.flush();           // Wait for TX to finish

    setOeMode(false);          // OE LOW = buffer tri-state, RX can receive
#endif
    
    _frameDebugCount++;
    
    // Debug: Log write result
    if (printDebug) {
        printf("[CRSF] write() returned %d/%d bytes\n", (int)written, (int)frameLen);
        printf("[CRSF] UART RX bytes: %lu, good: %lu, bad: %lu\n",
               (unsigned long)_crsf.bytesRead(),
               (unsigned long)_crsf.goodPackets(),
               (unsigned long)_crsf.badPackets());
    }
    
    // Warning if write failed
    if (written != frameLen) {
        printf("[CRSF] WARNING: write() returned %d, expected %d\n", (int)written, (int)frameLen);
    }
}

void CRSF_Manager::sendBootstrapFrame() {
    // Send center-value channels to bootstrap the TX module into transmitting.
    // This is required for ELRS modules that stay silent until they see RC frames.
    uint16_t channels[CPACK_NUM_CHANNELS];
    for (int i = 0; i < CPACK_NUM_CHANNELS; i++) {
        channels[i] = CPACK_CRSF_MID;  // 992 = 1500us center
    }
    sendRcChannelsPacked(channels);
}

void CRSF_Manager::sendDevicePing() {
    // Broadcast device ping to provoke a device info (0x29) response from TX module.
    // This helps detect when a freshly-powered radio module comes online.
    // Frame: [addr=0x00][len=4][type=0x28][dest=0x00][origin=0xEA][crc]
    uint8_t frame[6];
    frame[0] = CRSF_ADDRESS_BROADCAST;
    frame[1] = 4;
    frame[2] = CRSF_FRAMETYPE_DEVICE_PING;
    frame[3] = CRSF_ADDRESS_BROADCAST;
    frame[4] = CRSF_ADDRESS_RADIO_TRANSMITTER;
    frame[5] = _crc.calc(&frame[2], 3);

#if CRSF_HW_HALF_DUPLEX
    _serial.write(frame, 6);
    _serial.flush();
#else
    setOeMode(true);
    delayMicroseconds(2);
    _serial.write(frame, 6);
    _serial.flush();
    setOeMode(false);
#endif
}

void CRSF_Manager::sendLinkStatRequest() {
    // ELRS status request - exactly what the stock ELRS Lua sends every 1s:
    //   crossfireTelemetryPush(0x2D, { deviceId, handsetId, 0x0, 0x0 })
    // Frame: [addr=0xEE][len=6][type=0x2D][dest=0xEE][origin=0xEF][0x00][0x00][crc]
    // The module replies with an ELRS_STATUS frame (0x2E: bad/good + flags),
    // which keeps the echo-aware link detection fed while connected.
    uint8_t frame[8];
    frame[0] = CRSF_ADDRESS_CRSF_TRANSMITTER;
    frame[1] = 6;
    frame[2] = CRSF_FRAMETYPE_ELRS_STATUS_REQ;
    frame[3] = CRSF_ADDRESS_CRSF_TRANSMITTER;
    frame[4] = CRSF_ADDRESS_ELRS_LUA;
    frame[5] = 0x00;
    frame[6] = 0x00;
    frame[7] = _crc.calc(&frame[2], 5);

#if CRSF_HW_HALF_DUPLEX
    _serial.write(frame, sizeof(frame));
    _serial.flush();
#else
    setOeMode(true);
    delayMicroseconds(2);
    _serial.write(frame, sizeof(frame));
    _serial.flush();
    setOeMode(false);
#endif
}

void CRSF_Manager::setOeMode(bool txMode) {
    // OE is active-high.
    // HIGH = buffer output active (TX drives bus)
    // LOW = buffer output hi-Z (RX listens)
    digitalWrite(CRSF_OE_PIN, txMode ? HIGH : LOW);
}

void CRSF_Manager::discardEcho(size_t bytesSent) {
    // At 420000 baud, each byte ~24us (10 bits). Add margin.
    uint32_t byteTimeUs = 10000000UL / CRSF_BAUD;
    uint32_t timeoutUs = bytesSent * byteTimeUs + 500;
    uint32_t startUs = micros();
    size_t drained = 0;

    while (drained < bytesSent && (uint32_t)(micros() - startUs) < timeoutUs) {
        if (_serial.available()) {
            (void)_serial.read();
            drained++;
        }
    }
}

//=============================================================================
// Telemetry processing
//=============================================================================

void CRSF_Manager::processTelemetry() {
    _crsf.update();

    // Echo-aware link detection: the library tracks the last packet that could
    // not be a local echo (per packet, inside the parser). Mirror it here for
    // the retry logic in taskFunc.
    _lastModulePacketMs = _crsf.lastModulePacketTimeMs();

    // Compute link state with a timestamp captured AFTER parsing. The parser
    // stamps packets with millis() DURING _crsf.update(); comparing against a
    // timestamp captured before parsing let a fresh packet sit "in the
    // future", so the unsigned subtraction wrapped to ~4e9 and dropped the
    // link for one cycle (UP/DOWN log bombardment at the task rate).
    uint32_t nowMs = millis();
    _linkOk = (_lastModulePacketMs != 0) &&
              ((nowMs - _lastModulePacketMs) < CRSF_LINK_TIMEOUT_MS);

    if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) != pdTRUE) return;

    // Link state (echo-aware: _linkOk filters out self-echoed frames)
    _telemetry.link_up = _linkOk;
    if (_linkOk && !_prevLinkUp) {
        _telemetry.link_up_since_ms = millis();
    }

    // CRC errors
    _telemetry.crc_errors = _crsf.badPackets();
    _telemetry.frames_received = _crsf.goodPackets();

    // Link statistics
    const crsfLinkStatistics_t* stats = _crsf.getLinkStatistics();
    if (stats) {
        _telemetry.rssi_1   = -(int8_t)stats->uplink_RSSI_1;
        _telemetry.rssi_2   = -(int8_t)stats->uplink_RSSI_2;
        _telemetry.lq       = stats->uplink_Link_quality;
        _telemetry.snr      = stats->uplink_SNR;
        _telemetry.rf_mode  = stats->rf_Mode;
        _telemetry.tx_power = stats->uplink_TX_Power;
    }

    // Attitude (BNO055 Euler angles from receiver)
    const crsf_sensor_attitude_t* att = _crsf.getAttitudeSensor();
    if (att && _crsf.lastValidPacketType() == CRSF_FRAMETYPE_ATTITUDE) {
        // Attitude values are radians * 10000, big-endian int16
        int16_t pitch_raw = (int16_t)be16toh(att->pitch);
        int16_t roll_raw  = (int16_t)be16toh(att->roll);
        int16_t yaw_raw   = (int16_t)be16toh(att->yaw);

        _telemetry.pitch_deg = (float)pitch_raw / 10000.0f * (180.0f / (float)M_PI);
        _telemetry.roll_deg  = (float)roll_raw  / 10000.0f * (180.0f / (float)M_PI);
        _telemetry.yaw_deg   = (float)yaw_raw   / 10000.0f * (180.0f / (float)M_PI);
        _telemetry.attitude_valid = true;
        _telemetry.last_attitude_ms = millis();
    }

    xSemaphoreGive(_telemetryMutex);
}

//=============================================================================
// Log generation (state change detection)
//=============================================================================

// Queue a UI log message from any task. Messages are drained and pushed into
// LVGL by updateUI(), which runs on the LVGL thread. NEVER call
// ui_telemetry_add_log() from the CRSF task: LVGL is not thread-safe and the
// race with the core-1 renderer corrupts memory (observed StoreProhibited).
void CRSF_Manager::queueLog(const char* msg) {
    if (!_logQueue) return;
    char buf[CRSF_LOG_MSG_MAX];
    strncpy(buf, msg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    xQueueSend(_logQueue, buf, 0);  // Drop message if queue full
}

void CRSF_Manager::generateLogs() {
    CRSFTelemetry_t t;
    if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        t = _telemetry;
        xSemaphoreGive(_telemetryMutex);
    } else {
        return;
    }

    // Link up/down transitions
    if (t.link_up && !_prevLinkUp) {
        queueLog("CRSF: Link UP");
        printf("[CRSF] Link UP (connected to TX module)\n");
    } else if (!t.link_up && _prevLinkUp) {
        queueLog("CRSF: Link DOWN - reconnecting");
        printf("[CRSF] Link DOWN (last module packet %lu ms ago, last rx type 0x%02X) - bootstrapping...\n",
               (unsigned long)(millis() - _lastModulePacketMs),
               _crsf.lastValidPacketType());
    }
    _prevLinkUp = t.link_up;

    // First attitude received
    if (t.attitude_valid && !_prevAttitudeValid) {
        queueLog("CRSF: IMU data received");
        printf("[CRSF] First attitude data received\n");
    }
    _prevAttitudeValid = t.attitude_valid;

    // Periodic stats log
    if (millis() - _lastStatsLogMs >= CRSF_LOG_INTERVAL_MS) {
        _lastStatsLogMs = millis();
        if (t.link_up) {
            char buf[64];
            snprintf(buf, sizeof(buf), "CRSF: RSSI %d LQ %d%% TX %lu RX %lu",
                     t.rssi_1, t.lq,
                     (unsigned long)t.frames_sent,
                     (unsigned long)t.frames_received);
            queueLog(buf);
        }
    }
}

//=============================================================================
// UI update (must be called from main loop / LVGL thread)
//=============================================================================

void CRSF_Manager::updateUI() {
    if (!_initialized) return;

    // Drain queued log messages (produced by the CRSF task) into LVGL here,
    // on the LVGL thread, where it is safe.
    if (_logQueue) {
        char msg[CRSF_LOG_MSG_MAX];
        while (xQueueReceive(_logQueue, msg, 0) == pdTRUE) {
            ui_telemetry_add_log(msg);
        }
    }

    CRSFTelemetry_t t = getTelemetry();

    // Status panel
    uint32_t uptime = 0;
    if (t.link_up && t.link_up_since_ms > 0) {
        uptime = (millis() - t.link_up_since_ms) / 1000;
    }
    int rssi = t.rssi_1 < t.rssi_2 ? t.rssi_1 : t.rssi_2; // best (least negative)
    // RSSI values are negative; pick the one closer to 0
    rssi = (t.rssi_1 > t.rssi_2) ? t.rssi_1 : t.rssi_2;

    ui_telemetry_update_status(rssi, (int)t.lq, (int)t.crc_errors, uptime);

    // IMU panel (BNO055 Euler angles from receiver)
    if (t.attitude_valid) {
        ui_telemetry_update_imu9(
            t.pitch_deg, t.roll_deg, t.yaw_deg,             // Row 1: Euler angles
            (float)t.bno_cal_sys, (float)t.bno_cal_gyro,
            (float)t.bno_cal_accel,                          // Row 2: BNO055 cal
            t.voltage, (float)t.lq, (float)t.snr             // Row 3: Link info
        );
    }
}

//=============================================================================
// Public accessors
//=============================================================================

CRSFTelemetry_t CRSF_Manager::getTelemetry() {
    CRSFTelemetry_t copy = {};
    if (_telemetryMutex && xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        copy = _telemetry;
        xSemaphoreGive(_telemetryMutex);
    }
    return copy;
}

void CRSF_Manager::setFrameRate(uint16_t hz) {
    if (hz < 10) hz = 10;
    if (hz > 500) hz = 500;
    _frameRateHz = (uint8_t)hz;
    _frameIntervalUs = 1000000 / (uint32_t)hz;
    printf("[CRSF] Frame rate set to %d Hz\n", hz);
}

//=============================================================================
// C-linkage convenience
//=============================================================================

void CRSF_Init(void) {
    if (!CRSFLink.begin()) {
        printf("[CRSF] ERROR: Initialization failed!\n");
        return;
    }

    xTaskCreatePinnedToCore(
        CRSF_Manager::taskFunc,
        "CRSFTask",
        CRSF_TASK_STACK_SIZE,
        &CRSFLink,
        CRSF_TASK_PRIORITY,
        NULL,
        CRSF_TASK_CORE
    );

    // NOTE: not calling ui_telemetry_add_log here - CRSF_Init runs in
    // DriverTask (core 0) and LVGL calls are only safe on the LVGL thread.
    printf("[CRSF] Task created on core %d, priority %d\n",
           CRSF_TASK_CORE, CRSF_TASK_PRIORITY);
}
