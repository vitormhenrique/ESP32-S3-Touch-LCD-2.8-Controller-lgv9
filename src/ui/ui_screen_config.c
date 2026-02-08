/**
 * @file ui_screen_config.c
 * @brief Config Screen Implementation
 * 
 * Previously "Settings" - renamed to "Config"
 */

#include "ui_screen_config.h"
#include <stdio.h>

//=============================================================================
// Screen Objects
//=============================================================================

lv_obj_t *ui_ConfigScreen = NULL;

static lv_obj_t *ui_ConfigTitle = NULL;
static lv_obj_t *ui_BtnWiFi = NULL;
static lv_obj_t *ui_BtnCalibration = NULL;
static lv_obj_t *ui_BtnDisplay = NULL;
static lv_obj_t *ui_BtnAbout = NULL;

static lv_obj_t *ui_WiFiStatusLabel = NULL;
static lv_obj_t *ui_BrightnessSlider = NULL;
static lv_obj_t *ui_VersionLabel = NULL;

// Callbacks
static config_callback_t wifi_callback = NULL;
static config_callback_t calibration_callback = NULL;
static config_callback_t display_callback = NULL;
static config_callback_t about_callback = NULL;

//=============================================================================
// Event Handlers
//=============================================================================

static void wifi_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (wifi_callback) wifi_callback();
    }
}

static void calibration_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (calibration_callback) calibration_callback();
    }
}

static void display_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (display_callback) display_callback();
    }
}

static void about_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (about_callback) about_callback();
    }
}

static void brightness_slider_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        // Handle brightness change - this will be implemented in main
    }
}

//=============================================================================
// Create Menu Button
//=============================================================================

static lv_obj_t *create_menu_button(lv_obj_t *parent, const char *text, const char *icon, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, UI_SCREEN_WIDTH - 24, 50);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_ACCENT_BLUE), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_pad_all(btn, 10, 0);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    
    // Icon (using text symbol as placeholder)
    lv_obj_t *icon_label = lv_label_create(btn);
    lv_label_set_text(icon_label, icon);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_16, 0);
    lv_obj_align(icon_label, LV_ALIGN_LEFT_MID, 0, 0);
    
    // Text
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 30, 0);
    
    // Arrow indicator
    lv_obj_t *arrow = lv_label_create(btn);
    lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(arrow, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);
    
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    
    return btn;
}

//=============================================================================
// Public Functions
//=============================================================================

void ui_config_screen_create(lv_obj_t *parent)
{
    ui_ConfigScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_ConfigScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_ConfigScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_ConfigScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_ConfigScreen, 0, 0);
    lv_obj_add_flag(ui_ConfigScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_ConfigScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Title - "Config" (renamed from Settings)
    ui_ConfigTitle = ui_create_label(ui_ConfigScreen, "Config");
    lv_obj_set_style_text_font(ui_ConfigTitle, &lv_font_montserrat_16, 0);
    lv_obj_align(ui_ConfigTitle, LV_ALIGN_TOP_MID, 0, 4);
    
    // Content container
    lv_obj_t *content = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_size(content, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT - 30);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    // WiFi Button
    ui_BtnWiFi = create_menu_button(content, "WiFi", LV_SYMBOL_WIFI, wifi_btn_event_cb);
    
    // WiFi status label (sub-label)
    ui_WiFiStatusLabel = lv_label_create(ui_BtnWiFi);
    lv_label_set_text(ui_WiFiStatusLabel, "Not connected");
    lv_obj_set_style_text_color(ui_WiFiStatusLabel, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_font(ui_WiFiStatusLabel, &lv_font_montserrat_10, 0);
    lv_obj_align(ui_WiFiStatusLabel, LV_ALIGN_BOTTOM_LEFT, 30, 6);
    
    // Calibration Button
    ui_BtnCalibration = create_menu_button(content, "Calibration", LV_SYMBOL_SETTINGS, calibration_btn_event_cb);
    
    // Display Button
    ui_BtnDisplay = create_menu_button(content, "Display", LV_SYMBOL_IMAGE, display_btn_event_cb);
    
    // About Button
    ui_BtnAbout = create_menu_button(content, "About", LV_SYMBOL_WARNING, about_btn_event_cb);
    
    // Version label at bottom
    ui_VersionLabel = ui_create_label_secondary(content, "v1.0.0");
    lv_obj_set_style_text_align(ui_VersionLabel, LV_TEXT_ALIGN_CENTER, 0);
}

void ui_config_screen_show(bool show)
{
    if (ui_ConfigScreen) {
        if (show) {
            lv_obj_remove_flag(ui_ConfigScreen, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_ConfigScreen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_config_set_wifi_status(bool connected, const char *ssid, const char *ip)
{
    if (ui_WiFiStatusLabel) {
        if (connected && ssid && ip) {
            char buf[48];
            snprintf(buf, sizeof(buf), "%s - %s", ssid, ip);
            lv_label_set_text(ui_WiFiStatusLabel, buf);
            lv_obj_set_style_text_color(ui_WiFiStatusLabel, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        } else {
            lv_label_set_text(ui_WiFiStatusLabel, "Not connected");
            lv_obj_set_style_text_color(ui_WiFiStatusLabel, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
        }
    }
}

void ui_config_set_brightness(uint8_t brightness)
{
    if (ui_BrightnessSlider) {
        lv_slider_set_value(ui_BrightnessSlider, brightness, LV_ANIM_OFF);
    }
}

void ui_config_set_version(const char *version)
{
    if (ui_VersionLabel) {
        lv_label_set_text(ui_VersionLabel, version);
    }
}

void ui_config_set_wifi_callback(config_callback_t cb)
{
    wifi_callback = cb;
}

void ui_config_set_calibration_callback(config_callback_t cb)
{
    calibration_callback = cb;
}

void ui_config_set_display_callback(config_callback_t cb)
{
    display_callback = cb;
}

void ui_config_set_about_callback(config_callback_t cb)
{
    about_callback = cb;
}
