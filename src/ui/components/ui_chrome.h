#ifndef UI_CHROME_H
#define UI_CHROME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "../ui_styles.h" // For ScreenType_t definition? No, ScreenType_t is in ui_custom.h originally.
// I should define ScreenType_t in ui_helpers.h or a new ui_types.h

// Move ScreenType_t to ui_helpers.h

extern lv_obj_t *ui_HeaderPanel;
extern lv_obj_t *ui_TitleLabel;
extern lv_obj_t *ui_StatusIcon;
extern lv_obj_t *ui_BatteryIcon;
extern lv_obj_t *ui_BatteryLabel;

extern lv_obj_t *ui_NavPanel;
extern lv_obj_t *ui_NavBtnInput;
extern lv_obj_t *ui_NavBtnTelemetry;
extern lv_obj_t *ui_NavBtnSettings;

void ui_create_header(lv_obj_t *parent);
void ui_create_nav_bar(lv_obj_t *parent);
void ui_update_nav_buttons(int active_screen_index);

void ui_set_battery(uint8_t percent, int state); // int for BatteryState_t
void ui_set_status(bool connected);
void ui_set_title(const char *title);

#ifdef __cplusplus
}
#endif

#endif // UI_CHROME_H
