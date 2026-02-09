/**
 * @file ui_custom.c
 * @brief Custom RC Controller UI Implementation for LVGL 9
 */

#include "ui_custom.h"
#include <stdio.h>

//=============================================================================
// UI Object Definitions
//=============================================================================

// Main screen
lv_obj_t *ui_MainScreen = NULL;

// Header panel
lv_obj_t *ui_HeaderPanel = NULL;
lv_obj_t *ui_TitleLabel = NULL;
lv_obj_t *ui_StatusIcon = NULL;
lv_obj_t *ui_BatteryIcon = NULL;
lv_obj_t *ui_BatteryLabel = NULL;

// Navigation bar
lv_obj_t *ui_NavPanel = NULL;
lv_obj_t *ui_NavBtnInput = NULL;
lv_obj_t *ui_NavBtnTelemetry = NULL;
lv_obj_t *ui_NavBtnSettings = NULL;
static lv_obj_t *ui_NavBtnLabels[3] = {NULL};

// Content area
lv_obj_t *ui_ContentArea = NULL;

// Input screen objects
lv_obj_t *ui_InputScreen = NULL;
lv_obj_t *ui_GimbalPanel = NULL;
lv_obj_t *ui_GimbalLeft = NULL;
lv_obj_t *ui_GimbalLeftDot = NULL;
lv_obj_t *ui_GimbalRight = NULL;
lv_obj_t *ui_GimbalRightDot = NULL;
lv_obj_t *ui_InputControlsPanel = NULL;
lv_obj_t *ui_InputPage1 = NULL;
lv_obj_t *ui_InputPage2 = NULL;
lv_obj_t *ui_Buttons[4] = {NULL};
lv_obj_t *ui_BtnLabels[4] = {NULL};
lv_obj_t *ui_Switches[6] = {NULL};
lv_obj_t *ui_SwitchLabels[6] = {NULL};
lv_obj_t *ui_Toggle3Panels[2] = {NULL};
lv_obj_t *ui_Toggle3Indicators[2] = {NULL};
lv_obj_t *ui_Toggle3Labels[2] = {NULL};
lv_obj_t *ui_PotBars[2] = {NULL};
lv_obj_t *ui_PotLabels[2] = {NULL};
lv_obj_t *ui_PotValues[2] = {NULL};
lv_obj_t *ui_EncPanels[2] = {NULL};
lv_obj_t *ui_EncLabels[2] = {NULL};
lv_obj_t *ui_EncValues[2] = {NULL};

// Telemetry screen objects
lv_obj_t *ui_TelemetryScreen = NULL;
lv_obj_t *ui_TelemetryPanel1 = NULL;
lv_obj_t *ui_TelemetryPanel2 = NULL;
static lv_obj_t *ui_TelemetryLabels[8] = {NULL};
static lv_obj_t *ui_TelemetryValues[8] = {NULL};

// Settings screen objects
lv_obj_t *ui_SettingsScreen = NULL;

// Current screen tracking
static ScreenType_t current_screen = SCREEN_INPUT;

//=============================================================================
// Style Definitions
//=============================================================================

static lv_style_t style_panel;
static lv_style_t style_card;
static lv_style_t style_gimbal_bg;
static lv_style_t style_gimbal_dot;
static lv_style_t style_switch_off;
static lv_style_t style_switch_on;
static lv_style_t style_btn_default;
static lv_style_t style_btn_pressed;
static lv_style_t style_toggle3_bg;
static lv_style_t style_toggle3_indicator;
static lv_style_t style_bar_bg;
static lv_style_t style_bar_indicator;
static lv_style_t style_text_primary;
static lv_style_t style_text_secondary;
static lv_style_t style_text_small;
static lv_style_t style_nav_active;
static lv_style_t style_nav_inactive;

static bool styles_initialized = false;

//=============================================================================
// Icon Symbols
//=============================================================================

#define ICON_WIFI_ON      LV_SYMBOL_WIFI
#define ICON_WIFI_OFF     LV_SYMBOL_WARNING
#define ICON_BATTERY_FULL LV_SYMBOL_BATTERY_FULL
#define ICON_BATTERY_3    LV_SYMBOL_BATTERY_3
#define ICON_BATTERY_2    LV_SYMBOL_BATTERY_2
#define ICON_BATTERY_1    LV_SYMBOL_BATTERY_1
#define ICON_BATTERY_EMPTY LV_SYMBOL_BATTERY_EMPTY
#define ICON_CHARGE       LV_SYMBOL_CHARGE
#define ICON_INPUT        LV_SYMBOL_EDIT
#define ICON_TELEMETRY    LV_SYMBOL_DOWNLOAD
#define ICON_SETTINGS     LV_SYMBOL_SETTINGS

// Layout constants
#define INPUT_PANEL_HEIGHT  60
#define GIMBAL_PANEL_HEIGHT (UI_CONTENT_HEIGHT - INPUT_PANEL_HEIGHT)

//=============================================================================
// Style Initialization
//=============================================================================

static void init_styles(void)
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
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_12);
    
    // Nav active
    lv_style_init(&style_nav_active);
    lv_style_set_bg_color(&style_nav_active, lv_color_hex(UI_COLOR_NAV_ACTIVE));
    lv_style_set_bg_opa(&style_nav_active, LV_OPA_COVER);
    lv_style_set_radius(&style_nav_active, 4);
    lv_style_set_border_width(&style_nav_active, 0);
    lv_style_set_text_color(&style_nav_active, lv_color_hex(UI_COLOR_TEXT_PRIMARY));
    
    // Nav inactive
    lv_style_init(&style_nav_inactive);
    lv_style_set_bg_color(&style_nav_inactive, lv_color_hex(UI_COLOR_NAV_INACTIVE));
    lv_style_set_bg_opa(&style_nav_inactive, LV_OPA_COVER);
    lv_style_set_radius(&style_nav_inactive, 4);
    lv_style_set_border_color(&style_nav_inactive, lv_color_hex(UI_COLOR_BORDER));
    lv_style_set_border_width(&style_nav_inactive, 1);
    lv_style_set_text_color(&style_nav_inactive, lv_color_hex(UI_COLOR_TEXT_SECONDARY));
    
    styles_initialized = true;
}

//=============================================================================
// Navigation Event Handler
//=============================================================================

static void nav_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    
    if (btn == ui_NavBtnInput) {
        ui_show_screen(SCREEN_INPUT);
    } else if (btn == ui_NavBtnTelemetry) {
        ui_show_screen(SCREEN_TELEMETRY);
    } else if (btn == ui_NavBtnSettings) {
        ui_show_screen(SCREEN_SETTINGS);
    }
}

static void update_nav_buttons(ScreenType_t active)
{
    lv_obj_t *btns[] = {ui_NavBtnInput, ui_NavBtnTelemetry, ui_NavBtnSettings};
    
    for (int i = 0; i < 3; i++) {
        if (btns[i]) {
            lv_obj_remove_style(btns[i], &style_nav_active, 0);
            lv_obj_remove_style(btns[i], &style_nav_inactive, 0);
            
            if (i == (int)active) {
                lv_obj_add_style(btns[i], &style_nav_active, 0);
            } else {
                lv_obj_add_style(btns[i], &style_nav_inactive, 0);
            }
        }
    }
}

//=============================================================================
// Scroll event handler for snap behavior
//=============================================================================

static void input_panel_scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_SCROLL_END) {
        lv_point_t scroll_end;
        lv_obj_get_scroll_end(cont, &scroll_end);
        
        int32_t page_width = UI_SCREEN_WIDTH;
        int32_t scroll_x = scroll_end.x;
        uint8_t target_page = (scroll_x > page_width / 2) ? 1 : 0;
        
        lv_obj_scroll_to_x(cont, target_page * page_width, LV_ANIM_ON);
    }
}

//=============================================================================
// Create Header
//=============================================================================

static void create_header(void)
{
    ui_HeaderPanel = lv_obj_create(ui_MainScreen);
    lv_obj_set_size(ui_HeaderPanel, lv_pct(100), UI_HEADER_HEIGHT);
    lv_obj_align(ui_HeaderPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_HeaderPanel, &style_panel, 0);
    lv_obj_remove_flag(ui_HeaderPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(ui_HeaderPanel, LV_BORDER_SIDE_BOTTOM, 0);
    
    ui_TitleLabel = lv_label_create(ui_HeaderPanel);
    lv_label_set_text(ui_TitleLabel, "RC Control");
    lv_obj_add_style(ui_TitleLabel, &style_text_primary, 0);
    lv_obj_set_style_text_font(ui_TitleLabel, &lv_font_montserrat_12, 0);
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
    lv_obj_set_style_text_font(ui_BatteryLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(ui_BatteryLabel, LV_ALIGN_RIGHT_MID, -2, 0);
}

//=============================================================================
// Create Navigation Bar
//=============================================================================

static void create_nav_bar(void)
{
    ui_NavPanel = lv_obj_create(ui_MainScreen);
    lv_obj_set_size(ui_NavPanel, lv_pct(100), UI_NAV_HEIGHT);
    lv_obj_align(ui_NavPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ui_NavPanel, &style_panel, 0);
    lv_obj_remove_flag(ui_NavPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(ui_NavPanel, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_flex_flow(ui_NavPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_NavPanel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ui_NavPanel, 3, 0);
    
    const char *nav_icons[] = {ICON_INPUT, ICON_TELEMETRY, ICON_SETTINGS};
    const char *nav_labels[] = {"Input", "Telem", "Setup"};
    lv_obj_t **nav_btns[] = {&ui_NavBtnInput, &ui_NavBtnTelemetry, &ui_NavBtnSettings};
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(ui_NavPanel);
        lv_obj_set_size(btn, 100, 24);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, NULL);
        
        lv_obj_t *lbl = lv_label_create(btn);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %s", nav_icons[i], nav_labels[i]);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
        
        ui_NavBtnLabels[i] = lbl;
        *nav_btns[i] = btn;
    }
    
    update_nav_buttons(SCREEN_INPUT);
}

//=============================================================================
// Create Gimbal Widget
//=============================================================================

static lv_obj_t* create_gimbal_widget(lv_obj_t *parent, lv_obj_t **dot_out, const char *label)
{
    int gimbal_size = 70;  // Larger gimbals
    
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, gimbal_size + 4, gimbal_size + 14);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *gimbal = lv_obj_create(container);
    lv_obj_set_size(gimbal, gimbal_size, gimbal_size);
    lv_obj_align(gimbal, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(gimbal, &style_gimbal_bg, 0);
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
    lv_obj_set_size(dot, 14, 14);
    lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(dot, &style_gimbal_dot, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *lbl = lv_label_create(container);
    lv_label_set_text(lbl, label);
    lv_obj_add_style(lbl, &style_text_small, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    *dot_out = dot;
    return gimbal;
}

//=============================================================================
// Create Page 1 Content: Buttons + Toggles + Switches(3)
//=============================================================================

static void create_page1_content(lv_obj_t *page)
{
    // Single row with all controls spread across
    lv_obj_t *row = lv_obj_create(page);
    lv_obj_set_size(row, UI_SCREEN_WIDTH - 8, INPUT_PANEL_HEIGHT - 8);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 4 Buttons
    const char *btn_names[] = {"B1", "B2", "B3", "B4"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn_cont = lv_obj_create(row);
        lv_obj_set_size(btn_cont, 34, INPUT_PANEL_HEIGHT - 12);
        lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_cont, 0, 0);
        lv_obj_set_style_pad_all(btn_cont, 0, 0);
        lv_obj_remove_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        ui_Buttons[i] = lv_obj_create(btn_cont);
        lv_obj_set_size(ui_Buttons[i], 30, 28);
        lv_obj_align(ui_Buttons[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_style(ui_Buttons[i], &style_btn_default, 0);
        lv_obj_remove_flag(ui_Buttons[i], LV_OBJ_FLAG_SCROLLABLE);
        
        ui_BtnLabels[i] = lv_label_create(btn_cont);
        lv_label_set_text(ui_BtnLabels[i], btn_names[i]);
        lv_obj_add_style(ui_BtnLabels[i], &style_text_small, 0);
        lv_obj_align(ui_BtnLabels[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    
    // 2 Three-position toggles
    const char *toggle_names[] = {"T1", "T2"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *toggle_cont = lv_obj_create(row);
        lv_obj_set_size(toggle_cont, 24, INPUT_PANEL_HEIGHT - 12);
        lv_obj_set_style_bg_opa(toggle_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(toggle_cont, 0, 0);
        lv_obj_set_style_pad_all(toggle_cont, 0, 0);
        lv_obj_remove_flag(toggle_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        ui_Toggle3Panels[i] = lv_obj_create(toggle_cont);
        lv_obj_set_size(ui_Toggle3Panels[i], 18, 28);
        lv_obj_align(ui_Toggle3Panels[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_style(ui_Toggle3Panels[i], &style_toggle3_bg, 0);
        lv_obj_remove_flag(ui_Toggle3Panels[i], LV_OBJ_FLAG_SCROLLABLE);
        
        ui_Toggle3Indicators[i] = lv_obj_create(ui_Toggle3Panels[i]);
        lv_obj_set_size(ui_Toggle3Indicators[i], 12, 6);
        lv_obj_align(ui_Toggle3Indicators[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_style(ui_Toggle3Indicators[i], &style_toggle3_indicator, 0);
        lv_obj_remove_flag(ui_Toggle3Indicators[i], LV_OBJ_FLAG_SCROLLABLE);
        
        ui_Toggle3Labels[i] = lv_label_create(toggle_cont);
        lv_label_set_text(ui_Toggle3Labels[i], toggle_names[i]);
        lv_obj_add_style(ui_Toggle3Labels[i], &style_text_small, 0);
        lv_obj_align(ui_Toggle3Labels[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    
    // 3 Switches on page 1
    const char *sw_names[] = {"S1", "S2", "S3"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *sw_cont = lv_obj_create(row);
        lv_obj_set_size(sw_cont, 32, INPUT_PANEL_HEIGHT - 12);
        lv_obj_set_style_bg_opa(sw_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sw_cont, 0, 0);
        lv_obj_set_style_pad_all(sw_cont, 0, 0);
        lv_obj_remove_flag(sw_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        ui_Switches[i] = lv_obj_create(sw_cont);
        lv_obj_set_size(ui_Switches[i], 28, 20);
        lv_obj_align(ui_Switches[i], LV_ALIGN_TOP_MID, 0, 4);
        lv_obj_add_style(ui_Switches[i], &style_switch_off, 0);
        lv_obj_remove_flag(ui_Switches[i], LV_OBJ_FLAG_SCROLLABLE);
        
        ui_SwitchLabels[i] = lv_label_create(sw_cont);
        lv_label_set_text(ui_SwitchLabels[i], sw_names[i]);
        lv_obj_add_style(ui_SwitchLabels[i], &style_text_small, 0);
        lv_obj_align(ui_SwitchLabels[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

//=============================================================================
// Create Page 2 Content: Switches(3) + Pots + Encoders
//=============================================================================

static void create_page2_content(lv_obj_t *page)
{
    lv_obj_t *row = lv_obj_create(page);
    lv_obj_set_size(row, UI_SCREEN_WIDTH - 8, INPUT_PANEL_HEIGHT - 8);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 3 more switches
    const char *sw_names[] = {"S4", "S5", "S6"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *sw_cont = lv_obj_create(row);
        lv_obj_set_size(sw_cont, 32, INPUT_PANEL_HEIGHT - 12);
        lv_obj_set_style_bg_opa(sw_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sw_cont, 0, 0);
        lv_obj_set_style_pad_all(sw_cont, 0, 0);
        lv_obj_remove_flag(sw_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        ui_Switches[i + 3] = lv_obj_create(sw_cont);
        lv_obj_set_size(ui_Switches[i + 3], 28, 20);
        lv_obj_align(ui_Switches[i + 3], LV_ALIGN_TOP_MID, 0, 4);
        lv_obj_add_style(ui_Switches[i + 3], &style_switch_off, 0);
        lv_obj_remove_flag(ui_Switches[i + 3], LV_OBJ_FLAG_SCROLLABLE);
        
        ui_SwitchLabels[i + 3] = lv_label_create(sw_cont);
        lv_label_set_text(ui_SwitchLabels[i + 3], sw_names[i]);
        lv_obj_add_style(ui_SwitchLabels[i + 3], &style_text_small, 0);
        lv_obj_align(ui_SwitchLabels[i + 3], LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    
    // 2 Potentiometers (vertical layout)
    const char *pot_names[] = {"P1", "P2"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *pot_cont = lv_obj_create(row);
        lv_obj_set_size(pot_cont, 50, INPUT_PANEL_HEIGHT - 12);
        lv_obj_set_style_bg_opa(pot_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(pot_cont, 0, 0);
        lv_obj_set_style_pad_all(pot_cont, 0, 0);
        lv_obj_remove_flag(pot_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        ui_PotLabels[i] = lv_label_create(pot_cont);
        lv_label_set_text(ui_PotLabels[i], pot_names[i]);
        lv_obj_add_style(ui_PotLabels[i], &style_text_small, 0);
        lv_obj_align(ui_PotLabels[i], LV_ALIGN_TOP_MID, 0, 0);
        
        ui_PotBars[i] = lv_bar_create(pot_cont);
        lv_obj_set_size(ui_PotBars[i], 44, 8);
        lv_obj_align(ui_PotBars[i], LV_ALIGN_CENTER, 0, 2);
        lv_bar_set_range(ui_PotBars[i], 0, 1000);
        lv_bar_set_value(ui_PotBars[i], 500, LV_ANIM_OFF);
        lv_obj_add_style(ui_PotBars[i], &style_bar_bg, LV_PART_MAIN);
        lv_obj_add_style(ui_PotBars[i], &style_bar_indicator, LV_PART_INDICATOR);
        
        ui_PotValues[i] = lv_label_create(pot_cont);
        lv_label_set_text(ui_PotValues[i], "50%");
        lv_obj_add_style(ui_PotValues[i], &style_text_small, 0);
        lv_obj_align(ui_PotValues[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    
    // 2 Encoders
    const char *enc_names[] = {"E1", "E2"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *enc_cont = lv_obj_create(row);
        lv_obj_set_size(enc_cont, 50, INPUT_PANEL_HEIGHT - 12);
        lv_obj_add_style(enc_cont, &style_card, 0);
        lv_obj_remove_flag(enc_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        ui_EncLabels[i] = lv_label_create(enc_cont);
        lv_label_set_text(ui_EncLabels[i], enc_names[i]);
        lv_obj_add_style(ui_EncLabels[i], &style_text_small, 0);
        lv_obj_align(ui_EncLabels[i], LV_ALIGN_TOP_MID, 0, 0);
        
        ui_EncValues[i] = lv_label_create(enc_cont);
        lv_label_set_text(ui_EncValues[i], "0");
        lv_obj_add_style(ui_EncValues[i], &style_text_primary, 0);
        lv_obj_set_style_text_font(ui_EncValues[i], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_EncValues[i], LV_ALIGN_BOTTOM_MID, 0, -2);
        
        ui_EncPanels[i] = enc_cont;
    }
}

//=============================================================================
// Create Input Screen
//=============================================================================

static void create_input_screen(void)
{
    ui_InputScreen = lv_obj_create(ui_ContentArea);
    lv_obj_set_size(ui_InputScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_InputScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_InputScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_InputScreen, 0, 0);
    lv_obj_remove_flag(ui_InputScreen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Gimbal Panel (larger area) ===
    ui_GimbalPanel = lv_obj_create(ui_InputScreen);
    lv_obj_set_size(ui_GimbalPanel, lv_pct(100), GIMBAL_PANEL_HEIGHT);
    lv_obj_align(ui_GimbalPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_GimbalPanel, &style_panel, 0);
    lv_obj_remove_flag(ui_GimbalPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(ui_GimbalPanel, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_flex_flow(ui_GimbalPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_GimbalPanel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    ui_GimbalLeft = create_gimbal_widget(ui_GimbalPanel, &ui_GimbalLeftDot, "L");
    ui_GimbalRight = create_gimbal_widget(ui_GimbalPanel, &ui_GimbalRightDot, "R");
    
    // === Swipeable Input Panel ===
    ui_InputControlsPanel = lv_obj_create(ui_InputScreen);
    lv_obj_set_size(ui_InputControlsPanel, UI_SCREEN_WIDTH, INPUT_PANEL_HEIGHT);
    lv_obj_align(ui_InputControlsPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ui_InputControlsPanel, &style_panel, 0);
    lv_obj_set_style_border_width(ui_InputControlsPanel, 0, 0);
    lv_obj_set_style_pad_all(ui_InputControlsPanel, 0, 0);
    
    // Enable horizontal scrolling with snap
    lv_obj_add_flag(ui_InputControlsPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui_InputControlsPanel, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(ui_InputControlsPanel, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(ui_InputControlsPanel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(ui_InputControlsPanel, input_panel_scroll_event_cb, LV_EVENT_SCROLL_END, NULL);
    
    // Page 1 container
    ui_InputPage1 = lv_obj_create(ui_InputControlsPanel);
    lv_obj_set_size(ui_InputPage1, UI_SCREEN_WIDTH, INPUT_PANEL_HEIGHT - 4);
    lv_obj_set_pos(ui_InputPage1, 0, 0);
    lv_obj_set_style_bg_opa(ui_InputPage1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_InputPage1, 0, 0);
    lv_obj_set_style_pad_all(ui_InputPage1, 2, 0);
    lv_obj_remove_flag(ui_InputPage1, LV_OBJ_FLAG_SCROLLABLE);
    
    // Page 2 container
    ui_InputPage2 = lv_obj_create(ui_InputControlsPanel);
    lv_obj_set_size(ui_InputPage2, UI_SCREEN_WIDTH, INPUT_PANEL_HEIGHT - 4);
    lv_obj_set_pos(ui_InputPage2, UI_SCREEN_WIDTH, 0);
    lv_obj_set_style_bg_opa(ui_InputPage2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_InputPage2, 0, 0);
    lv_obj_set_style_pad_all(ui_InputPage2, 2, 0);
    lv_obj_remove_flag(ui_InputPage2, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create content
    create_page1_content(ui_InputPage1);
    create_page2_content(ui_InputPage2);
}

//=============================================================================
// Create Telemetry Screen
//=============================================================================

static void create_telemetry_screen(void)
{
    ui_TelemetryScreen = lv_obj_create(ui_ContentArea);
    lv_obj_set_size(ui_TelemetryScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_TelemetryScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_TelemetryScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_TelemetryScreen, 2, 0);
    lv_obj_remove_flag(ui_TelemetryScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Panel 1: Robot Data
    ui_TelemetryPanel1 = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(ui_TelemetryPanel1, 155, UI_CONTENT_HEIGHT - 8);
    lv_obj_align(ui_TelemetryPanel1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(ui_TelemetryPanel1, &style_card, 0);
    lv_obj_set_style_pad_all(ui_TelemetryPanel1, 4, 0);
    lv_obj_add_flag(ui_TelemetryPanel1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_TelemetryPanel1, LV_SCROLLBAR_MODE_AUTO);
    
    lv_obj_t *title1 = lv_label_create(ui_TelemetryPanel1);
    lv_label_set_text(title1, "Robot Data");
    lv_obj_add_style(title1, &style_text_primary, 0);
    lv_obj_set_style_text_font(title1, &lv_font_montserrat_12, 0);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 0);
    
    const char *labels1[] = {"Temp 1:", "Temp 2:", "Bat V:", "Current:"};
    const char *values1[] = {"--°C", "--°C", "--V", "--A"};
    for (int i = 0; i < 4; i++) {
        ui_TelemetryLabels[i] = lv_label_create(ui_TelemetryPanel1);
        lv_label_set_text(ui_TelemetryLabels[i], labels1[i]);
        lv_obj_add_style(ui_TelemetryLabels[i], &style_text_secondary, 0);
        lv_obj_set_style_text_font(ui_TelemetryLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_TelemetryLabels[i], LV_ALIGN_TOP_LEFT, 0, 18 + i * 18);
        
        ui_TelemetryValues[i] = lv_label_create(ui_TelemetryPanel1);
        lv_label_set_text(ui_TelemetryValues[i], values1[i]);
        lv_obj_add_style(ui_TelemetryValues[i], &style_text_primary, 0);
        lv_obj_set_style_text_font(ui_TelemetryValues[i], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_TelemetryValues[i], LV_ALIGN_TOP_RIGHT, 0, 18 + i * 18);
    }
    
    // Panel 2: Status
    ui_TelemetryPanel2 = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(ui_TelemetryPanel2, 155, UI_CONTENT_HEIGHT - 8);
    lv_obj_align(ui_TelemetryPanel2, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_style(ui_TelemetryPanel2, &style_card, 0);
    lv_obj_set_style_pad_all(ui_TelemetryPanel2, 4, 0);
    lv_obj_add_flag(ui_TelemetryPanel2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_TelemetryPanel2, LV_SCROLLBAR_MODE_AUTO);
    
    lv_obj_t *title2 = lv_label_create(ui_TelemetryPanel2);
    lv_label_set_text(title2, "Status");
    lv_obj_add_style(title2, &style_text_primary, 0);
    lv_obj_set_style_text_font(title2, &lv_font_montserrat_12, 0);
    lv_obj_align(title2, LV_ALIGN_TOP_MID, 0, 0);
    
    const char *labels2[] = {"RSSI:", "Latency:", "Errors:", "Uptime:"};
    const char *values2[] = {"--dBm", "--ms", "0", "--"};
    for (int i = 0; i < 4; i++) {
        ui_TelemetryLabels[i + 4] = lv_label_create(ui_TelemetryPanel2);
        lv_label_set_text(ui_TelemetryLabels[i + 4], labels2[i]);
        lv_obj_add_style(ui_TelemetryLabels[i + 4], &style_text_secondary, 0);
        lv_obj_set_style_text_font(ui_TelemetryLabels[i + 4], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_TelemetryLabels[i + 4], LV_ALIGN_TOP_LEFT, 0, 18 + i * 18);
        
        ui_TelemetryValues[i + 4] = lv_label_create(ui_TelemetryPanel2);
        lv_label_set_text(ui_TelemetryValues[i + 4], values2[i]);
        lv_obj_add_style(ui_TelemetryValues[i + 4], &style_text_primary, 0);
        lv_obj_set_style_text_font(ui_TelemetryValues[i + 4], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_TelemetryValues[i + 4], LV_ALIGN_TOP_RIGHT, 0, 18 + i * 18);
    }
}

//=============================================================================
// Create Settings Screen
//=============================================================================

static void create_settings_screen(void)
{
    ui_SettingsScreen = lv_obj_create(ui_ContentArea);
    lv_obj_set_size(ui_SettingsScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_SettingsScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_SettingsScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_SettingsScreen, 4, 0);
    lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_SettingsScreen, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t *title = lv_label_create(ui_SettingsScreen);
    lv_label_set_text(title, "Settings");
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);
    
    const char *settings[] = {"WiFi Config", "Calibration", "Display", "About"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(ui_SettingsScreen);
        lv_obj_set_size(btn, lv_pct(90), 30);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 30 + i * 38);
        lv_obj_add_style(btn, &style_card, 0);
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, settings[i]);
        lv_obj_add_style(lbl, &style_text_primary, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void ui_custom_init(void)
{
    init_styles();
    
    ui_MainScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_MainScreen, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_bg_opa(ui_MainScreen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(ui_MainScreen, LV_OBJ_FLAG_SCROLLABLE);
    
    create_header();
    
    ui_ContentArea = lv_obj_create(ui_MainScreen);
    lv_obj_set_size(ui_ContentArea, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(ui_ContentArea, 0, UI_HEADER_HEIGHT);
    lv_obj_set_style_bg_opa(ui_ContentArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_ContentArea, 0, 0);
    lv_obj_set_style_pad_all(ui_ContentArea, 0, 0);
    lv_obj_remove_flag(ui_ContentArea, LV_OBJ_FLAG_SCROLLABLE);
    
    create_input_screen();
    create_telemetry_screen();
    create_settings_screen();
    create_nav_bar();
    
    lv_screen_load(ui_MainScreen);
}

void ui_custom_destroy(void)
{
    if (ui_MainScreen) {
        lv_obj_del(ui_MainScreen);
        ui_MainScreen = NULL;
    }
}

void ui_show_screen(ScreenType_t screen)
{
    if (screen >= SCREEN_COUNT) return;
    
    if (ui_InputScreen) lv_obj_add_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
    if (ui_TelemetryScreen) lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
    if (ui_SettingsScreen) lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
    
    switch (screen) {
        case SCREEN_INPUT:
            if (ui_InputScreen) lv_obj_remove_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
            break;
        case SCREEN_TELEMETRY:
            if (ui_TelemetryScreen) lv_obj_remove_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
            break;
        case SCREEN_SETTINGS:
            if (ui_SettingsScreen) lv_obj_remove_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
    
    current_screen = screen;
    update_nav_buttons(screen);
}

ScreenType_t ui_get_current_screen(void)
{
    return current_screen;
}

void ui_set_gimbal_left(int16_t x, int16_t y)
{
    if (!ui_GimbalLeftDot) return;
    int16_t gimbal_size = 70;
    int16_t dot_size = 14;
    int16_t max_offset = (gimbal_size - dot_size) / 2 - 2;
    int16_t px = (x * max_offset) / 1000;
    int16_t py = (-y * max_offset) / 1000;
    lv_obj_align(ui_GimbalLeftDot, LV_ALIGN_CENTER, px, py);
}

void ui_set_gimbal_right(int16_t x, int16_t y)
{
    if (!ui_GimbalRightDot) return;
    int16_t gimbal_size = 70;
    int16_t dot_size = 14;
    int16_t max_offset = (gimbal_size - dot_size) / 2 - 2;
    int16_t px = (x * max_offset) / 1000;
    int16_t py = (-y * max_offset) / 1000;
    lv_obj_align(ui_GimbalRightDot, LV_ALIGN_CENTER, px, py);
}

void ui_set_button(uint8_t index, bool pressed)
{
    if (index >= 4 || !ui_Buttons[index]) return;
    lv_obj_remove_style(ui_Buttons[index], pressed ? &style_btn_default : &style_btn_pressed, 0);
    lv_obj_add_style(ui_Buttons[index], pressed ? &style_btn_pressed : &style_btn_default, 0);
}

void ui_set_switch(uint8_t index, bool on)
{
    if (index >= 6 || !ui_Switches[index]) return;
    lv_obj_remove_style(ui_Switches[index], on ? &style_switch_off : &style_switch_on, 0);
    lv_obj_add_style(ui_Switches[index], on ? &style_switch_on : &style_switch_off, 0);
}

void ui_set_toggle3(uint8_t index, uint8_t position)
{
    if (index >= 2 || !ui_Toggle3Indicators[index]) return;
    int16_t y_offset = (position == 0) ? -8 : (position == 2) ? 8 : 0;
    lv_color_t color = (position == 0) ? lv_color_hex(UI_COLOR_ACCENT_GREEN) :
                       (position == 2) ? lv_color_hex(UI_COLOR_ACCENT_RED) :
                       lv_color_hex(UI_COLOR_ACCENT_ORANGE);
    lv_obj_align(ui_Toggle3Indicators[index], LV_ALIGN_CENTER, 0, y_offset);
    lv_obj_set_style_bg_color(ui_Toggle3Indicators[index], color, 0);
}

void ui_set_pot(uint8_t index, int16_t value)
{
    if (index >= 2 || !ui_PotBars[index]) return;
    if (value < 0) value = 0;
    if (value > 1000) value = 1000;
    lv_bar_set_value(ui_PotBars[index], value, LV_ANIM_OFF);
    
    if (ui_PotValues[index]) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", value / 10);
        lv_label_set_text(ui_PotValues[index], buf);
    }
}

void ui_set_encoder(uint8_t index, int32_t value)
{
    if (index >= 2 || !ui_EncValues[index]) return;
    char buf[12];
    snprintf(buf, sizeof(buf), "%ld", (long)value);
    lv_label_set_text(ui_EncValues[index], buf);
}

void ui_set_telemetry_value(uint8_t index, const char *label, const char *value)
{
    if (index >= 8) return;
    if (ui_TelemetryLabels[index] && label) {
        lv_label_set_text(ui_TelemetryLabels[index], label);
    }
    if (ui_TelemetryValues[index] && value) {
        lv_label_set_text(ui_TelemetryValues[index], value);
    }
}

void ui_set_battery(uint8_t percent, BatteryState_t state)
{
    if (!ui_BatteryLabel || !ui_BatteryIcon) return;
    
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(ui_BatteryLabel, buf);
    
    const char *icon;
    lv_color_t color;
    
    if (state == BATTERY_STATE_CHARGING) {
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

void ui_custom_update(void)
{
    // Called in main loop if needed
}
