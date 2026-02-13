#ifndef UI_SCREEN_SETTINGS_H
#define UI_SCREEN_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Settings menu types
typedef enum {
    SETTINGS_MENU_MAIN = 0,
    SETTINGS_MENU_RADIO,
    SETTINGS_MENU_ROBOT,
    SETTINGS_MENU_TOUCH_CAL,
    SETTINGS_MENU_GIMBAL_CAL,
    SETTINGS_MENU_ABOUT
} SettingsMenu_t;

extern lv_obj_t *ui_SettingsScreen;

// Create the settings screen
void ui_create_settings_screen(lv_obj_t *parent);

// Navigate to a specific settings submenu
void ui_settings_show_menu(SettingsMenu_t menu);

// Get current settings menu
SettingsMenu_t ui_settings_get_current_menu(void);

// Touch calibration functions
void ui_touch_cal_start(void);
void ui_touch_cal_record_point(int16_t raw_x, int16_t raw_y);
bool ui_touch_cal_is_active(void);
uint8_t ui_touch_cal_get_point(void);

// Gimbal calibration functions
void ui_gimbal_cal_start(void);
void ui_gimbal_cal_record_step(void);
void ui_gimbal_cal_update(int16_t values[4]);
bool ui_gimbal_cal_is_active(void);
uint8_t ui_gimbal_cal_get_axis(void);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_SETTINGS_H
