#ifndef UI_SCREEN_SETTINGS_H
#define UI_SCREEN_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_SettingsScreen;

void ui_create_settings_screen(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_SETTINGS_H
