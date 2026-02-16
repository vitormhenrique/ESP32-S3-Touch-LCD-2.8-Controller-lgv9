#include "CRSF_Manager.h"
#include "InputManager.h"
#include "ui/screens/ui_screen_telemetry.h"
#include <cstring>
#include <cmath>
#include <driver/gpio.h>  // For gpio_set_pull_mode

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
    , _prevLinkUp(false)
    , _prevAttitudeValid(false)
    , _lastLogMs(0)
    , _lastStatsLogMs(0)
{
}

//=============================================================================
// Initialization
//=============================================================================

bool CRSF_Manager::begin() {
    printf("[CRSF] Initializing...\n");
    printf("[CRSF] TX Pin: %d, RX Pin: %d, OE Pin: %d\n",
           CRSF_UART_TX_PIN, CRSF_UART_RX_PIN, CRSF_OE_PIN);

    // Configure OE pin for SN74LVC1G125 buffer control
    // Active-low: HIGH = hi-Z (RX mode), LOW = output active (TX mode)
    pinMode(CRSF_OE_PIN, OUTPUT);
    digitalWrite(CRSF_OE_PIN, HIGH);  // Default: RX mode
    // Enable internal pull-up to ensure buffer stays tri-stated during boot/glitches
    gpio_set_pull_mode((gpio_num_t)CRSF_OE_PIN, GPIO_PULLUP_ONLY);

    // Initialize UART
    _serial.begin(CRSF_BAUD, SERIAL_8N1, CRSF_UART_RX_PIN, CRSF_UART_TX_PIN);

    // Initialize CRSF library
    _crsf.begin(_serial);

    // Create telemetry mutex
    _telemetryMutex = xSemaphoreCreateMutex();
    if (!_telemetryMutex) {
        printf("[CRSF] ERROR: Failed to create mutex\n");
        return false;
    }

    _initialized = true;
    printf("[CRSF] Initialized at %d baud, %d Hz frame rate\n",
           CRSF_BAUD, _frameRateHz);

    return true;
}

//=============================================================================
// FreeRTOS Task
//=============================================================================

void CRSF_Manager::taskFunc(void* param) {
    CRSF_Manager* self = static_cast<CRSF_Manager*>(param);

    // Small delay to let InputManager initialize
    vTaskDelay(pdMS_TO_TICKS(500));
    printf("[CRSF] Task started on core %d\n", xPortGetCoreID());

    while (true) {
        self->update();
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz
    }
}

//=============================================================================
// Main update (called from CRSF task at 50Hz)
//=============================================================================

void CRSF_Manager::update() {
    if (!_initialized) return;

    uint32_t nowUs = micros();

    // Process incoming telemetry from ELRS TX module
    processTelemetry();

    // Send RC channels at configured rate
    if ((nowUs - _lastFrameUs) >= _frameIntervalUs) {
        _lastFrameUs = nowUs;

        if (RCInput.isReady()) {
            uint16_t channels[CPACK_NUM_CHANNELS];
            gatherAndPackChannels(channels);
            sendRcChannelsPacked(channels);

            if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                _telemetry.frames_sent++;
                xSemaphoreGive(_telemetryMutex);
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

    // 8 two-position switches (indices 0-7 in SWITCH_CONFIGS)
    for (int i = 0; i < 8; i++) {
        inputs.switches[i] = RCInput.isSwitchOn(i);
    }

    // 3 buttons (BTN_1=idx 8, BTN_3=idx 9, BTN_4=idx 10)
    inputs.buttons[0] = RCInput.isSwitchOn(8);   // BTN_1
    inputs.buttons[1] = RCInput.isSwitchOn(9);   // BTN_3
    inputs.buttons[2] = RCInput.isSwitchOn(10);  // BTN_4

    // 3-position toggles
    inputs.toggles[0] = (uint8_t)RCInput.getToggle3Pos(TOGGLE_3POS_1);
    inputs.toggles[1] = (uint8_t)RCInput.getToggle3Pos(TOGGLE_3POS_2);

    // Nav switches (NAV1: indices 11-15, NAV2: indices 16-20)
    for (int i = 0; i < 5; i++) {
        inputs.nav[0][i] = RCInput.isSwitchOn(11 + i);  // NAV1 U,D,L,R,C
        inputs.nav[1][i] = RCInput.isSwitchOn(16 + i);  // NAV2 U,D,L,R,C
    }

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

    // Half-duplex with hardware buffer (SN74LVC1G125DCKR):
    // When using a tri-state buffer, we DON'T get echo - the buffer isolates TX from RX.
    // So no need to discard echo like in one-wire mode.
    setOeMode(true);           // OE LOW = buffer enabled, TX drives line
    delayMicroseconds(2);      // Let OE settle

    _serial.write(frame, frameLen);
    _serial.flush();           // Wait for TX complete

    setOeMode(false);          // OE HIGH = buffer tri-state, RX can receive
}

void CRSF_Manager::setOeMode(bool txMode) {
    // SN74LVC1G125: OE is active-low
    // LOW = buffer output active (TX drives bus)
    // HIGH = buffer output hi-Z (RX listens)
    digitalWrite(CRSF_OE_PIN, txMode ? LOW : HIGH);
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

    if (xSemaphoreTake(_telemetryMutex, pdMS_TO_TICKS(5)) != pdTRUE) return;

    // Link state
    bool linkUp = _crsf.isLinkUp();
    _telemetry.link_up = linkUp;
    if (linkUp && !_prevLinkUp) {
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
        ui_telemetry_add_log("CRSF: Link UP");
        printf("[CRSF] Link UP\n");
    } else if (!t.link_up && _prevLinkUp) {
        ui_telemetry_add_log("CRSF: Link DOWN");
        printf("[CRSF] Link DOWN\n");
    }
    _prevLinkUp = t.link_up;

    // First attitude received
    if (t.attitude_valid && !_prevAttitudeValid) {
        ui_telemetry_add_log("CRSF: IMU data received");
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
            ui_telemetry_add_log(buf);
        }
    }
}

//=============================================================================
// UI update (must be called from main loop / LVGL thread)
//=============================================================================

void CRSF_Manager::updateUI() {
    if (!_initialized) return;

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

    ui_telemetry_add_log("CRSF: Initialized");
    printf("[CRSF] Task created on core %d, priority %d\n",
           CRSF_TASK_CORE, CRSF_TASK_PRIORITY);
}
