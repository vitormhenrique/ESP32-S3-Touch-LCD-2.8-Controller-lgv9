#include "ui_styles.h"

lv_style_t style_panel;
lv_style_t style_card;
lv_style_t style_gimbal_bg;
lv_style_t style_gimbal_dot;
lv_style_t style_switch_off;
lv_style_t style_switch_on;
lv_style_t style_btn_default;
lv_style_t style_btn_pressed;
lv_style_t style_toggle3_bg;
lv_style_t style_toggle3_indicator;
lv_style_t style_bar_bg;
lv_style_t style_bar_indicator;
lv_style_t style_text_primary;
lv_style_t style_text_secondary;
lv_style_t style_text_small;
lv_style_t style_nav_active;
lv_style_t style_nav_inactive;

static bool styles_initialized = false;

void ui_styles_init(void)
{
    if (styles_initialized) return;
    
    // Panel style
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, lv_color_hex(UI_COLOR_BG_PANEL));
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_border_color(&style_panel, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_radius(&style_panel, 0);
    lv_style_set_pad_all(&style_panel, 2);
    
    // Card style
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, lv_color_hex(UI_COLOR_BORDER_ACCENT));
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, 4);
    lv_style_set_pad_all(&style_card, 2);
    
    // Gimbal background
    lv_style_init(&style_gimbal_bg);
    lv_style_set_bg_color(&style_gimbal_bg, lv_color_hex(UI_COLOR_BG_DARK));
    lv_style_set_bg_opa(&style_gimbal_bg, LV_OPA_COVER);
    lv_style_set_border_color(&style_gimbal_bg, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_border_width(&style_gimbal_bg, 2);
    lv_style_set_radius(&style_gimbal_bg, 6);
    lv_style_set_pad_all(&style_gimbal_bg, 0);
    
    // Gimbal dot
    lv_style_init(&style_gimbal_dot);
    lv_style_set_bg_color(&style_gimbal_dot, lv_color_hex(UI_COLOR_ACCENT_CYAN));
    lv_style_set_bg_opa(&style_gimbal_dot, LV_OPA_COVER);
    lv_style_set_radius(&style_gimbal_dot, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style_gimbal_dot, 0);
    lv_style_set_shadow_width(&style_gimbal_dot, 6);
    lv_style_set_shadow_color(&style_gimbal_dot, lv_color_hex(UI_COLOR_ACCENT_CYAN));
    lv_style_set_shadow_opa(&style_gimbal_dot, LV_OPA_50);
    
    // Switch OFF
    lv_style_init(&style_switch_off);
    lv_style_set_bg_color(&style_switch_off, lv_color_hex(UI_COLOR_SWITCH_OFF));
    lv_style_set_bg_opa(&style_switch_off, LV_OPA_COVER);
    lv_style_set_radius(&style_switch_off, 3);
    lv_style_set_border_color(&style_switch_off, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&style_switch_off, 1);
    
    // Switch ON
    lv_style_init(&style_switch_on);
    lv_style_set_bg_color(&style_switch_on, lv_color_hex(UI_COLOR_SWITCH_ON));
    lv_style_set_bg_opa(&style_switch_on, LV_OPA_COVER);
    lv_style_set_radius(&style_switch_on, 3);
    lv_style_set_border_width(&style_switch_on, 0);
    lv_style_set_shadow_width(&style_switch_on, 3);
    lv_style_set_shadow_color(&style_switch_on, lv_color_hex(UI_COLOR_SWITCH_ON));
    lv_style_set_shadow_opa(&style_switch_on, LV_OPA_40);
    
    // Button default
    lv_style_init(&style_btn_default);
    lv_style_set_bg_color(&style_btn_default, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&style_btn_default, LV_OPA_COVER);
    lv_style_set_border_color(&style_btn_default, lv_color_hex(UI_COLOR_BORDER_ACCENT));
    lv_style_set_border_width(&style_btn_default, 1);
    lv_style_set_radius(&style_btn_default, 3);
    lv_style_set_pad_all(&style_btn_default, 2);
    
    // Button pressed
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&style_btn_pressed, 0);
    lv_style_set_radius(&style_btn_pressed, 3);
    lv_style_set_shadow_width(&style_btn_pressed, 4);
    lv_style_set_shadow_color(&style_btn_pressed, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_shadow_opa(&style_btn_pressed, LV_OPA_50);
    
    // Toggle 3-pos background
    lv_style_init(&style_toggle3_bg);
    lv_style_set_bg_color(&style_toggle3_bg, lv_color_hex(UI_COLOR_BG_DARK));
    lv_style_set_bg_opa(&style_toggle3_bg, LV_OPA_COVER);
    lv_style_set_border_color(&style_toggle3_bg, lv_color_hex(UI_COLOR_BORDER_ACCENT));
    lv_style_set_border_width(&style_toggle3_bg, 1);
    lv_style_set_radius(&style_toggle3_bg, 8);
    lv_style_set_pad_all(&style_toggle3_bg, 2);
    
    // Toggle 3-pos indicator
    lv_style_init(&style_toggle3_indicator);
    lv_style_set_bg_color(&style_toggle3_indicator, lv_color_hex(UI_COLOR_ACCENT_ORANGE));
    lv_style_set_bg_opa(&style_toggle3_indicator, LV_OPA_COVER);
    lv_style_set_radius(&style_toggle3_indicator, 4);
    lv_style_set_border_width(&style_toggle3_indicator, 0);
    
    // Bar background
    lv_style_init(&style_bar_bg);
    lv_style_set_bg_color(&style_bar_bg, lv_color_hex(UI_COLOR_BG_DARK));
    lv_style_set_bg_opa(&style_bar_bg, LV_OPA_COVER);
    lv_style_set_radius(&style_bar_bg, 3);
    lv_style_set_border_color(&style_bar_bg, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&style_bar_bg, 1);
    
    // Bar indicator
    lv_style_init(&style_bar_indicator);
    lv_style_set_bg_color(&style_bar_indicator, lv_color_hex(UI_COLOR_ACCENT_PURPLE));
    lv_style_set_bg_opa(&style_bar_indicator, LV_OPA_COVER);
    lv_style_set_radius(&style_bar_indicator, 2);
    
    // Text primary
    lv_style_init(&style_text_primary);
    lv_style_set_text_color(&style_text_primary, lv_color_hex(UI_COLOR_TEXT_PRIMARY));
    
    // Text secondary
    lv_style_init(&style_text_secondary);
    lv_style_set_text_color(&style_text_secondary, lv_color_hex(UI_COLOR_TEXT_SECONDARY));
    
    // Text small
    lv_style_init(&style_text_small);
    lv_style_set_text_color(&style_text_small, lv_color_hex(UI_COLOR_TEXT_TERTIARY));
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_10);
    
    // Nav active - pill with translucent accent tint and soft glow
    lv_style_init(&style_nav_active);
    lv_style_set_bg_color(&style_nav_active, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_bg_opa(&style_nav_active, LV_OPA_20);
    lv_style_set_radius(&style_nav_active, LV_RADIUS_CIRCLE);
    lv_style_set_border_color(&style_nav_active, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_border_width(&style_nav_active, 1);
    lv_style_set_border_opa(&style_nav_active, LV_OPA_50);
    lv_style_set_shadow_width(&style_nav_active, 8);
    lv_style_set_shadow_color(&style_nav_active, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_shadow_opa(&style_nav_active, LV_OPA_30);
    lv_style_set_text_color(&style_nav_active, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    
    // Nav inactive - flat/transparent, muted text
    lv_style_init(&style_nav_inactive);
    lv_style_set_bg_color(&style_nav_inactive, lv_color_hex(UI_COLOR_BG_PANEL));
    lv_style_set_bg_opa(&style_nav_inactive, LV_OPA_TRANSP);
    lv_style_set_radius(&style_nav_inactive, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style_nav_inactive, 0);
    lv_style_set_shadow_width(&style_nav_inactive, 0);
    lv_style_set_text_color(&style_nav_inactive, lv_color_hex(UI_COLOR_TEXT_SECONDARY));
    
    styles_initialized = true;
}
