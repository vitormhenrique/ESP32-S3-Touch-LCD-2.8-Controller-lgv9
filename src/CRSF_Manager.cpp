#include "CRSF_Manager.h"
#include "InputManager.h"
#include "InputSim.h"
#include "ui/screens/ui_screen_telemetry.h"
#include "elrs/elrs_client.h"
#include "elrs/elrs_service.h"
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
    , _lastHexapodErrorSequence(0)
    , _lastHexapodErrorCode(0)
    , _lastHexapodErrorDetail(0)
    , _lastHexapodErrorSeverity(0)
    , _lastHexapodErrorCount(0)
    , _lastLogMs(0)
    , _lastStatsLogMs(0)
    , _logQueue(nullptr)
    , _elrsTxQueue(nullptr)
    , _elrsRxQueue(nullptr)
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

    // ELRS config bridge queues + raw frame capture (device info 0x29,
    // parameter entries 0x2B are not decoded by AlfredoCRSF itself).
    _elrsTxQueue = xQueueCreate(CRSF_ELRS_QUEUE_LEN, sizeof(CRSFElrsFrame_t));
    _elrsRxQueue = xQueueCreate(CRSF_ELRS_QUEUE_LEN, sizeof(CRSFElrsFrame_t));
    _crsf.setRawFrameCallback(rawFrameCb, this);

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

    // ELRS config frames queued by the UI (parameter reads/writes). One per
    // cycle (4ms loop) so they interleave between RC frames without jitter.
    sendElrsQueued();

#if UI_INPUT_SIM
    // Simulated input mode: hardware inputs are not required to transmit
    const bool inputsReady = true;
#else
    const bool inputsReady = RCInput.isReady();
#endif
    if (_linkOk && inputsReady) {
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

#if UI_INPUT_SIM
    //=========================================================================
    // Simulated input mode: values come from the touch UI (InputSim).
    // Channels not exposed in the UI keep default/neutral values:
    // pots = 0, encoders = 0, nav = all false, switches[6..7] = false.
    //=========================================================================
    const input_sim_state_t *sim = input_sim_get();

    inputs.gimbal[0] = sim->gimbal[0];
    inputs.gimbal[1] = sim->gimbal[1];
    inputs.gimbal[2] = sim->gimbal[2];
    inputs.gimbal[3] = sim->gimbal[3];

    for (int i = 0; i < 6; i++) {
        inputs.switches[i] = sim->sw[i];
    }

    for (int i = 0; i < 4; i++) {
        inputs.buttons[i] = sim->btn[i];
    }

    inputs.toggles[0] = sim->toggle3[0];
    inputs.toggles[1] = sim->toggle3[1];
#else
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
#endif // UI_INPUT_SIM

    ChannelPack::packInputs(&inputs, channels);
}

//=============================================================================
// CRSF frame building & sending (half-duplex)
//=============================================================================

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
    // The outer address targets the local TX module; only the extended-frame
    // destination is broadcast to discover every device behind that module.
    // Frame: [addr=0xEE][len=4][type=0x28][dest=0x00][origin=0xEA][crc]
    uint8_t frame[6];
    frame[0] = CRSF_ADDRESS_CRSF_TRANSMITTER;
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

//=============================================================================
// ELRS config protocol bridge
//=============================================================================

// Called on the LVGL thread (elrs_client send callback). Thread-safe: only
// queues the frame; the CRSF task owns the UART.
bool CRSF_Manager::elrsEnqueueTx(uint8_t frame_type, const uint8_t* payload, uint8_t len) {
    if (!_elrsTxQueue || len > CRSF_ELRS_FRAME_MAX) return false;
    CRSFElrsFrame_t f;
    f.type = frame_type;
    f.len = len;
    memcpy(f.payload, payload, len);
    return xQueueSend(_elrsTxQueue, &f, 0) == pdTRUE;
}

// CRSF task: transmit at most one queued config frame per update() cycle.
void CRSF_Manager::sendElrsQueued() {
    if (!_elrsTxQueue) return;
    CRSFElrsFrame_t f;
    if (xQueueReceive(_elrsTxQueue, &f, 0) != pdTRUE) return;

    // Wire format: [bus addr][len][type][payload...][crc]
    // Handset-originated config traffic always enters through the local TX
    // module. Extended payload [dest][origin] routes it to TX/RX/broadcast.
    uint8_t frame[CRSF_ELRS_FRAME_MAX + 4];
    frame[0] = CRSF_ADDRESS_CRSF_TRANSMITTER;
    frame[1] = f.len + 2;  // type + payload + crc
    frame[2] = f.type;
    memcpy(&frame[3], f.payload, f.len);
    frame[3 + f.len] = _crc.calc(&frame[2], f.len + 1);
    size_t total = (size_t)f.len + 4;

#if CRSF_HW_HALF_DUPLEX
    _serial.write(frame, total);
    _serial.flush();
#else
    setOeMode(true);
    delayMicroseconds(2);
    _serial.write(frame, total);
    _serial.flush();
    setOeMode(false);
#endif
}

// CRSF task context (from AlfredoCRSF::processPacketIn). Forward config
// protocol replies to the LVGL thread; everything else is decoded in-library.
void CRSF_Manager::rawFrameCb(uint8_t type, const uint8_t* payload,
                              uint8_t len, void* user) {
    CRSF_Manager* self = static_cast<CRSF_Manager*>(user);

    if (type == CRSF_FT_DEVICE_INFO || type == CRSF_FT_PARAM_SETTINGS_ENTRY) {
        if (!self->_elrsRxQueue || len > CRSF_ELRS_FRAME_MAX) return;
        CRSFElrsFrame_t f;
        f.type = type;
        f.len = len;
        memcpy(f.payload, payload, len);
        xQueueSend(self->_elrsRxQueue, &f, 0);  // drop if full; client retries
        return;
    }

    if (!self->_telemetryMutex || !payload) return;

    HexapodTelemetryStatus hexapod;
    if (type == HEXAPOD_CRSF_FRAME_TYPE &&
        hexapod_telemetry_decode(payload, len, &hexapod)) {
        bool log_error = false;
        const uint8_t severity = hexapod_error_severity(&hexapod);
        if (xSemaphoreTake(self->_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            const bool new_announcement =
                hexapod.error_sequence != 0 &&
                hexapod.error_sequence != self->_lastHexapodErrorSequence;
            if (new_announcement) {
                // Firmware advances the sequence for both a new incident and
                // its periodic "still failing" heartbeat. Only log the former;
                // a lower count marks the same key after its quiet interval.
                log_error = hexapod.error_code != self->_lastHexapodErrorCode ||
                            hexapod.error_detail != self->_lastHexapodErrorDetail ||
                            hexapod.error_count < self->_lastHexapodErrorCount ||
                            severity > self->_lastHexapodErrorSeverity;
                self->_lastHexapodErrorSequence = hexapod.error_sequence;
                self->_lastHexapodErrorCode = hexapod.error_code;
                self->_lastHexapodErrorDetail = hexapod.error_detail;
                self->_lastHexapodErrorSeverity = severity;
                self->_lastHexapodErrorCount = hexapod.error_count;
            }
            self->_telemetry.hexapod = hexapod;
            self->_telemetry.hexapod_valid = true;
            self->_telemetry.last_hexapod_ms = millis();
            self->_telemetry.voltage = (float)hexapod.battery_mv / 1000.0f;
            self->_telemetry.battery_valid =
                (hexapod.flags & HEXAPOD_FLAG_BATTERY_VALID) != 0;
            self->_telemetry.last_battery_ms = self->_telemetry.last_hexapod_ms;
            xSemaphoreGive(self->_telemetryMutex);
        }
        if (log_error) {
            char message[CRSF_LOG_MSG_MAX];
            snprintf(message, sizeof(message), "Robot %s: %s (%u)",
                     hexapod_error_severity_name(severity),
                     hexapod_error_name(hexapod.error_code),
                     hexapod.error_detail);
            self->queueLog(message);
        }
        return;
    }

    if (type == CRSF_FRAMETYPE_BATTERY_SENSOR && len == 8) {
        const uint16_t voltage_x10 =
            (uint16_t)(((uint16_t)payload[0] << 8) | payload[1]);
        if (xSemaphoreTake(self->_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            self->_telemetry.voltage = (float)voltage_x10 / 10.0f;
            self->_telemetry.battery_valid = true;
            self->_telemetry.last_battery_ms = millis();
            self->_telemetry.remaining = payload[7];
            xSemaphoreGive(self->_telemetryMutex);
        }
        return;
    }

    if (type == CRSF_FRAMETYPE_ATTITUDE && len == 6) {
        const int16_t pitch = (int16_t)(((uint16_t)payload[0] << 8) | payload[1]);
        const int16_t roll = (int16_t)(((uint16_t)payload[2] << 8) | payload[3]);
        const int16_t yaw = (int16_t)(((uint16_t)payload[4] << 8) | payload[5]);
        if (xSemaphoreTake(self->_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            self->_telemetry.pitch_deg =
                (float)pitch / 10000.0f * (180.0f / (float)M_PI);
            self->_telemetry.roll_deg =
                (float)roll / 10000.0f * (180.0f / (float)M_PI);
            self->_telemetry.yaw_deg =
                (float)yaw / 10000.0f * (180.0f / (float)M_PI);
            self->_telemetry.attitude_valid = true;
            self->_telemetry.last_attitude_ms = millis();
            xSemaphoreGive(self->_telemetryMutex);
        }
    }
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

    // Feed queued ELRS config replies into the client state machine here on
    // the LVGL thread - the same thread that runs elrs_client_poll() and all
    // other elrs_client_* calls (the client is single-threaded by design).
    if (_elrsRxQueue) {
        CRSFElrsFrame_t f;
        while (xQueueReceive(_elrsRxQueue, &f, 0) == pdTRUE) {
            elrs_client_on_frame(f.type, f.payload, f.len);
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

    const uint32_t now = millis();
    ui_telemetry_update_status(rssi, (int)t.lq, (int)t.crc_errors, uptime);
    ui_telemetry_update_battery(
        t.voltage, t.battery_valid,
        hexapod_telemetry_is_fresh(t.battery_valid, now, t.last_battery_ms));

    const uint32_t hexapod_age = t.hexapod_valid
                                     ? now - t.last_hexapod_ms
                                     : UINT32_MAX;
    const bool hexapod_fresh = hexapod_telemetry_is_fresh(
        t.hexapod_valid, now, t.last_hexapod_ms);
    if (Settings_Get()->robot_profile == ROBOT_PROFILE_HEXAPOD) {
        ui_telemetry_update_hexapod(
            t.hexapod_valid ? &t.hexapod : nullptr,
            hexapod_fresh, hexapod_age);
    }

    // Robot IMU attitude is valid only for a bounded interval. The Hexapod
    // status flags distinguish an absent sensor from stale samples.
    const bool attitude_fresh = hexapod_telemetry_is_fresh(
        t.attitude_valid, now, t.last_attitude_ms);
    bool imu_present = t.attitude_valid;
    if (t.hexapod_valid) {
        imu_present = (t.hexapod.flags & HEXAPOD_FLAG_IMU_PRESENT) != 0;
    }
    if (t.attitude_valid) {
        ui_telemetry_update_imu9(
            t.pitch_deg, t.roll_deg, t.yaw_deg,             // Row 1: Euler angles
            (float)((t.hexapod.imu_calibration >> 6) & 0x03),
            (float)((t.hexapod.imu_calibration >> 4) & 0x03),
            (float)((t.hexapod.imu_calibration >> 2) & 0x03),
            t.voltage, (float)t.lq, (float)t.snr             // Row 3: Link info
        );
    }
    ui_telemetry_set_imu_state(imu_present, attitude_fresh);
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

// Transport callback for the ELRS config client: runs on the LVGL thread,
// hands the frame to the CRSF task via the TX queue.
static void crsfElrsSendFn(uint8_t frame_type, const uint8_t* payload,
                           uint8_t len, void* user) {
    (void)user;
    CRSFLink.elrsEnqueueTx(frame_type, payload, len);
}

void CRSF_ElrsClientInit(void) {
    elrs_client_init(crsfElrsSendFn, nullptr);
    elrs_service_set_baud(CRSF_BAUD);
}
