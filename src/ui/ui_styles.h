#ifndef UI_STYLES_H
#define UI_STYLES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_helpers.h"

extern lv_style_t style_panel;
extern lv_style_t style_card;
extern lv_style_t style_gimbal_bg;
extern lv_style_t style_gimbal_dot;
extern lv_style_t style_switch_off;
extern lv_style_t style_switch_on;
extern lv_style_t style_btn_default;
extern lv_style_t style_btn_pressed;
extern lv_style_t style_toggle3_bg;
extern lv_style_t style_toggle3_indicator;
extern lv_style_t style_bar_bg;
extern lv_style_t style_bar_indicator;
extern lv_style_t style_text_primary;
extern lv_style_t style_text_secondary;
extern lv_style_t style_text_small;
extern lv_style_t style_nav_active;
extern lv_style_t style_nav_inactive;

void ui_styles_init(void);

#ifdef __cplusplus
}
#endif

#endif // UI_STYLES_H
