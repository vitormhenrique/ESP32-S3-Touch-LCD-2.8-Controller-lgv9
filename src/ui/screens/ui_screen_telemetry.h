#ifndef UI_SCREEN_TELEMETRY_H
#define UI_SCREEN_TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "../../Settings.h"
#include "../../telemetry/hexapod_telemetry.h"

extern lv_obj_t *ui_TelemetryScreen;

// Initialize telemetry screen
void ui_create_telemetry_screen(lv_obj_t *parent);

// Refresh telemetry panels based on current robot profile
void ui_telemetry_refresh_for_profile(RobotProfile_t profile);

// Update functions for common panels
void ui_telemetry_update_status(int rssi, int latency, int errors, uint32_t uptime);
void ui_telemetry_update_battery(float voltage, bool valid);
void ui_telemetry_add_log(const char *message);

// Update function for 9-DOF IMU panel (hexapod profile)
void ui_telemetry_update_imu9(float ax, float ay, float az, 
                              float gx, float gy, float gz,
                              float mx, float my, float mz);
void ui_telemetry_set_imu_state(bool present, bool fresh);

// Update the profile-specific Hexapod page. A NULL status or fresh=false
// clears live values so stale telemetry is never presented as current.
void ui_telemetry_update_hexapod(const HexapodTelemetryStatus *status,
                                 bool fresh, uint32_t age_ms);

// Get current panel index
uint8_t ui_telemetry_get_panel(void);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_TELEMETRY_H
