#pragma once

#include <Arduino.h>
#include <AlfredoCRSF.h>
#include "InputConfig.h"
#include "ChannelPack.h"

//=============================================================================
// Configuration
//=============================================================================

#define CRSF_BAUD               420000
#define CRSF_DEFAULT_RATE_HZ    50
#define CRSF_BOOTSTRAP_RATE_HZ  10      // Low-rate bootstrap to minimize echo while waking TX module
#define CRSF_LINK_TIMEOUT_MS    1000
#define CRSF_LINK_DIAG_MS       2000    // Diagnostic log interval while waiting for link
#define CRSF_PING_INTERVAL_MS   1000    // Device ping interval while waiting for link
#define CRSF_LOG_INTERVAL_MS    30000   // Periodic stats log every 30s

// CRSF frame types not defined in AlfredoCRSF library
#define CRSF_FRAMETYPE_DEVICE_PING  0x28
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

    // BNO055 Attitude from receiver
    float    pitch_deg;
    float    roll_deg;
    float    yaw_deg;
    bool     attitude_valid;
    uint32_t last_attitude_ms;

    // Battery/calibration from receiver
    float    voltage;
    uint8_t  bno_cal_sys;        // BNO055 calibration (0-3)
    uint8_t  bno_cal_gyro;
    uint8_t  bno_cal_accel;
    uint8_t  bno_cal_mag;
    uint8_t  remaining;          // Battery remaining %

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

    // Internal methods
    void gatherAndPackChannels(uint16_t channels[CPACK_NUM_CHANNELS]);
    void sendRcChannelsPacked(const uint16_t channels[CPACK_NUM_CHANNELS]);
    void sendBootstrapFrame();
    void sendDevicePing();
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
#ifdef __cplusplus
}
#endif
