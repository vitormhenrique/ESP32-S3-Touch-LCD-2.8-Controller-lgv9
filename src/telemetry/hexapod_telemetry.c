#include "hexapod_telemetry.h"

#include <stddef.h>

static uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

bool hexapod_telemetry_decode(const uint8_t *payload, uint8_t len,
                              HexapodTelemetryStatus *status)
{
    if (!payload || !status || len != HEXAPOD_STATUS_PAYLOAD_SIZE) {
        return false;
    }
    if (payload[0] != HEXAPOD_TELEMETRY_MAGIC_0 ||
        payload[1] != HEXAPOD_TELEMETRY_MAGIC_1 ||
        payload[2] != HEXAPOD_TELEMETRY_VERSION) {
        return false;
    }
    if (payload[4] >= 13 || payload[5] >= 4 || payload[6] >= 6 ||
        payload[7] >= 3 || payload[8] >= 8) {
        return false;
    }

    HexapodTelemetryStatus decoded = {0};
    decoded.flags = payload[3];
    decoded.safety_state = payload[4];
    decoded.command_source = payload[5];
    decoded.gait = payload[6];
    decoded.control_mode = payload[7];
    decoded.fault_reason = payload[8];
    decoded.speed_x255 = payload[9];
    decoded.duty_x255 = payload[10];
    decoded.imu_calibration = payload[11];
    decoded.battery_mv = read_be16(&payload[12]);
    decoded.body_height_mm = read_be16(&payload[14]);
    decoded.stride_mm = read_be16(&payload[16]);
    decoded.step_height_mm = read_be16(&payload[18]);
    decoded.tune_flags = payload[20];
    decoded.error_code = payload[21];
    decoded.error_detail = payload[22];
    decoded.error_sequence = payload[23];
    decoded.error_count = read_be16(&payload[24]);
    decoded.error_suppressed = read_be16(&payload[26]);
    *status = decoded;
    return true;
}

bool hexapod_telemetry_is_fresh(bool valid, uint32_t now_ms,
                                uint32_t last_sample_ms)
{
    return valid &&
           (uint32_t)(now_ms - last_sample_ms) < HEXAPOD_TELEMETRY_STALE_MS;
}

const char *hexapod_safety_state_name(uint8_t state)
{
    static const char *const names[] = {
        "Boot", "Config", "Disarmed", "Arming", "Stand Ready",
        "RC Manual", "Contact", "Jetson", "Maintenance", "Passive",
        "Soft Fault", "Hard Fault", "E-Stop",
    };
    return state < (sizeof(names) / sizeof(names[0])) ? names[state] : "Unknown";
}

const char *hexapod_command_source_name(uint8_t source)
{
    static const char *const names[] = {"None", "RC", "Jetson", "Mac"};
    return source < (sizeof(names) / sizeof(names[0])) ? names[source] : "Unknown";
}

const char *hexapod_gait_name(uint8_t gait)
{
    static const char *const names[] = {
        "None", "None", "Tripod", "Ripple", "Wave", "Crawl",
    };
    return gait < (sizeof(names) / sizeof(names[0])) ? names[gait] : "Unknown";
}

const char *hexapod_control_mode_name(uint8_t mode)
{
    /* What the RIGHT gimbal does; the left gimbal walks in every mode. */
    static const char *const names[] = {"Yaw", "Translate", "Rotate"};
    return mode < (sizeof(names) / sizeof(names[0])) ? names[mode] : "Unknown";
}

const char *hexapod_fault_name(uint8_t fault)
{
    static const char *const names[] = {
        "None", "RC Kill", "Host E-Stop", "RC Link", "Battery",
        "Watchdog", "DYNAMIXEL", "Arm Timeout",
    };
    return fault < (sizeof(names) / sizeof(names[0])) ? names[fault] : "Unknown";
}

const char *hexapod_tune_param_name(uint8_t param)
{
    static const char *const names[] = {"Step Height", "Stride", "Duty"};
    return param < (sizeof(names) / sizeof(names[0])) ? names[param] : "Unknown";
}

const char *hexapod_error_name(uint8_t code)
{
    /* Mirrors safety::ErrorCode in the OpenRB firmware. Keep in sync. */
    static const char *const names[] = {
        "None",           "Safety Fault",   "DXL Write",     "DXL Read",
        "DXL Hardware",   "DXL Bus Silent", "DXL Missing",   "Foot Sensor",
        "I2C Mux",        "I2C EEPROM",     "Cfg Volatile",  "Cfg Commit",
        "RC Failsafe",    "RC Frames",      "Unreachable",   "Goal Clamped",
        "Battery Low",    "Watchdog",       "Save Rejected",
    };
    return code < (sizeof(names) / sizeof(names[0])) ? names[code] : "Unknown";
}

const char *hexapod_error_severity_name(uint8_t severity)
{
    static const char *const names[] = {"Info", "Warn", "Error", "Critical"};
    return severity < (sizeof(names) / sizeof(names[0])) ? names[severity]
                                                         : "Unknown";
}

uint8_t hexapod_tune_param(const HexapodTelemetryStatus *status)
{
    if (!status) {
        return 0;
    }
    return (uint8_t)((status->tune_flags & HEXAPOD_TUNE_PARAM_MASK) >>
                     HEXAPOD_TUNE_PARAM_SHIFT);
}

uint8_t hexapod_error_severity(const HexapodTelemetryStatus *status)
{
    if (!status) {
        return 0;
    }
    return (uint8_t)((status->tune_flags & HEXAPOD_TUNE_SEVERITY_MASK) >>
                     HEXAPOD_TUNE_SEVERITY_SHIFT);
}
