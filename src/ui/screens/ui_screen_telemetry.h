#ifndef UI_SCREEN_TELEMETRY_H
#define UI_SCREEN_TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "../../Settings.h"

extern lv_obj_t *ui_TelemetryScreen;

// Initialize telemetry screen
void ui_create_telemetry_screen(lv_obj_t *parent);

// Refresh telemetry panels based on current robot profile
void ui_telemetry_refresh_for_profile(RobotProfile_t profile);

// Update functions for common panels
void ui_telemetry_update_status(int rssi, int latency, int errors, uint32_t uptime);
void ui_telemetry_add_log(const char *message);

// Update function for 9-DOF IMU panel (hexapod profile)
void ui_telemetry_update_imu9(float ax, float ay, float az, 
                              float gx, float gy, float gz,
                              float mx, float my, float mz);

// Get current panel index
uint8_t ui_telemetry_get_panel(void);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_TELEMETRY_H
