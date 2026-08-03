#pragma once

#include <Arduino.h>
#include <AlfredoCRSF.h>
#include "InputConfig.h"
#include "ChannelPack.h"
#include "telemetry/hexapod_telemetry.h"

//=============================================================================
// Configuration
//=============================================================================

#define CRSF_BAUD               420000  // CRSF handset standard (matches working example)
#define CRSF_DEFAULT_RATE_HZ    50
#define CRSF_BOOTSTRAP_RATE_HZ  10      // Low-rate RC frames while waiting for link,
                                        // same as the working example (TX_BOOTSTRAP_RATE_HZ)
#define CRSF_LINK_TIMEOUT_MS    1000
#define CRSF_LINK_DIAG_MS       2000    // Diagnostic log interval while waiting for link
#define CRSF_PING_INTERVAL_MS   1000    // Continuous device ping keepalive (module only
                                        // talks when polled, like the stock Lua script)
#define CRSF_LOG_INTERVAL_MS    30000   // Periodic stats log every 30s

// Cold-boot recovery: if the module never responds, periodically go silent so
// its parser can time out and resync, then restart a clean CRSF stream.
// Set CRSF_RETRY_ENABLED to 0 (e.g. via build_flags) to disable.
#ifndef CRSF_RETRY_ENABLED
#define CRSF_RETRY_ENABLED      1
#endif
#define CRSF_RETRY_SILENCE_MS   3000    // If the module stays silent this long past a full retry
                                        // period, go quiet (mimics what a manual ESP32 reset does)
#define CRSF_RETRY_PERIOD_MS    10000   // Module response deadline before each silent retry cycle

// UI log queue (drained on the LVGL thread via updateUI)
#define CRSF_LOG_QUEUE_LEN      8
#define CRSF_LOG_MSG_MAX        64

// ELRS config protocol bridge: frames pass between the CRSF task (UART I/O)
// and the LVGL thread (elrs_client state machine) through these queues.
#define CRSF_ELRS_QUEUE_LEN     8
#define CRSF_ELRS_FRAME_MAX     60

typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t payload[CRSF_ELRS_FRAME_MAX];
} CRSFElrsFrame_t;

// Hardware half-duplex: let UART1 drive the tri-state buffer OE pin via the
// RS485 half-duplex RTS signal through the GPIO matrix.
// The UART TX-done interrupt releases the bus, so OE timing is immune to
// task preemption. Set to 0 to fall back to software GPIO toggling.
#ifndef CRSF_HW_HALF_DUPLEX
#define CRSF_HW_HALF_DUPLEX     0
#endif

// CRSF frame types not defined in AlfredoCRSF library
#define CRSF_FRAMETYPE_DEVICE_PING      0x28
#define CRSF_FRAMETYPE_ELRS_STATUS_REQ  0x2D    // ELRS status request (Lua: 0x2D -> 0x2E reply)
#define CRSF_ADDRESS_ELRS_LUA           0xEF    // Handset Lua origin address (ELRS TX module)
#define CRSF_TASK_STACK_SIZE    4096
#define CRSF_TASK_PRIORITY      4
#define CRSF_TASK_CORE          0

//=============================================================================
// Telemetry state (received from ELRS)
//=============================================================================

typedef struct {
    // Link statistics
    int8_t   rssi_1;             // Uplink RSSI ant 1 (dBm, positive value)
    int8_t   rssi_2;             // Uplink RSSI ant 2
    uint8_t  lq;                 // Link Quality (0-100%)
    int8_t   snr;                // SNR (dB)
    uint8_t  rf_mode;
    uint8_t  tx_power;

    // BNO085 rotation-vector attitude from the robot
    float    pitch_deg;
    float    roll_deg;
    float    yaw_deg;
    bool     attitude_valid;
    uint32_t last_attitude_ms;

    // Battery/calibration from receiver
    float    voltage;
    bool     battery_valid;
    uint32_t last_battery_ms;
    uint8_t  remaining;          // Battery remaining %

    // Versioned robot-specific status (custom CRSF frame 0x80)
    HexapodTelemetryStatus hexapod;
    bool     hexapod_valid;
    uint32_t last_hexapod_ms;

    // Connection state
    bool     link_up;
    uint32_t link_up_since_ms;
    uint32_t frames_sent;
    uint32_t frames_received;
    uint32_t crc_errors;
} CRSFTelemetry_t;

//=============================================================================
// CRSF Manager class
//=============================================================================

class CRSF_Manager {
public:
    CRSF_Manager();

    bool begin();
    void update();              // Called from CRSFTask (internal)

    // Thread-safe telemetry access (copies under mutex)
    CRSFTelemetry_t getTelemetry();

    // Update LVGL UI from telemetry - call from main loop only
    void updateUI();

    void setFrameRate(uint16_t hz);
    bool isReady() const { return _initialized; }
    bool isLinkUp() const { return _linkOk; }  // Lock-free read; used to gate low-priority work

    // ELRS config protocol bridge (thread-safe):
    // enqueue an extended frame for transmission on the CRSF bus (called from
    // the LVGL thread via the elrs_client send callback).
    bool elrsEnqueueTx(uint8_t frame_type, const uint8_t* payload, uint8_t len);

    // FreeRTOS task entry point (must be public for xTaskCreate)
    static void taskFunc(void* param);

private:
    bool _initialized;
    AlfredoCRSF _crsf;
    HardwareSerial& _serial;
    Crc8 _crc;

    // Timing
    uint32_t _frameIntervalUs;
    uint32_t _lastFrameUs;
    uint8_t  _frameRateHz;

    // Telemetry (written by CRSFTask, read by main loop)
    CRSFTelemetry_t _telemetry;
    SemaphoreHandle_t _telemetryMutex;

    // Bootstrap / link detection
    uint32_t _bootstrapIntervalUs;
    uint32_t _lastBootstrapUs;
    uint32_t _lastDiagMs;
    uint32_t _lastModulePacketMs;    // Last genuine packet from TX module (not echo)
    uint32_t _lastPingMs;
    bool     _linkOk;

    // Log state tracking
    bool     _prevLinkUp;
    bool     _prevAttitudeValid;
    uint32_t _lastLogMs;
    uint32_t _lastStatsLogMs;

    // UI log queue: CRSF task produces, LVGL thread (updateUI) consumes.
    // LVGL is NOT thread-safe - calling ui_telemetry_add_log directly from
    // the CRSF task races the core-1 renderer and corrupts memory.
    QueueHandle_t _logQueue;
    void queueLog(const char* msg);

    // ELRS config bridge queues (see elrsEnqueueTx / updateUI)
    QueueHandle_t _elrsTxQueue;   // LVGL thread -> CRSF task (outgoing frames)
    QueueHandle_t _elrsRxQueue;   // CRSF task -> LVGL thread (config replies)
    static void rawFrameCb(uint8_t type, const uint8_t* payload,
                           uint8_t len, void* user);
    void sendElrsQueued();

    // Internal methods
    void gatherAndPackChannels(uint16_t channels[CPACK_NUM_CHANNELS]);
    void sendRcChannelsPacked(const uint16_t channels[CPACK_NUM_CHANNELS]);
    void sendBootstrapFrame();
    void sendDevicePing();
    void sendLinkStatRequest();
    void setOeMode(bool txMode);
    void discardEcho(size_t bytesSent);
    void processTelemetry();
    void generateLogs();

};

// Global instance
extern CRSF_Manager CRSFLink;

// C-linkage convenience functions
#ifdef __cplusplus
extern "C" {
#endif
void CRSF_Init(void);
// Initialize the ELRS config client with the CRSF UART transport.
// Must be called on the main/LVGL thread BEFORE the UI is created
// (elrs_client_init resets state, including UI-registered callbacks).
void CRSF_ElrsClientInit(void);
#ifdef __cplusplus
}
#endif
