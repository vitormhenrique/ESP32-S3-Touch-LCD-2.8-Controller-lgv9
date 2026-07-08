#include "ui_screen_settings.h"
#include "ui_screen_telemetry.h"
#include "../ui_styles.h"
#include "../ui_helpers.h"
#include "../components/ui_chrome.h"
#include "../../Settings.h"
#include "../../ui_custom_integration.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *ui_SettingsScreen = NULL;

// Menu containers
static lv_obj_t *menu_main = NULL;
static lv_obj_t *menu_radio = NULL;
static lv_obj_t *menu_robot = NULL;
static lv_obj_t *menu_touch_cal = NULL;
static lv_obj_t *menu_gimbal_cal = NULL;
static lv_obj_t *menu_about = NULL;

// Current menu state
static SettingsMenu_t current_menu = SETTINGS_MENU_MAIN;

// Touch calibration state
static bool touch_cal_active = false;
static uint8_t touch_cal_point = 0;
static lv_obj_t *touch_cal_target = NULL;
static lv_obj_t *touch_cal_label = NULL;
static lv_obj_t *touch_cal_overlay = NULL;  // Clickable overlay for capturing touches
static lv_obj_t *touch_cal_start_btn = NULL;
static int16_t touch_cal_raw[4][2] = {{0}}; // 4 points, raw x/y

// Gimbal calibration UI
static lv_obj_t *gimbal_bars[4] = {NULL};
static lv_obj_t *gimbal_labels[4] = {NULL};

// Gimbal calibration state
static bool gimbal_cal_active = false;
static uint8_t gimbal_cal_axis = 0;        // Current axis being calibrated (0-3)
static uint8_t gimbal_cal_step = 0;        // 0=center, 1=min, 2=max
static lv_obj_t *gimbal_cal_instruction = NULL;
static lv_obj_t *gimbal_cal_axis_label = NULL;
static lv_obj_t *gimbal_cal_value_label = NULL;
static lv_obj_t *gimbal_cal_bar = NULL;
static lv_obj_t *gimbal_cal_continue_btn = NULL;
static lv_obj_t *gimbal_cal_start_btn = NULL;
static lv_obj_t *gimbal_cal_status_panel = NULL;
static int16_t gimbal_cal_current_value = 0;   // Current live reading
static int16_t gimbal_cal_recorded[4][3] = {{0}}; // [axis][center/min/max]
static const char *gimbal_axis_names[] = {"Left X", "Left Y", "Right X", "Right Y"};

// Robot profile selection buttons and checkmarks
static lv_obj_t *robot_profile_btns[ROBOT_PROFILE_COUNT] = {NULL};
static lv_obj_t *robot_profile_checks[ROBOT_PROFILE_COUNT] = {NULL};

// Forward declarations
static void create_main_menu(lv_obj_t *parent);
static void create_radio_menu(lv_obj_t *parent);
static void create_robot_menu(lv_obj_t *parent);
static void create_touch_cal_menu(lv_obj_t *parent);
static void create_gimbal_cal_menu(lv_obj_t *parent);
static void create_about_menu(lv_obj_t *parent);
static void hide_all_menus(void);

//=============================================================================
// Event Callbacks
//=============================================================================

static void back_btn_cb(lv_event_t *e) {
    (void)e;
    ui_settings_show_menu(SETTINGS_MENU_MAIN);
}

static void menu_btn_cb(lv_event_t *e) {
    intptr_t menu_id = (intptr_t)lv_event_get_user_data(e);
    ui_settings_show_menu((SettingsMenu_t)menu_id);
}

static void update_robot_profile_selection(RobotProfile_t selected) {
    for (int i = 0; i < ROBOT_PROFILE_COUNT; i++) {
        if (robot_profile_btns[i]) {
            if (i == selected) {
                // Highlight selected profile
                lv_obj_set_style_bg_color(robot_profile_btns[i], lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
                lv_obj_set_style_border_color(robot_profile_btns[i], lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
                lv_obj_set_style_border_width(robot_profile_btns[i], 2, 0);
                // Show checkmark
                if (robot_profile_checks[i]) {
                    lv_obj_remove_flag(robot_profile_checks[i], LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                // Normal style for non-selected
                lv_obj_set_style_bg_color(robot_profile_btns[i], lv_color_hex(UI_COLOR_BG_CARD), 0);
                lv_obj_set_style_border_color(robot_profile_btns[i], lv_color_hex(UI_COLOR_BORDER), 0);
                lv_obj_set_style_border_width(robot_profile_btns[i], 1, 0);
                // Hide checkmark
                if (robot_profile_checks[i]) {
                    lv_obj_add_flag(robot_profile_checks[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    }
}

static void robot_select_cb(lv_event_t *e) {
    intptr_t profile = (intptr_t)lv_event_get_user_data(e);
    Settings_SetRobotProfile((RobotProfile_t)profile);
    update_robot_profile_selection((RobotProfile_t)profile);
    ui_set_title(Settings_GetRobotProfileName((RobotProfile_t)profile));
    // Refresh telemetry panels for the new profile
    ui_telemetry_refresh_for_profile((RobotProfile_t)profile);
    printf("Settings: Robot profile set to %s\r\n", Settings_GetRobotProfileName((RobotProfile_t)profile));
}

static void touch_cal_start_cb(lv_event_t *e) {
    (void)e;
    ui_touch_cal_start();
}

// Touch calibration overlay click handler
static void touch_cal_overlay_cb(lv_event_t *e) {
    if (!touch_cal_active) return;
    
    lv_point_t point;
    lv_indev_get_point(lv_indev_active(), &point);
    
    // Record the point (using screen coordinates as "raw" for simulator)
    ui_touch_cal_record_point(point.x, point.y);
}

// Gimbal calibration callbacks
static void gimbal_cal_start_cb(lv_event_t *e);
static void gimbal_cal_continue_cb(lv_event_t *e);
static void gimbal_cal_update_ui(void);

//=============================================================================
// Menu Creation
//=============================================================================

static lv_obj_t* create_back_header(lv_obj_t *parent, const char *title) {
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 26);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *back = lv_button_create(header);
    lv_obj_set_size(back, 56, 22);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(back, &style_card, 0);
    lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(back, back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(back_lbl);
    
    lv_obj_t *title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, title);
    lv_obj_add_style(title_lbl, &style_text_primary, 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(title_lbl, LV_ALIGN_CENTER, 10, 0);
    
    return header;
}

static void create_main_menu(lv_obj_t *parent) {
    menu_main = lv_obj_create(parent);
    lv_obj_set_size(menu_main, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_main, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_main, 0, 0);
    lv_obj_set_style_pad_all(menu_main, 6, 0);
    lv_obj_remove_flag(menu_main, LV_OBJ_FLAG_SCROLLABLE);
    
    typedef struct {
        const char *icon;
        const char *label;
        SettingsMenu_t menu;
    } MenuItem;
    
    MenuItem items[] = {
        {LV_SYMBOL_WIFI, "Radio (ELRS)", SETTINGS_MENU_RADIO},
        {LV_SYMBOL_DIRECTORY, "Robot Profile", SETTINGS_MENU_ROBOT},
        {LV_SYMBOL_GPS, "Touch Calibration", SETTINGS_MENU_TOUCH_CAL},
        {LV_SYMBOL_REFRESH, "Gimbal Calibration", SETTINGS_MENU_GIMBAL_CAL},
        {LV_SYMBOL_FILE, "About", SETTINGS_MENU_ABOUT},
    };
    int n = sizeof(items) / sizeof(items[0]);
    
    for (int i = 0; i < n; i++) {
        lv_obj_t *btn = lv_button_create(menu_main);
        lv_obj_set_size(btn, lv_pct(98), 28);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 2 + i * 32);
        lv_obj_add_style(btn, &style_card, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_pad_left(btn, 12, 0);
        lv_obj_set_style_pad_right(btn, 8, 0);
        lv_obj_add_event_cb(btn, menu_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)items[i].menu);
        
        // Icon on the left with accent color
        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, items[i].icon);
        lv_obj_set_style_text_color(icon, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
        
        // Label text
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, items[i].label);
        lv_obj_add_style(lbl, &style_text_primary, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 24, 0);
        
        // Arrow on the right
        lv_obj_t *arrow = lv_label_create(btn);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_add_style(arrow, &style_text_secondary, 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

static void create_radio_menu(lv_obj_t *parent) {
    menu_radio = lv_obj_create(parent);
    lv_obj_set_size(menu_radio, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_radio, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_radio, 0, 0);
    lv_obj_set_style_pad_all(menu_radio, 4, 0);
    lv_obj_add_flag(menu_radio, LV_OBJ_FLAG_HIDDEN);
    
    create_back_header(menu_radio, "Radio (ELRS)");
    
    const Settings_t *s = Settings_Get();
    int y = 30;
    
    // Packet Rate
    lv_obj_t *rate_lbl = lv_label_create(menu_radio);
    lv_label_set_text(rate_lbl, "Packet Rate:");
    lv_obj_add_style(rate_lbl, &style_text_secondary, 0);
    lv_obj_align(rate_lbl, LV_ALIGN_TOP_LEFT, 4, y);
    
    lv_obj_t *rate_val = lv_label_create(menu_radio);
    lv_label_set_text(rate_val, Settings_GetPacketRateName(s->radio.packet_rate));
    lv_obj_add_style(rate_val, &style_text_primary, 0);
    lv_obj_align(rate_val, LV_ALIGN_TOP_RIGHT, -4, y);
    y += 22;
    
    // TX Power
    lv_obj_t *pwr_lbl = lv_label_create(menu_radio);
    lv_label_set_text(pwr_lbl, "TX Power:");
    lv_obj_add_style(pwr_lbl, &style_text_secondary, 0);
    lv_obj_align(pwr_lbl, LV_ALIGN_TOP_LEFT, 4, y);
    
    lv_obj_t *pwr_val = lv_label_create(menu_radio);
    lv_label_set_text(pwr_val, Settings_GetTxPowerName(s->radio.tx_power));
    lv_obj_add_style(pwr_val, &style_text_primary, 0);
    lv_obj_align(pwr_val, LV_ALIGN_TOP_RIGHT, -4, y);
    y += 22;
    
    // Telemetry
    lv_obj_t *telem_lbl = lv_label_create(menu_radio);
    lv_label_set_text(telem_lbl, "Telemetry:");
    lv_obj_add_style(telem_lbl, &style_text_secondary, 0);
    lv_obj_align(telem_lbl, LV_ALIGN_TOP_LEFT, 4, y);
    
    lv_obj_t *telem_val = lv_label_create(menu_radio);
    lv_label_set_text(telem_val, Settings_GetTelemetryRatioName(s->radio.telemetry_ratio));
    lv_obj_add_style(telem_val, &style_text_primary, 0);
    lv_obj_align(telem_val, LV_ALIGN_TOP_RIGHT, -4, y);
    y += 22;
    
    // Bind Phrase
    lv_obj_t *bind_lbl = lv_label_create(menu_radio);
    lv_label_set_text(bind_lbl, "Bind Phrase:");
    lv_obj_add_style(bind_lbl, &style_text_secondary, 0);
    lv_obj_align(bind_lbl, LV_ALIGN_TOP_LEFT, 4, y);
    
    lv_obj_t *bind_val = lv_label_create(menu_radio);
    lv_label_set_text(bind_val, s->radio.bind_phrase);
    lv_obj_add_style(bind_val, &style_text_primary, 0);
    lv_obj_align(bind_val, LV_ALIGN_TOP_RIGHT, -4, y);
}

static void create_robot_menu(lv_obj_t *parent) {
    menu_robot = lv_obj_create(parent);
    lv_obj_set_size(menu_robot, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_robot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_robot, 0, 0);
    lv_obj_set_style_pad_all(menu_robot, 4, 0);
    lv_obj_add_flag(menu_robot, LV_OBJ_FLAG_HIDDEN);
    
    create_back_header(menu_robot, "Robot Profile");
    
    const char *profiles[] = {"Generic", "Hexapod"};
    
    const Settings_t *s = Settings_Get();
    
    for (int i = 0; i < ROBOT_PROFILE_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(menu_robot);
        lv_obj_set_size(btn, lv_pct(95), 36);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 30 + i * 44);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_add_event_cb(btn, robot_select_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        
        // Store button reference
        robot_profile_btns[i] = btn;
        
        // Profile name - centered vertically
        lv_obj_t *name = lv_label_create(btn);
        lv_label_set_text(name, profiles[i]);
        lv_obj_add_style(name, &style_text_primary, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_12, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 8, 0);
        
        // Add checkmark (created for all, visibility controlled by update function)
        lv_obj_t *check = lv_label_create(btn);
        lv_label_set_text(check, LV_SYMBOL_OK);
        lv_obj_add_style(check, &style_text_primary, 0);
        lv_obj_align(check, LV_ALIGN_RIGHT_MID, -8, 0);
        robot_profile_checks[i] = check;
    }
    
    // Set initial highlight based on current setting
    update_robot_profile_selection(s->robot_profile);
}

static void create_touch_cal_menu(lv_obj_t *parent) {
    menu_touch_cal = lv_obj_create(parent);
    lv_obj_set_size(menu_touch_cal, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_touch_cal, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_touch_cal, 0, 0);
    lv_obj_set_style_pad_all(menu_touch_cal, 0, 0);
    lv_obj_add_flag(menu_touch_cal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(menu_touch_cal, LV_OBJ_FLAG_SCROLLABLE);
    
    create_back_header(menu_touch_cal, "Touch Calibration");
    
    touch_cal_label = lv_label_create(menu_touch_cal);
    lv_label_set_text(touch_cal_label, "Press Start, then tap the\ncrosshairs that appear.");
    lv_obj_add_style(touch_cal_label, &style_text_secondary, 0);
    lv_obj_set_style_text_align(touch_cal_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(touch_cal_label, LV_ALIGN_CENTER, 0, -15);
    
    touch_cal_start_btn = lv_button_create(menu_touch_cal);
    lv_obj_set_size(touch_cal_start_btn, 100, 28);
    lv_obj_align(touch_cal_start_btn, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_style_bg_color(touch_cal_start_btn, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_add_event_cb(touch_cal_start_btn, touch_cal_start_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *start_lbl = lv_label_create(touch_cal_start_btn);
    lv_label_set_text(start_lbl, LV_SYMBOL_PLAY " Start");
    lv_obj_center(start_lbl);
    
    // Calibration target crosshair (hidden until calibration starts)
    touch_cal_target = lv_obj_create(menu_touch_cal);
    lv_obj_set_size(touch_cal_target, 30, 30);
    lv_obj_set_style_bg_opa(touch_cal_target, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(touch_cal_target, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(touch_cal_target, 2, 0);
    lv_obj_set_style_radius(touch_cal_target, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
    
    // Transparent overlay to capture touch events during calibration
    touch_cal_overlay = lv_obj_create(menu_touch_cal);
    lv_obj_set_size(touch_cal_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(touch_cal_overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(touch_cal_overlay, 0, 0);
    lv_obj_remove_flag(touch_cal_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(touch_cal_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(touch_cal_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(touch_cal_overlay, touch_cal_overlay_cb, LV_EVENT_CLICKED, NULL);
}

static void create_gimbal_cal_menu(lv_obj_t *parent) {
    menu_gimbal_cal = lv_obj_create(parent);
    lv_obj_set_size(menu_gimbal_cal, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_gimbal_cal, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_gimbal_cal, 0, 0);
    lv_obj_set_style_pad_all(menu_gimbal_cal, 4, 0);
    lv_obj_add_flag(menu_gimbal_cal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(menu_gimbal_cal, LV_OBJ_FLAG_SCROLLABLE);
    
    create_back_header(menu_gimbal_cal, "Gimbal Calibration");
    
    // Instruction label - centered, shows current step (includes axis name)
    gimbal_cal_instruction = lv_label_create(menu_gimbal_cal);
    lv_label_set_text(gimbal_cal_instruction, "Press Start to calibrate\nall gimbal axes.");
    lv_obj_add_style(gimbal_cal_instruction, &style_text_secondary, 0);
    lv_obj_set_style_text_align(gimbal_cal_instruction, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(gimbal_cal_instruction, LV_ALIGN_TOP_MID, 0, 28);
    
    // Axis name label - not used separately anymore (integrated in instruction)
    gimbal_cal_axis_label = lv_label_create(menu_gimbal_cal);
    lv_label_set_text(gimbal_cal_axis_label, "");
    lv_obj_add_flag(gimbal_cal_axis_label, LV_OBJ_FLAG_HIDDEN);  // Always hidden
    
    // Live value bar - centered, prominent
    gimbal_cal_bar = lv_bar_create(menu_gimbal_cal);
    lv_obj_set_size(gimbal_cal_bar, 200, 14);
    lv_obj_align(gimbal_cal_bar, LV_ALIGN_CENTER, 0, -5);
    lv_bar_set_range(gimbal_cal_bar, 0, 4095);
    lv_bar_set_value(gimbal_cal_bar, 2048, LV_ANIM_OFF);
    lv_obj_add_flag(gimbal_cal_bar, LV_OBJ_FLAG_HIDDEN);
    
    // Live value label - below bar, shows raw value
    gimbal_cal_value_label = lv_label_create(menu_gimbal_cal);
    lv_label_set_text(gimbal_cal_value_label, "2048");
    lv_obj_add_style(gimbal_cal_value_label, &style_text_secondary, 0);
    lv_obj_align(gimbal_cal_value_label, LV_ALIGN_CENTER, 0, 12);
    lv_obj_add_flag(gimbal_cal_value_label, LV_OBJ_FLAG_HIDDEN);
    
    // Compact status row at bottom - shows all 4 values in one line
    gimbal_cal_status_panel = lv_obj_create(menu_gimbal_cal);
    lv_obj_set_size(gimbal_cal_status_panel, lv_pct(95), 24);
    lv_obj_align(gimbal_cal_status_panel, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(gimbal_cal_status_panel, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_set_style_radius(gimbal_cal_status_panel, 4, 0);
    lv_obj_set_style_pad_all(gimbal_cal_status_panel, 2, 0);
    lv_obj_remove_flag(gimbal_cal_status_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create compact horizontal status: LX: val  LY: val  RX: val  RY: val
    static const char* axis_short[] = {"LX", "LY", "RX", "RY"};
    for (int i = 0; i < 4; i++) {
        int x = i * 75 + 4;
        
        lv_obj_t *name = lv_label_create(gimbal_cal_status_panel);
        lv_label_set_text(name, axis_short[i]);
        lv_obj_add_style(name, &style_text_small, 0);
        lv_obj_set_pos(name, x, 4);
        
        gimbal_labels[i] = lv_label_create(gimbal_cal_status_panel);
        lv_label_set_text(gimbal_labels[i], "----");
        lv_obj_add_style(gimbal_labels[i], &style_text_small, 0);
        lv_obj_set_pos(gimbal_labels[i], x + 22, 4);
        
        // No individual bars - keep it clean
        gimbal_bars[i] = NULL;
    }
    
    // Start button - centered
    gimbal_cal_start_btn = lv_button_create(menu_gimbal_cal);
    lv_obj_set_size(gimbal_cal_start_btn, 100, 28);
    lv_obj_align(gimbal_cal_start_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(gimbal_cal_start_btn, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_add_event_cb(gimbal_cal_start_btn, gimbal_cal_start_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *start_lbl = lv_label_create(gimbal_cal_start_btn);
    lv_label_set_text(start_lbl, LV_SYMBOL_PLAY " Start");
    lv_obj_center(start_lbl);
    
    // Record button - below value label (hidden until calibration)
    gimbal_cal_continue_btn = lv_button_create(menu_gimbal_cal);
    lv_obj_set_size(gimbal_cal_continue_btn, 100, 28);
    lv_obj_align(gimbal_cal_continue_btn, LV_ALIGN_CENTER, 0, 38);
    lv_obj_set_style_bg_color(gimbal_cal_continue_btn, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_add_event_cb(gimbal_cal_continue_btn, gimbal_cal_continue_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(gimbal_cal_continue_btn, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t *cont_lbl = lv_label_create(gimbal_cal_continue_btn);
    lv_label_set_text(cont_lbl, LV_SYMBOL_OK " Record");
    lv_obj_center(cont_lbl);
}

// Gimbal calibration start callback
static void gimbal_cal_start_cb(lv_event_t *e) {
    (void)e;
    ui_gimbal_cal_start();
}

// Gimbal calibration continue/record callback
static void gimbal_cal_continue_cb(lv_event_t *e) {
    (void)e;
    ui_gimbal_cal_record_step();
}

// Update gimbal calibration UI based on current state
static void gimbal_cal_update_ui(void) {
    if (!gimbal_cal_active) return;
    
    // Short axis names for display
    static const char* axis_display[] = {"Left X", "Left Y", "Right X", "Right Y"};
    const char *step_actions[] = {"release stick", "push to MIN", "push to MAX"};
    
    // Update instruction text - show axis and step info together
    char buf[80];
    snprintf(buf, sizeof(buf), "%s (%d/4) - Step %d/3:\n%s", 
             axis_display[gimbal_cal_axis],
             gimbal_cal_axis + 1,
             gimbal_cal_step + 1,
             step_actions[gimbal_cal_step]);
    lv_label_set_text(gimbal_cal_instruction, buf);
}

static void create_about_menu(lv_obj_t *parent) {
    menu_about = lv_obj_create(parent);
    lv_obj_set_size(menu_about, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_about, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_about, 0, 0);
    lv_obj_set_style_pad_all(menu_about, 4, 0);
    lv_obj_add_flag(menu_about, LV_OBJ_FLAG_HIDDEN);
    
    create_back_header(menu_about, "About");
    
    lv_obj_t *title = lv_label_create(menu_about);
    lv_label_set_text(title, "RC Controller");
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);
    
    lv_obj_t *ver = lv_label_create(menu_about);
    lv_label_set_text(ver, "Version 1.0.0");
    lv_obj_add_style(ver, &style_text_secondary, 0);
    lv_obj_align(ver, LV_ALIGN_TOP_MID, 0, 54);
    
    lv_obj_t *hw = lv_label_create(menu_about);
    lv_label_set_text(hw, "ESP32-S3 + LVGL 9");
    lv_obj_add_style(hw, &style_text_secondary, 0);
    lv_obj_align(hw, LV_ALIGN_TOP_MID, 0, 70);
}

static void hide_all_menus(void) {
    if (menu_main) lv_obj_add_flag(menu_main, LV_OBJ_FLAG_HIDDEN);
    if (menu_radio) lv_obj_add_flag(menu_radio, LV_OBJ_FLAG_HIDDEN);
    if (menu_robot) lv_obj_add_flag(menu_robot, LV_OBJ_FLAG_HIDDEN);
    if (menu_touch_cal) lv_obj_add_flag(menu_touch_cal, LV_OBJ_FLAG_HIDDEN);
    if (menu_gimbal_cal) lv_obj_add_flag(menu_gimbal_cal, LV_OBJ_FLAG_HIDDEN);
    if (menu_about) lv_obj_add_flag(menu_about, LV_OBJ_FLAG_HIDDEN);
    
    // Cancel touch calibration if leaving
    touch_cal_active = false;
    if (touch_cal_target) lv_obj_add_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
}

//=============================================================================
// Public API
//=============================================================================

void ui_create_settings_screen(lv_obj_t *parent) {
    printf("Settings: Creating screen container...\r\n");
    ui_SettingsScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_SettingsScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_SettingsScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_SettingsScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_SettingsScreen, 0, 0);
    lv_obj_remove_flag(ui_SettingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
    
    printf("Settings: Creating main menu...\r\n");
    create_main_menu(ui_SettingsScreen);
    printf("Settings: Creating radio menu...\r\n");
    create_radio_menu(ui_SettingsScreen);
    printf("Settings: Creating robot menu...\r\n");
    create_robot_menu(ui_SettingsScreen);
    printf("Settings: Creating touch cal menu...\r\n");
    create_touch_cal_menu(ui_SettingsScreen);
    printf("Settings: Creating gimbal cal menu...\r\n");
    create_gimbal_cal_menu(ui_SettingsScreen);
    printf("Settings: Creating about menu...\r\n");
    create_about_menu(ui_SettingsScreen);
    printf("Settings: All menus created\r\n");
    
    current_menu = SETTINGS_MENU_MAIN;
}

void ui_settings_show_menu(SettingsMenu_t menu) {
    hide_all_menus();
    current_menu = menu;
    
    switch (menu) {
        case SETTINGS_MENU_MAIN:
            lv_obj_remove_flag(menu_main, LV_OBJ_FLAG_HIDDEN);
            break;
        case SETTINGS_MENU_RADIO:
            lv_obj_remove_flag(menu_radio, LV_OBJ_FLAG_HIDDEN);
            break;
        case SETTINGS_MENU_ROBOT:
            lv_obj_remove_flag(menu_robot, LV_OBJ_FLAG_HIDDEN);
            break;
        case SETTINGS_MENU_TOUCH_CAL:
            lv_obj_remove_flag(menu_touch_cal, LV_OBJ_FLAG_HIDDEN);
            break;
        case SETTINGS_MENU_GIMBAL_CAL:
            lv_obj_remove_flag(menu_gimbal_cal, LV_OBJ_FLAG_HIDDEN);
            break;
        case SETTINGS_MENU_ABOUT:
            lv_obj_remove_flag(menu_about, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

SettingsMenu_t ui_settings_get_current_menu(void) {
    return current_menu;
}

//=============================================================================
// Touch Calibration
//=============================================================================

void ui_touch_cal_start(void) {
    touch_cal_active = true;
    touch_cal_point = 0;
    memset(touch_cal_raw, 0, sizeof(touch_cal_raw));
    
    // Hide start button, show overlay
    if (touch_cal_start_btn) lv_obj_add_flag(touch_cal_start_btn, LV_OBJ_FLAG_HIDDEN);
    if (touch_cal_overlay) lv_obj_remove_flag(touch_cal_overlay, LV_OBJ_FLAG_HIDDEN);
    
    // Show first target (top-left) - position matches positions[0] in record_point
    lv_obj_remove_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(touch_cal_target, 15, 35);
    lv_label_set_text(touch_cal_label, "Tap point 1 of 4\n(top-left)");
}

void ui_touch_cal_record_point(int16_t raw_x, int16_t raw_y) {
    if (!touch_cal_active || touch_cal_point >= 4) return;
    
    touch_cal_raw[touch_cal_point][0] = raw_x;
    touch_cal_raw[touch_cal_point][1] = raw_y;
    touch_cal_point++;
    
    // Target positions: TL, TR, BR, BL (corners of calibration area)
    // Keep within visible content area (account for header at top)
    int16_t positions[4][2] = {
        {15, 35},                               // Top-left
        {UI_SCREEN_WIDTH - 45, 35},             // Top-right  
        {UI_SCREEN_WIDTH - 45, 130},            // Bottom-right (well above nav bar)
        {15, 130}                               // Bottom-left (well above nav bar)
    };
    const char *labels[] = {
        "Tap point 2 of 4\n(top-right)",
        "Tap point 3 of 4\n(bottom-right)",
        "Tap point 4 of 4\n(bottom-left)",
        "Calibration complete!"
    };
    
    if (touch_cal_point < 4) {
        lv_obj_set_pos(touch_cal_target, positions[touch_cal_point][0], positions[touch_cal_point][1]);
        lv_label_set_text(touch_cal_label, labels[touch_cal_point - 1]);
        lv_obj_align(touch_cal_label, LV_ALIGN_CENTER, 0, 0);
    } else {
        // Calibration complete
        touch_cal_active = false;
        lv_obj_add_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(touch_cal_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(touch_cal_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(touch_cal_label, "Calibration complete!\nData saved.");
        lv_obj_align(touch_cal_label, LV_ALIGN_CENTER, 0, -15);
        
        // Calculate and save calibration
        TouchCalibration_t cal;
        cal.x_min = (touch_cal_raw[0][0] + touch_cal_raw[3][0]) / 2;
        cal.x_max = (touch_cal_raw[1][0] + touch_cal_raw[2][0]) / 2;
        cal.y_min = (touch_cal_raw[0][1] + touch_cal_raw[1][1]) / 2;
        cal.y_max = (touch_cal_raw[2][1] + touch_cal_raw[3][1]) / 2;
        cal.calibrated = true;
        Settings_SetTouchCalibration(&cal);
        
        printf("Touch cal: x=%d-%d, y=%d-%d\r\n", cal.x_min, cal.x_max, cal.y_min, cal.y_max);
    }
}

bool ui_touch_cal_is_active(void) {
    return touch_cal_active;
}

uint8_t ui_touch_cal_get_point(void) {
    return touch_cal_point;
}

//=============================================================================
// Gimbal Calibration
//=============================================================================

void ui_gimbal_cal_start(void) {
    gimbal_cal_active = true;
    gimbal_cal_axis = 0;
    gimbal_cal_step = 0;
    memset(gimbal_cal_recorded, 0, sizeof(gimbal_cal_recorded));
    
    // Show calibration UI elements (status panel is already visible)
    lv_obj_add_flag(gimbal_cal_start_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(gimbal_cal_continue_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(gimbal_cal_axis_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(gimbal_cal_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(gimbal_cal_value_label, LV_OBJ_FLAG_HIDDEN);
    
    gimbal_cal_update_ui();
    
    printf("Gimbal calibration started\r\n");
}

void ui_gimbal_cal_record_step(void) {
    if (!gimbal_cal_active) return;
    
    // Record current value for this step
    gimbal_cal_recorded[gimbal_cal_axis][gimbal_cal_step] = gimbal_cal_current_value;
    
    printf("Gimbal cal: %s step %d = %d\r\n", 
           gimbal_axis_names[gimbal_cal_axis], gimbal_cal_step, gimbal_cal_current_value);
    
    gimbal_cal_step++;
    
    if (gimbal_cal_step >= 3) {
        // Finished this axis - calculate calibration
        int16_t center = gimbal_cal_recorded[gimbal_cal_axis][0];
        int16_t min_val = gimbal_cal_recorded[gimbal_cal_axis][1];
        int16_t max_val = gimbal_cal_recorded[gimbal_cal_axis][2];
        
        // Detect inversion: if min > max, the axis is inverted
        bool inverted = (min_val > max_val);
        if (inverted) {
            int16_t temp = min_val;
            min_val = max_val;
            max_val = temp;
        }
        
        // Calculate deadzone (2% of range around center)
        int16_t range = max_val - min_val;
        int16_t deadzone = range / 50;  // 2% deadzone
        
        // Save calibration to Settings
        GimbalCalibration_t cal;
        cal.min_raw = min_val;
        cal.max_raw = max_val;
        cal.center_raw = center;
        cal.deadzone = deadzone;
        cal.inverted = inverted;
        cal.calibrated = true;
        Settings_SetGimbalCalibration(gimbal_cal_axis, &cal);
        
        // Apply calibration to the driver immediately
        ui_apply_gimbal_calibration(gimbal_cal_axis, min_val, center, max_val, deadzone, inverted);
        
        printf("Gimbal %s calibrated: min=%d, center=%d, max=%d, dz=%d, inverted=%d\r\n",
               gimbal_axis_names[gimbal_cal_axis], min_val, center, max_val, deadzone, inverted);
        
        // Update status label for this axis
        char buf[16];
        snprintf(buf, sizeof(buf), "%s%s", inverted ? "INV" : "OK", "");
        lv_label_set_text(gimbal_labels[gimbal_cal_axis], buf);
        lv_obj_set_style_text_color(gimbal_labels[gimbal_cal_axis], 
            lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        
        // Move to next axis
        gimbal_cal_axis++;
        gimbal_cal_step = 0;
        
        if (gimbal_cal_axis >= 4) {
            // All axes calibrated!
            gimbal_cal_active = false;
            lv_label_set_text(gimbal_cal_instruction, "Calibration complete!\nAll axes calibrated.");
            lv_obj_add_flag(gimbal_cal_continue_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(gimbal_cal_axis_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(gimbal_cal_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(gimbal_cal_value_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(gimbal_cal_start_btn, LV_OBJ_FLAG_HIDDEN);
            
            // Change start button to "Restart"
            lv_obj_t *lbl = lv_obj_get_child(gimbal_cal_start_btn, 0);
            if (lbl) lv_label_set_text(lbl, LV_SYMBOL_REFRESH " Restart");
            
            printf("Gimbal calibration complete!\r\n");
            return;
        }
    }
    
    gimbal_cal_update_ui();
}

void ui_gimbal_cal_update(int16_t values[4]) {
    char buf[16];
    
    // Update all status bars and labels (always, for live display)
    for (int i = 0; i < 4; i++) {
        if (gimbal_bars[i]) {
            lv_bar_set_value(gimbal_bars[i], values[i], LV_ANIM_OFF);
        }
        if (gimbal_labels[i]) {
            snprintf(buf, sizeof(buf), "%d", values[i]);
            lv_label_set_text(gimbal_labels[i], buf);
        }
    }
    
    // If calibration is active, update the main display for current axis
    if (gimbal_cal_active && gimbal_cal_axis < 4) {
        gimbal_cal_current_value = values[gimbal_cal_axis];
        
        if (gimbal_cal_bar) {
            lv_bar_set_value(gimbal_cal_bar, gimbal_cal_current_value, LV_ANIM_OFF);
        }
        if (gimbal_cal_value_label) {
            snprintf(buf, sizeof(buf), "%d", gimbal_cal_current_value);
            lv_label_set_text(gimbal_cal_value_label, buf);
        }
    }
}

bool ui_gimbal_cal_is_active(void) {
    return gimbal_cal_active;
}

uint8_t ui_gimbal_cal_get_axis(void) {
    return gimbal_cal_axis;
}
