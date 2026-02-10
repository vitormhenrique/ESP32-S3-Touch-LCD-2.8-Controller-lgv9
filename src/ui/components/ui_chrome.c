#include "ui_chrome.h"
#include "../ui_helpers.h"
#include <stdio.h>

lv_obj_t *ui_HeaderPanel = NULL;
lv_obj_t *ui_TitleLabel = NULL;
lv_obj_t *ui_StatusIcon = NULL;
lv_obj_t *ui_BatteryIcon = NULL;
lv_obj_t *ui_BatteryLabel = NULL;

lv_obj_t *ui_NavPanel = NULL;
lv_obj_t *ui_NavBtnInput = NULL;
lv_obj_t *ui_NavBtnTelemetry = NULL;
lv_obj_t *ui_NavBtnSettings = NULL;
static lv_obj_t *ui_NavBtnLabels[3] = {NULL};

// Forward declaration of logic to switch screen - passed as callback?
// Or we just expose the buttons and let the main controller attach events?
// ui_custom.c: nav_btn_event_cb calls ui_show_screen.
// Including ui.h here might create circular dependency if ui.h includes ui_chrome.h.
// Let's declare external void ui_show_screen(int screen);

extern void ui_show_screen(int screen);

static void nav_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    
    // 0=Input, 1=Telemetry, 2=Settings
    if (btn == ui_NavBtnInput) {
        ui_show_screen(0);
    } else if (btn == ui_NavBtnTelemetry) {
        ui_show_screen(1);
    } else if (btn == ui_NavBtnSettings) {
        ui_show_screen(2);
    }
}

void ui_create_header(lv_obj_t *parent)
{
    ui_HeaderPanel = lv_obj_create(parent);
    lv_obj_set_size(ui_HeaderPanel, lv_pct(100), UI_HEADER_HEIGHT);
    lv_obj_align(ui_HeaderPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_HeaderPanel, &style_panel, 0);
    lv_obj_remove_flag(ui_HeaderPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(ui_HeaderPanel, LV_BORDER_SIDE_BOTTOM, 0);
    
    ui_TitleLabel = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_TitleLabel, "RC Control");
    lv_obj_add_style(ui_TitleLabel, &style_text_primary, 0);
    lv_obj_set_style_text_font(ui_TitleLabel, &lv_font_montserrat_10, 0);
    lv_obj_align(ui_TitleLabel, LV_ALIGN_LEFT_MID, 2, 0);
    
    ui_StatusIcon = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_StatusIcon, ICON_WIFI_OFF);
    lv_obj_set_style_text_color(ui_StatusIcon, lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
    lv_obj_align(ui_StatusIcon, LV_ALIGN_RIGHT_MID, -62, 0);
    
    ui_BatteryIcon = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_BatteryIcon, ICON_BATTERY_FULL);
    lv_obj_set_style_text_color(ui_BatteryIcon, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_align(ui_BatteryIcon, LV_ALIGN_RIGHT_MID, -34, 0);
    
    ui_BatteryLabel = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_BatteryLabel, "100%");
    lv_obj_add_style(ui_BatteryLabel, &style_text_secondary, 0);
    lv_obj_set_style_text_font(ui_BatteryLabel, &lv_font_montserrat_10, 0);
    lv_obj_align(ui_BatteryLabel, LV_ALIGN_RIGHT_MID, -2, 0);
}

void ui_create_nav_bar(lv_obj_t *parent)
{
    ui_NavPanel = lv_obj_create(parent);
    lv_obj_set_size(ui_NavPanel, lv_pct(100), UI_NAV_HEIGHT);
    lv_obj_align(ui_NavPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ui_NavPanel, &style_panel, 0);
    lv_obj_remove_flag(ui_NavPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(ui_NavPanel, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_flex_flow(ui_NavPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_NavPanel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(ui_NavPanel, 4, 0);
    lv_obj_set_style_pad_bottom(ui_NavPanel, 6, 0);  // Extra padding at bottom
    lv_obj_set_style_pad_hor(ui_NavPanel, 8, 0);
    
    // Icons with labels - improved touch targets
    const char *nav_icons[] = {ICON_INPUT, ICON_TELEMETRY, ICON_SETTINGS};
    const char *nav_labels[] = {"Input", "Data", "Settings"};
    lv_obj_t **nav_btns[] = {&ui_NavBtnInput, &ui_NavBtnTelemetry, &ui_NavBtnSettings};
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(ui_NavPanel);
        lv_obj_set_size(btn, 90, 28);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        
        // Horizontal layout: icon + label
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(btn, 4, 0);
        
        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, nav_icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, nav_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
        
        ui_NavBtnLabels[i] = icon;
        *nav_btns[i] = btn;
    }
}

void ui_update_nav_buttons(int active_screen_index)
{
    lv_obj_t *btns[] = {ui_NavBtnInput, ui_NavBtnTelemetry, ui_NavBtnSettings};
    
    for (int i = 0; i < 3; i++) {
        if (btns[i]) {
            lv_obj_remove_style(btns[i], &style_nav_active, 0);
            lv_obj_remove_style(btns[i], &style_nav_inactive, 0);
            
            if (i == active_screen_index) {
                lv_obj_add_style(btns[i], &style_nav_active, 0);
            } else {
                lv_obj_add_style(btns[i], &style_nav_inactive, 0);
            }
        }
    }
}

void ui_set_battery(uint8_t percent, int state)
{
    if (!ui_BatteryLabel || !ui_BatteryIcon) return;
    
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(ui_BatteryLabel, buf);
    
    const char *icon;
    lv_color_t color;
    
    // 0=Unknown, 1=Discharging, 2=Charging, 3=Full
    if (state == 2) { // CHARGING
        icon = ICON_CHARGE;
        color = lv_color_hex(UI_COLOR_ACCENT_CYAN);
    } else if (percent > 80) {
        icon = ICON_BATTERY_FULL;
        color = lv_color_hex(UI_COLOR_ACCENT_GREEN);
    } else if (percent > 60) {
        icon = ICON_BATTERY_3;
        color = lv_color_hex(UI_COLOR_ACCENT_GREEN);
    } else if (percent > 40) {
        icon = ICON_BATTERY_2;
        color = lv_color_hex(UI_COLOR_ACCENT_ORANGE);
    } else if (percent > 20) {
        icon = ICON_BATTERY_1;
        color = lv_color_hex(UI_COLOR_ACCENT_ORANGE);
    } else {
        icon = ICON_BATTERY_EMPTY;
        color = lv_color_hex(UI_COLOR_ACCENT_RED);
    }
    
    lv_label_set_text(ui_BatteryIcon, icon);
    lv_obj_set_style_text_color(ui_BatteryIcon, color, 0);
    lv_obj_set_style_text_color(ui_BatteryLabel, color, 0);
}

void ui_set_status(bool connected)
{
    if (!ui_StatusIcon) return;
    lv_label_set_text(ui_StatusIcon, connected ? ICON_WIFI_ON : ICON_WIFI_OFF);
    lv_obj_set_style_text_color(ui_StatusIcon, 
        lv_color_hex(connected ? UI_COLOR_ACCENT_GREEN : UI_COLOR_ACCENT_ORANGE), 0);
}
