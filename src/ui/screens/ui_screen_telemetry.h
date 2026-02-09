#ifndef UI_SCREEN_TELEMETRY_H
#define UI_SCREEN_TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_TelemetryScreen;

void ui_create_telemetry_screen(lv_obj_t *parent);
void ui_update_telemetry_value(uint8_t index, const char *label, const char *value);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_TELEMETRY_H
