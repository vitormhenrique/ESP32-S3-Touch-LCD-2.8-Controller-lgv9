/**
 * @file ui_screen_telemetry.h
 * @brief Telemetry Screen - Robot data, logs, hexapod servo info, IMU
 */

#ifndef UI_SCREEN_TELEMETRY_H
#define UI_SCREEN_TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_common.h"

//=============================================================================
// Telemetry Screen Objects
//=============================================================================

extern lv_obj_t *ui_TelemetryScreen;

//=============================================================================
// Functions
//=============================================================================

/**
 * Create the telemetry screen
 */
void ui_telemetry_screen_create(lv_obj_t *parent);

/**
 * Show/hide the telemetry screen
 */
void ui_telemetry_screen_show(bool show);

/**
 * Set robot type for telemetry display
 * @param robot_type 0 = Generic, 1 = Hexapod
 */
void ui_telemetry_set_robot_type(uint8_t robot_type);

/**
 * Update status panel values
 */
void ui_telemetry_set_status(const char *rssi, const char *latency, const char *errors, const char *uptime);

/**
 * Add log message
 */
void ui_telemetry_add_log(const char *message);

/**
 * Clear log
 */
void ui_telemetry_clear_log(void);

//=============================================================================
// Hexapod-specific functions
//=============================================================================

/**
 * Update servo data (for hexapod)
 * @param servo_id Servo ID (0-17 for 18 servos)
 * @param position Current position
 * @param load Current load
 * @param temp Temperature
 * @param voltage Voltage
 */
void ui_telemetry_set_servo(uint8_t servo_id, int16_t position, int16_t load, uint8_t temp, float voltage);

/**
 * Update IMU data
 * @param roll Roll angle in degrees
 * @param pitch Pitch angle in degrees
 * @param yaw Yaw angle in degrees
 * @param accel_x X acceleration
 * @param accel_y Y acceleration
 * @param accel_z Z acceleration
 */
void ui_telemetry_set_imu(float roll, float pitch, float yaw, float accel_x, float accel_y, float accel_z);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_TELEMETRY_H
