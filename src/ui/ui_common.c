/**
 * @file ui_common.c
 * @brief Common UI styles and widget creation functions
 */

#include "ui_common.h"
#include <stdio.h>

// Global styles
UI_Styles_t ui_styles;
static bool styles_initialized = false;

//=============================================================================
// Style Initialization
//=============================================================================

void ui_styles_init(void)
{
    if (styles_initialized) return;
    
    // Panel style
    lv_style_init(&ui_styles.panel);
    lv_style_set_bg_color(&ui_styles.panel, lv_color_hex(UI_COLOR_BG_PANEL));
    lv_style_set_bg_opa(&ui_styles.panel, LV_OPA_COVER);
    lv_style_set_border_color(&ui_styles.panel, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&ui_styles.panel, 1);
    lv_style_set_radius(&ui_styles.panel, 0);
    lv_style_set_pad_all(&ui_styles.panel, 2);
    
    // Card style
    lv_style_init(&ui_styles.card);
    lv_style_set_bg_color(&ui_styles.card, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.card, LV_OPA_COVER);
    lv_style_set_border_color(&ui_styles.card, lv_color_hex(UI_COLOR_BORDER_ACCENT));
    lv_style_set_border_width(&ui_styles.card, 1);
    lv_style_set_radius(&ui_styles.card, 4);
    lv_style_set_pad_all(&ui_styles.card, 2);
    
    // Gimbal background
    lv_style_init(&ui_styles.gimbal_bg);
    lv_style_set_bg_color(&ui_styles.gimbal_bg, lv_color_hex(UI_COLOR_BG_DARK));
    lv_style_set_bg_opa(&ui_styles.gimbal_bg, LV_OPA_COVER);
    lv_style_set_border_color(&ui_styles.gimbal_bg, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_border_width(&ui_styles.gimbal_bg, 2);
    lv_style_set_radius(&ui_styles.gimbal_bg, 6);
    lv_style_set_pad_all(&ui_styles.gimbal_bg, 0);
    
    // Gimbal dot
    lv_style_init(&ui_styles.gimbal_dot);
    lv_style_set_bg_color(&ui_styles.gimbal_dot, lv_color_hex(UI_COLOR_ACCENT_CYAN));
    lv_style_set_bg_opa(&ui_styles.gimbal_dot, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.gimbal_dot, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&ui_styles.gimbal_dot, 0);
    lv_style_set_shadow_width(&ui_styles.gimbal_dot, 6);
    lv_style_set_shadow_color(&ui_styles.gimbal_dot, lv_color_hex(UI_COLOR_ACCENT_CYAN));
    lv_style_set_shadow_opa(&ui_styles.gimbal_dot, LV_OPA_50);
    
    // Switch OFF
    lv_style_init(&ui_styles.switch_off);
    lv_style_set_bg_color(&ui_styles.switch_off, lv_color_hex(UI_COLOR_SWITCH_OFF));
    lv_style_set_bg_opa(&ui_styles.switch_off, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.switch_off, 3);
    lv_style_set_border_color(&ui_styles.switch_off, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&ui_styles.switch_off, 1);
    
    // Switch ON
    lv_style_init(&ui_styles.switch_on);
    lv_style_set_bg_color(&ui_styles.switch_on, lv_color_hex(UI_COLOR_SWITCH_ON));
    lv_style_set_bg_opa(&ui_styles.switch_on, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.switch_on, 3);
    lv_style_set_border_width(&ui_styles.switch_on, 0);
    lv_style_set_shadow_width(&ui_styles.switch_on, 3);
    lv_style_set_shadow_color(&ui_styles.switch_on, lv_color_hex(UI_COLOR_SWITCH_ON));
    lv_style_set_shadow_opa(&ui_styles.switch_on, LV_OPA_40);
    
    // Button default
    lv_style_init(&ui_styles.btn_default);
    lv_style_set_bg_color(&ui_styles.btn_default, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.btn_default, LV_OPA_COVER);
    lv_style_set_border_color(&ui_styles.btn_default, lv_color_hex(UI_COLOR_BORDER_ACCENT));
    lv_style_set_border_width(&ui_styles.btn_default, 1);
    lv_style_set_radius(&ui_styles.btn_default, 3);
    lv_style_set_pad_all(&ui_styles.btn_default, 2);
    
    // Button pressed
    lv_style_init(&ui_styles.btn_pressed);
    lv_style_set_bg_color(&ui_styles.btn_pressed, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_bg_opa(&ui_styles.btn_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&ui_styles.btn_pressed, 0);
    lv_style_set_radius(&ui_styles.btn_pressed, 3);
    lv_style_set_shadow_width(&ui_styles.btn_pressed, 4);
    lv_style_set_shadow_color(&ui_styles.btn_pressed, lv_color_hex(UI_COLOR_ACCENT_BLUE));
    lv_style_set_shadow_opa(&ui_styles.btn_pressed, LV_OPA_50);
    
    // Button highlight (for nav switch highlighting)
    lv_style_init(&ui_styles.btn_highlight);
    lv_style_set_bg_color(&ui_styles.btn_highlight, lv_color_hex(UI_COLOR_ACCENT_GREEN));
    lv_style_set_bg_opa(&ui_styles.btn_highlight, LV_OPA_COVER);
    lv_style_set_border_width(&ui_styles.btn_highlight, 0);
    lv_style_set_radius(&ui_styles.btn_highlight, 3);
    lv_style_set_shadow_width(&ui_styles.btn_highlight, 4);
    lv_style_set_shadow_color(&ui_styles.btn_highlight, lv_color_hex(UI_COLOR_ACCENT_GREEN));
    lv_style_set_shadow_opa(&ui_styles.btn_highlight, LV_OPA_50);
    
    // Toggle 3-pos background
    lv_style_init(&ui_styles.toggle3_bg);
    lv_style_set_bg_color(&ui_styles.toggle3_bg, lv_color_hex(UI_COLOR_BG_DARK));
    lv_style_set_bg_opa(&ui_styles.toggle3_bg, LV_OPA_COVER);
    lv_style_set_border_color(&ui_styles.toggle3_bg, lv_color_hex(UI_COLOR_BORDER_ACCENT));
    lv_style_set_border_width(&ui_styles.toggle3_bg, 1);
    lv_style_set_radius(&ui_styles.toggle3_bg, 8);
    lv_style_set_pad_all(&ui_styles.toggle3_bg, 2);
    
    // Toggle 3-pos indicator
    lv_style_init(&ui_styles.toggle3_indicator);
    lv_style_set_bg_color(&ui_styles.toggle3_indicator, lv_color_hex(UI_COLOR_ACCENT_ORANGE));
    lv_style_set_bg_opa(&ui_styles.toggle3_indicator, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.toggle3_indicator, 4);
    lv_style_set_border_width(&ui_styles.toggle3_indicator, 0);
    
    // Bar background
    lv_style_init(&ui_styles.bar_bg);
    lv_style_set_bg_color(&ui_styles.bar_bg, lv_color_hex(UI_COLOR_BG_DARK));
    lv_style_set_bg_opa(&ui_styles.bar_bg, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.bar_bg, 3);
    lv_style_set_border_color(&ui_styles.bar_bg, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&ui_styles.bar_bg, 1);
    
    // Bar indicator
    lv_style_init(&ui_styles.bar_indicator);
    lv_style_set_bg_color(&ui_styles.bar_indicator, lv_color_hex(UI_COLOR_ACCENT_PURPLE));
    lv_style_set_bg_opa(&ui_styles.bar_indicator, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.bar_indicator, 2);
    
    // Text primary
    lv_style_init(&ui_styles.text_primary);
    lv_style_set_text_color(&ui_styles.text_primary, lv_color_hex(UI_COLOR_TEXT_PRIMARY));
    
    // Text secondary
    lv_style_init(&ui_styles.text_secondary);
    lv_style_set_text_color(&ui_styles.text_secondary, lv_color_hex(UI_COLOR_TEXT_SECONDARY));
    
    // Text small
    lv_style_init(&ui_styles.text_small);
    lv_style_set_text_color(&ui_styles.text_small, lv_color_hex(UI_COLOR_TEXT_TERTIARY));
    lv_style_set_text_font(&ui_styles.text_small, &lv_font_montserrat_12);
    
    // Nav active
    lv_style_init(&ui_styles.nav_active);
    lv_style_set_bg_color(&ui_styles.nav_active, lv_color_hex(UI_COLOR_NAV_ACTIVE));
    lv_style_set_bg_opa(&ui_styles.nav_active, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_active, 4);
    lv_style_set_border_width(&ui_styles.nav_active, 0);
    lv_style_set_text_color(&ui_styles.nav_active, lv_color_hex(UI_COLOR_TEXT_PRIMARY));
    
    // Nav inactive
    lv_style_init(&ui_styles.nav_inactive);
    lv_style_set_bg_color(&ui_styles.nav_inactive, lv_color_hex(UI_COLOR_NAV_INACTIVE));
    lv_style_set_bg_opa(&ui_styles.nav_inactive, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_inactive, 4);
    lv_style_set_border_color(&ui_styles.nav_inactive, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&ui_styles.nav_inactive, 1);
    lv_style_set_text_color(&ui_styles.nav_inactive, lv_color_hex(UI_COLOR_TEXT_SECONDARY));
    
    // Navigation button styles for 5-way switch
    lv_style_init(&ui_styles.nav_btn_up);
    lv_style_set_bg_color(&ui_styles.nav_btn_up, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.nav_btn_up, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_btn_up, 2);
    lv_style_set_border_width(&ui_styles.nav_btn_up, 1);
    lv_style_set_border_color(&ui_styles.nav_btn_up, lv_color_hex(UI_COLOR_BORDER));
    
    lv_style_init(&ui_styles.nav_btn_down);
    lv_style_set_bg_color(&ui_styles.nav_btn_down, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.nav_btn_down, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_btn_down, 2);
    lv_style_set_border_width(&ui_styles.nav_btn_down, 1);
    lv_style_set_border_color(&ui_styles.nav_btn_down, lv_color_hex(UI_COLOR_BORDER));
    
    lv_style_init(&ui_styles.nav_btn_left);
    lv_style_set_bg_color(&ui_styles.nav_btn_left, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.nav_btn_left, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_btn_left, 2);
    lv_style_set_border_width(&ui_styles.nav_btn_left, 1);
    lv_style_set_border_color(&ui_styles.nav_btn_left, lv_color_hex(UI_COLOR_BORDER));
    
    lv_style_init(&ui_styles.nav_btn_right);
    lv_style_set_bg_color(&ui_styles.nav_btn_right, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.nav_btn_right, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_btn_right, 2);
    lv_style_set_border_width(&ui_styles.nav_btn_right, 1);
    lv_style_set_border_color(&ui_styles.nav_btn_right, lv_color_hex(UI_COLOR_BORDER));
    
    lv_style_init(&ui_styles.nav_btn_center);
    lv_style_set_bg_color(&ui_styles.nav_btn_center, lv_color_hex(UI_COLOR_BG_CARD));
    lv_style_set_bg_opa(&ui_styles.nav_btn_center, LV_OPA_COVER);
    lv_style_set_radius(&ui_styles.nav_btn_center, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&ui_styles.nav_btn_center, 1);
    lv_style_set_border_color(&ui_styles.nav_btn_center, lv_color_hex(UI_COLOR_BORDER));
    
    styles_initialized = true;
}

bool ui_styles_ready(void)
{
    return styles_initialized;
}

//=============================================================================
// Common Widget Creation Functions
//=============================================================================

lv_obj_t* ui_create_panel(lv_obj_t *parent, int32_t width, int32_t height)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, width, height);
    lv_obj_add_style(panel, &ui_styles.panel, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

lv_obj_t* ui_create_card(lv_obj_t *parent, int32_t width, int32_t height)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, width, height);
    lv_obj_add_style(card, &ui_styles.card, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

lv_obj_t* ui_create_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &ui_styles.text_primary, 0);
    return label;
}

lv_obj_t* ui_create_label_secondary(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &ui_styles.text_secondary, 0);
    return label;
}

lv_obj_t* ui_create_label_small(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &ui_styles.text_small, 0);
    return label;
}

lv_obj_t* ui_create_button(lv_obj_t *parent, const char *text, int32_t width, int32_t height)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_add_style(btn, &ui_styles.card, 0);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &ui_styles.text_primary, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_center(label);
    
    return btn;
}

lv_obj_t* ui_create_gimbal(lv_obj_t *parent, lv_obj_t **dot_out, const char *label)
{
    int gimbal_size = UI_GIMBAL_SIZE;
    
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, gimbal_size + 4, gimbal_size + 14);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *gimbal = lv_obj_create(container);
    lv_obj_set_size(gimbal, gimbal_size, gimbal_size);
    lv_obj_align(gimbal, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(gimbal, &ui_styles.gimbal_bg, 0);
    lv_obj_remove_flag(gimbal, LV_OBJ_FLAG_SCROLLABLE);
    
    // Crosshairs
    lv_obj_t *line_h = lv_obj_create(gimbal);
    lv_obj_set_size(line_h, gimbal_size - 6, 1);
    lv_obj_align(line_h, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(line_h, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_bg_opa(line_h, LV_OPA_50, 0);
    lv_obj_set_style_border_width(line_h, 0, 0);
    lv_obj_remove_flag(line_h, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *line_v = lv_obj_create(gimbal);
    lv_obj_set_size(line_v, 1, gimbal_size - 6);
    lv_obj_align(line_v, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(line_v, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_bg_opa(line_v, LV_OPA_50, 0);
    lv_obj_set_style_border_width(line_v, 0, 0);
    lv_obj_remove_flag(line_v, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *dot = lv_obj_create(gimbal);
    lv_obj_set_size(dot, UI_GIMBAL_DOT_SIZE, UI_GIMBAL_DOT_SIZE);
    lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(dot, &ui_styles.gimbal_dot, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *lbl = lv_label_create(container);
    lv_label_set_text(lbl, label);
    lv_obj_add_style(lbl, &ui_styles.text_small, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    *dot_out = dot;
    return container;
}

lv_obj_t* ui_create_nav_switch_widget(lv_obj_t *parent, lv_obj_t **buttons_out, const char *label)
{
    // Create container for the nav switch display (5-way layout)
    int nav_size = 60;
    int btn_size = 16;
    
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, nav_size + 4, nav_size + 14);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *nav_bg = lv_obj_create(container);
    lv_obj_set_size(nav_bg, nav_size, nav_size);
    lv_obj_align(nav_bg, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(nav_bg, &ui_styles.gimbal_bg, 0);
    lv_obj_remove_flag(nav_bg, LV_OBJ_FLAG_SCROLLABLE);
    
    // Up button
    buttons_out[0] = lv_obj_create(nav_bg);
    lv_obj_set_size(buttons_out[0], btn_size, btn_size);
    lv_obj_align(buttons_out[0], LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_add_style(buttons_out[0], &ui_styles.btn_default, 0);
    lv_obj_remove_flag(buttons_out[0], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *up_lbl = lv_label_create(buttons_out[0]);
    lv_label_set_text(up_lbl, LV_SYMBOL_UP);
    lv_obj_center(up_lbl);
    lv_obj_set_style_text_font(up_lbl, &lv_font_montserrat_12, 0);
    
    // Down button
    buttons_out[1] = lv_obj_create(nav_bg);
    lv_obj_set_size(buttons_out[1], btn_size, btn_size);
    lv_obj_align(buttons_out[1], LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_add_style(buttons_out[1], &ui_styles.btn_default, 0);
    lv_obj_remove_flag(buttons_out[1], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *down_lbl = lv_label_create(buttons_out[1]);
    lv_label_set_text(down_lbl, LV_SYMBOL_DOWN);
    lv_obj_center(down_lbl);
    lv_obj_set_style_text_font(down_lbl, &lv_font_montserrat_12, 0);
    
    // Left button
    buttons_out[2] = lv_obj_create(nav_bg);
    lv_obj_set_size(buttons_out[2], btn_size, btn_size);
    lv_obj_align(buttons_out[2], LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_style(buttons_out[2], &ui_styles.btn_default, 0);
    lv_obj_remove_flag(buttons_out[2], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *left_lbl = lv_label_create(buttons_out[2]);
    lv_label_set_text(left_lbl, LV_SYMBOL_LEFT);
    lv_obj_center(left_lbl);
    lv_obj_set_style_text_font(left_lbl, &lv_font_montserrat_12, 0);
    
    // Right button
    buttons_out[3] = lv_obj_create(nav_bg);
    lv_obj_set_size(buttons_out[3], btn_size, btn_size);
    lv_obj_align(buttons_out[3], LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_add_style(buttons_out[3], &ui_styles.btn_default, 0);
    lv_obj_remove_flag(buttons_out[3], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *right_lbl = lv_label_create(buttons_out[3]);
    lv_label_set_text(right_lbl, LV_SYMBOL_RIGHT);
    lv_obj_center(right_lbl);
    lv_obj_set_style_text_font(right_lbl, &lv_font_montserrat_12, 0);
    
    // Center button
    buttons_out[4] = lv_obj_create(nav_bg);
    lv_obj_set_size(buttons_out[4], btn_size + 4, btn_size + 4);
    lv_obj_align(buttons_out[4], LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(buttons_out[4], &ui_styles.btn_default, 0);
    lv_obj_set_style_radius(buttons_out[4], LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(buttons_out[4], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *center_lbl = lv_label_create(buttons_out[4]);
    lv_label_set_text(center_lbl, LV_SYMBOL_OK);
    lv_obj_center(center_lbl);
    lv_obj_set_style_text_font(center_lbl, &lv_font_montserrat_12, 0);
    
    // Label
    lv_obj_t *lbl = lv_label_create(container);
    lv_label_set_text(lbl, label);
    lv_obj_add_style(lbl, &ui_styles.text_small, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    return container;
}

// Scroll event handler for snap behavior
static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_SCROLL_END) {
        lv_point_t scroll_end;
        lv_obj_get_scroll_end(cont, &scroll_end);
        
        int32_t page_width = UI_SCREEN_WIDTH;
        int32_t scroll_x = scroll_end.x;
        uint8_t target_page = (scroll_x + page_width / 2) / page_width;
        
        lv_obj_scroll_to_x(cont, target_page * page_width, LV_ANIM_ON);
    }
}

lv_obj_t* ui_create_scroll_container(lv_obj_t *parent, int32_t width, int32_t height, uint8_t num_pages)
{
    lv_obj_t *scroll_cont = lv_obj_create(parent);
    lv_obj_set_size(scroll_cont, width, height);
    lv_obj_add_style(scroll_cont, &ui_styles.panel, 0);
    lv_obj_set_style_border_width(scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(scroll_cont, 0, 0);
    
    // Enable horizontal scrolling with snap
    lv_obj_add_flag(scroll_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(scroll_cont, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(scroll_cont, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(scroll_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(scroll_cont, scroll_event_cb, LV_EVENT_SCROLL_END, NULL);
    
    return scroll_cont;
}

lv_obj_t* ui_create_title_bar(lv_obj_t *parent, const char *title)
{
    lv_obj_t *bar = ui_create_panel(parent, lv_pct(100), 24);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t *label = ui_create_label(bar, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_center(label);
    
    return bar;
}
