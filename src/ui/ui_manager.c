/**
 * @file ui_manager.c
 * @brief Main UI Manager Implementation
 * 
 * Coordinates all screens and navigation
 */

#include "ui_manager.h"
#include "ui_screen_input.h"
#include "ui_screen_telemetry.h"
#include "ui_screen_config.h"
#include "ui_screen_calibration.h"
#include "ui_screen_robot.h"
#include <stdio.h>

//=============================================================================
// Global Variables (declared extern in header)
//=============================================================================

lv_obj_t *ui_MainScreen = NULL;

//=============================================================================
// Header Objects (declared extern in header)
//=============================================================================

lv_obj_t *ui_HeaderPanel = NULL;
lv_obj_t *ui_TitleLabel = NULL;
lv_obj_t *ui_StatusIcon = NULL;
lv_obj_t *ui_BatteryIcon = NULL;
lv_obj_t *ui_BatteryLabel = NULL;
lv_obj_t *ui_ChargeIcon = NULL;

//=============================================================================
// Static Variables
//=============================================================================

static lv_obj_t *ui_TabView = NULL;
static lv_obj_t *ui_ContentContainer = NULL;

static UIScreen_t current_screen = UI_SCREEN_INPUT;
static ui_screen_change_cb_t screen_change_callback = NULL;

//=============================================================================
// Tab Bar Height
//=============================================================================

#define UI_TAB_BAR_HEIGHT 36

//=============================================================================
// Tab Event Handler
//=============================================================================

static void tab_change_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        uint16_t tab_id = lv_tabview_get_tab_active(ui_TabView);
        
        if (tab_id < UI_SCREEN_COUNT) {
            ui_show_screen((UIScreen_t)tab_id);
        }
    }
}

//=============================================================================
// Create Header
//=============================================================================

static void create_header(void)
{
    // Header panel at top of screen
    ui_HeaderPanel = lv_obj_create(ui_MainScreen);
    lv_obj_set_size(ui_HeaderPanel, UI_SCREEN_WIDTH, UI_HEADER_HEIGHT);
    lv_obj_align(ui_HeaderPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_HeaderPanel, &ui_styles.panel, 0);
    lv_obj_remove_flag(ui_HeaderPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(ui_HeaderPanel, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_left(ui_HeaderPanel, 6, 0);
    lv_obj_set_style_pad_right(ui_HeaderPanel, 6, 0);
    
    // Title label on the left
    ui_TitleLabel = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_TitleLabel, "RC Control");
    lv_obj_add_style(ui_TitleLabel, &ui_styles.text_primary, 0);
    lv_obj_set_style_text_font(ui_TitleLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(ui_TitleLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    // WiFi status icon
    ui_StatusIcon = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_StatusIcon, ICON_WIFI_OFF);
    lv_obj_set_style_text_color(ui_StatusIcon, lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
    lv_obj_align(ui_StatusIcon, LV_ALIGN_RIGHT_MID, -70, 0);
    
    // Charging indicator (initially hidden)
    ui_ChargeIcon = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_ChargeIcon, ICON_CHARGE);
    lv_obj_set_style_text_color(ui_ChargeIcon, lv_color_hex(UI_COLOR_ACCENT_CYAN), 0);
    lv_obj_align(ui_ChargeIcon, LV_ALIGN_RIGHT_MID, -52, 0);
    lv_obj_add_flag(ui_ChargeIcon, LV_OBJ_FLAG_HIDDEN);
    
    // Battery icon
    ui_BatteryIcon = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_BatteryIcon, ICON_BATTERY_FULL);
    lv_obj_set_style_text_color(ui_BatteryIcon, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_align(ui_BatteryIcon, LV_ALIGN_RIGHT_MID, -34, 0);
    
    // Battery percentage label
    ui_BatteryLabel = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_BatteryLabel, "100%");
    lv_obj_add_style(ui_BatteryLabel, &ui_styles.text_secondary, 0);
    lv_obj_set_style_text_font(ui_BatteryLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(ui_BatteryLabel, LV_ALIGN_RIGHT_MID, 0, 0);
}

//=============================================================================
// Public Functions
//=============================================================================

void ui_init(void)
{
    // Initialize global styles
    ui_styles_init();
    
    // Create main screen
    ui_MainScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_MainScreen, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_remove_flag(ui_MainScreen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create header at the top
    create_header();
    
    // Create tabview for navigation (below header)
    ui_TabView = lv_tabview_create(ui_MainScreen);
    lv_obj_set_size(ui_TabView, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT - UI_HEADER_HEIGHT);
    lv_obj_set_pos(ui_TabView, 0, UI_HEADER_HEIGHT);
    lv_tabview_set_tab_bar_position(ui_TabView, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(ui_TabView, UI_TAB_BAR_HEIGHT);
    
    // Style the tabview
    lv_obj_set_style_bg_color(ui_TabView, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_border_width(ui_TabView, 0, 0);
    
    // Style the tab bar
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(ui_TabView);
    lv_obj_set_style_bg_color(tab_bar, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_border_width(tab_bar, 0, 0);
    lv_obj_set_style_pad_all(tab_bar, 0, 0);
    
    // Create tabs
    lv_obj_t *tab_input = lv_tabview_add_tab(ui_TabView, LV_SYMBOL_EDIT);
    lv_obj_t *tab_telemetry = lv_tabview_add_tab(ui_TabView, LV_SYMBOL_EYE_OPEN);
    lv_obj_t *tab_config = lv_tabview_add_tab(ui_TabView, LV_SYMBOL_SETTINGS);
    
    // Style all tabs
    lv_obj_t *tabs[] = {tab_input, tab_telemetry, tab_config};
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_opa(tabs[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(tabs[i], 0, 0);
        lv_obj_set_style_pad_all(tabs[i], 0, 0);
        lv_obj_remove_flag(tabs[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    
    // Style tab buttons
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
        if (btn) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_BG_PANEL), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_ACCENT_BLUE), LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
            lv_obj_set_style_text_color(btn, lv_color_hex(UI_COLOR_TEXT_PRIMARY), LV_STATE_CHECKED);
        }
    }
    
    // Add tab change event
    lv_obj_add_event_cb(ui_TabView, tab_change_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Create screen content
    ui_input_screen_create(tab_input);
    ui_telemetry_screen_create(tab_telemetry);
    ui_config_screen_create(tab_config);
    
    // Create overlay screens (not in tabs)
    ui_touch_calib_screen_create(ui_MainScreen);
    ui_gimbal_calib_screen_create(ui_MainScreen);
    ui_robot_screen_create(ui_MainScreen);
    
    // Show input screen by default
    ui_show_screen(UI_SCREEN_INPUT);
    
    // Load the main screen
    lv_screen_load(ui_MainScreen);
}

void ui_update(void)
{
    // Called periodically to update UI elements
    // Most updates happen through the set functions
}

void ui_show_screen(UIScreen_t screen)
{
    // Hide all main screens
    ui_input_screen_show(false);
    ui_telemetry_screen_show(false);
    ui_config_screen_show(false);
    
    // Show requested screen
    switch (screen) {
        case UI_SCREEN_INPUT:
            ui_input_screen_show(true);
            lv_tabview_set_active(ui_TabView, 0, LV_ANIM_OFF);
            break;
            
        case UI_SCREEN_TELEMETRY:
            ui_telemetry_screen_show(true);
            lv_tabview_set_active(ui_TabView, 1, LV_ANIM_OFF);
            break;
            
        case UI_SCREEN_CONFIG:
            ui_config_screen_show(true);
            lv_tabview_set_active(ui_TabView, 2, LV_ANIM_OFF);
            break;
            
        default:
            break;
    }
    
    current_screen = screen;
    
    if (screen_change_callback) {
        screen_change_callback(screen);
    }
}

UIScreen_t ui_get_current_screen(void)
{
    return current_screen;
}

void ui_set_screen_change_callback(ui_screen_change_cb_t cb)
{
    screen_change_callback = cb;
}

UI_Styles_t *ui_get_styles(void)
{
    return &ui_styles;  // Returns the global ui_styles from ui_common.c
}

//=============================================================================
// Config Screen Callbacks Setup
//=============================================================================

static void on_calibration_pressed(void)
{
    // Show gimbal calibration screen
    ui_gimbal_calib_start(0);
}

static void on_touch_calib_pressed(void)
{
    // Show touch calibration screen
    ui_touch_calib_start();
}

void ui_setup_callbacks(void)
{
    // Set up config screen callbacks
    ui_config_set_calibration_callback(on_calibration_pressed);
    
    // Robot selection callback
    ui_robot_set_callback(ui_on_robot_selected);
}

//=============================================================================
// External Callbacks (to be overridden by user)
//=============================================================================

__attribute__((weak)) void ui_on_robot_selected(RobotType_t robot_type)
{
    // Default implementation - update telemetry screen
    ui_telemetry_set_robot_type((uint8_t)robot_type);
}

__attribute__((weak)) void ui_on_gimbal_calibrated(uint8_t gimbal_id, int16_t min_val, int16_t center_val, int16_t max_val)
{
    // Default implementation - do nothing
}

__attribute__((weak)) void ui_on_touch_calibrated(int16_t x1, int16_t y1, int16_t x2, int16_t y2, 
                                                   int16_t x3, int16_t y3, int16_t x4, int16_t y4)
{
    // Default implementation - do nothing
}

//=============================================================================
// Header Update Functions
//=============================================================================

void ui_set_battery(uint8_t percent, BatteryState_t state)
{
    if (!ui_BatteryLabel || !ui_BatteryIcon) return;
    
    // Update percentage label
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(ui_BatteryLabel, buf);
    
    // Determine icon and color based on state and percentage
    const char *icon;
    lv_color_t color;
    
    // Show/hide charging icon
    if (state == BATTERY_STATE_CHARGING) {
        if (ui_ChargeIcon) {
            lv_obj_remove_flag(ui_ChargeIcon, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (ui_ChargeIcon) {
            lv_obj_add_flag(ui_ChargeIcon, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // Select icon based on percentage
    if (percent > 80) {
        icon = ICON_BATTERY_FULL;
    } else if (percent > 60) {
        icon = ICON_BATTERY_3;
    } else if (percent > 40) {
        icon = ICON_BATTERY_2;
    } else if (percent > 20) {
        icon = ICON_BATTERY_1;
    } else {
        icon = ICON_BATTERY_EMPTY;
    }
    
    // Select color based on percentage (color coding from spec)
    if (percent > 50) {
        color = lv_color_hex(UI_COLOR_ACCENT_GREEN);   // Green: Above 50%
    } else if (percent >= 10) {
        color = lv_color_hex(UI_COLOR_ACCENT_ORANGE);  // Orange: 10-50%
    } else {
        color = lv_color_hex(UI_COLOR_ACCENT_RED);     // Red: Below 10%
    }
    
    lv_label_set_text(ui_BatteryIcon, icon);
    lv_obj_set_style_text_color(ui_BatteryIcon, color, 0);
    lv_obj_set_style_text_color(ui_BatteryLabel, color, 0);
}

void ui_set_wifi_status(bool connected)
{
    if (!ui_StatusIcon) return;
    
    if (connected) {
        lv_label_set_text(ui_StatusIcon, ICON_WIFI_ON);
        lv_obj_set_style_text_color(ui_StatusIcon, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    } else {
        lv_label_set_text(ui_StatusIcon, ICON_WIFI_OFF);
        lv_obj_set_style_text_color(ui_StatusIcon, lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
    }
}
