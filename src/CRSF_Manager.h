#pragma once

#include <Arduino.h>
#include <AlfredoCRSF.h>
#include "InputConfig.h"
#include "ChannelPack.h"

//=============================================================================
// Configuration
//=============================================================================

#define CRSF_BAUD               400000
#define CRSF_DEFAULT_RATE_HZ    50
#define CRSF_BOOTSTRAP_RATE_HZ  50      // ELRS TX module UART watchdog re-evaluates good/bad
                                        // packets every 1s and cycles baud rates when unhappy;
                                        // a steady 50Hz stream locks it quickly and reliably.
                                        // (Echo is no longer a concern with HW half-duplex.)
#define CRSF_LINK_TIMEOUT_MS    1000
#define CRSF_LINK_DIAG_MS       2000    // Diagnostic log interval while waiting for link
#define CRSF_PING_INTERVAL_MS   1000    // Device ping interval while waiting for link
#define CRSF_LOG_INTERVAL_MS    30000   // Periodic stats log every 30s
#define CRSF_BOOT_QUIET_MS      15000   // Do not even configure the CRSF UART before this
                                        // point after power-on. Some ELRS TX modules are still
                                        // booting/autobauding at 5s and can wedge until the
                                        // handset MCU is reset while the module stays powered.
#define CRSF_RETRY_SILENCE_MS   3000    // If the module stays silent this long past a full retry
                                        // period, go quiet (mimics what a manual ESP32 reset does)
#define CRSF_RETRY_PERIOD_MS    10000   // Module response deadline before each silent retry cycle

// UI log queue (drained on the LVGL thread via updateUI)
#define CRSF_LOG_QUEUE_LEN      8
#define CRSF_LOG_MSG_MAX        64

// Hardware half-duplex: let UART1 drive the SN74LVC1G125 OE pin via the
// RS485 half-duplex RTS signal (inverted through the GPIO matrix).
// The UART TX-done interrupt releases the bus, so OE timing is immune to
// task preemption. Set to 0 to fall back to software GPIO toggling.
#ifndef CRSF_HW_HALF_DUPLEX
#define CRSF_HW_HALF_DUPLEX     1
#endif

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
    bool isLinkUp() const { return _linkOk; }  // Lock-free read; used to gate low-priority work

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
