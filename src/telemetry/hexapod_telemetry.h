#ifndef HEXAPOD_TELEMETRY_H
#define HEXAPOD_TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEXAPOD_CRSF_FRAME_TYPE 0x80u
#define HEXAPOD_TELEMETRY_MAGIC_0 0x48u
#define HEXAPOD_TELEMETRY_MAGIC_1 0x58u
#define HEXAPOD_TELEMETRY_VERSION 2u
#define HEXAPOD_TELEMETRY_STALE_MS 2000u
#define HEXAPOD_STATUS_PAYLOAD_SIZE 28u

#define HEXAPOD_FLAG_ARMED       (1u << 0)
#define HEXAPOD_FLAG_MOTION_GATE (1u << 1)
#define HEXAPOD_FLAG_KILL        (1u << 2)
#define HEXAPOD_FLAG_FAILSAFE    (1u << 3)
#define HEXAPOD_FLAG_IMU_PRESENT (1u << 4)
#define HEXAPOD_FLAG_IMU_FRESH   (1u << 5)
#define HEXAPOD_FLAG_FAULT       (1u << 6)
#define HEXAPOD_FLAG_BATTERY_VALID (1u << 7)

/* Second flag byte (v2): gait-tune editor state and reported error severity. */
#define HEXAPOD_TUNE_ACTIVE      (1u << 0)
#define HEXAPOD_TUNE_PREVIEW     (1u << 1)
#define HEXAPOD_TUNE_SAVE_PENDING (1u << 2)
#define HEXAPOD_TUNE_CFG_VOLATILE (1u << 3)
#define HEXAPOD_TUNE_PARAM_SHIFT 4
#define HEXAPOD_TUNE_PARAM_MASK  (0x03u << HEXAPOD_TUNE_PARAM_SHIFT)
#define HEXAPOD_TUNE_SEVERITY_SHIFT 6
#define HEXAPOD_TUNE_SEVERITY_MASK  (0x03u << HEXAPOD_TUNE_SEVERITY_SHIFT)

/* Gait parameters the NAV1 cluster can edit (payload param field). */
#define HEXAPOD_TUNE_PARAM_STEP_HEIGHT 0u
#define HEXAPOD_TUNE_PARAM_STRIDE      1u
#define HEXAPOD_TUNE_PARAM_DUTY        2u

typedef struct {
    uint8_t flags;
    uint8_t safety_state;
    uint8_t command_source;
    uint8_t gait;
    uint8_t control_mode;
    uint8_t fault_reason;
    uint8_t speed_x255;
    uint8_t duty_x255;
    uint8_t imu_calibration;
    uint16_t battery_mv;
    uint16_t body_height_mm;
    uint16_t stride_mm;
    uint16_t step_height_mm;
    /* v2 */
    uint8_t tune_flags;
    uint8_t error_code;
    uint8_t error_detail;
    uint8_t error_sequence;
    uint16_t error_count;
    uint16_t error_suppressed;
} HexapodTelemetryStatus;

bool hexapod_telemetry_decode(const uint8_t *payload, uint8_t len,
                              HexapodTelemetryStatus *status);
bool hexapod_telemetry_is_fresh(bool valid, uint32_t now_ms,
                                uint32_t last_sample_ms);

const char *hexapod_safety_state_name(uint8_t state);
const char *hexapod_command_source_name(uint8_t source);
const char *hexapod_gait_name(uint8_t gait);
const char *hexapod_control_mode_name(uint8_t mode);
const char *hexapod_fault_name(uint8_t fault);
const char *hexapod_tune_param_name(uint8_t param);
const char *hexapod_error_name(uint8_t code);
const char *hexapod_error_severity_name(uint8_t severity);

/* Convenience accessors for the packed v2 tune-flag byte. */
uint8_t hexapod_tune_param(const HexapodTelemetryStatus *status);
uint8_t hexapod_error_severity(const HexapodTelemetryStatus *status);

#ifdef __cplusplus
}
#endif

#endif
