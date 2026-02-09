#include "ui_screen_settings.h"
#include "../ui_styles.h"
#include "../ui_helpers.h"
#include "../../Settings.h"
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
static int16_t touch_cal_raw[4][2] = {{0}}; // 4 points, raw x/y

// Gimbal calibration UI
static lv_obj_t *gimbal_bars[4] = {NULL};
static lv_obj_t *gimbal_labels[4] = {NULL};

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

static void robot_select_cb(lv_event_t *e) {
    intptr_t profile = (intptr_t)lv_event_get_user_data(e);
    Settings_SetRobotProfile((RobotProfile_t)profile);
    printf("Settings: Robot profile set to %s\r\n", Settings_GetRobotProfileName((RobotProfile_t)profile));
}

static void touch_cal_start_cb(lv_event_t *e) {
    (void)e;
    ui_touch_cal_start();
}

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
    lv_obj_set_size(back, 50, 22);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(back, &style_card, 0);
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
    lv_obj_set_style_pad_all(menu_main, 4, 0);
    lv_obj_remove_flag(menu_main, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *title = lv_label_create(menu_main);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    
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
        lv_obj_set_size(btn, lv_pct(95), 26);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 24 + i * 30);
        lv_obj_add_style(btn, &style_card, 0);
        lv_obj_add_event_cb(btn, menu_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)items[i].menu);
        
        lv_obj_t *lbl = lv_label_create(btn);
        char buf[48];
        snprintf(buf, sizeof(buf), "%s  %s", items[i].icon, items[i].label);
        lv_label_set_text(lbl, buf);
        lv_obj_add_style(lbl, &style_text_primary, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
        
        lv_obj_t *arrow = lv_label_create(btn);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_add_style(arrow, &style_text_secondary, 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -4, 0);
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
    const char *descs[] = {"Standard RC channels", "6-leg walking robot"};
    
    for (int i = 0; i < ROBOT_PROFILE_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(menu_robot);
        lv_obj_set_size(btn, lv_pct(95), 38);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 30 + i * 44);
        lv_obj_add_style(btn, &style_card, 0);
        lv_obj_add_event_cb(btn, robot_select_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        
        lv_obj_t *name = lv_label_create(btn);
        lv_label_set_text(name, profiles[i]);
        lv_obj_add_style(name, &style_text_primary, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_12, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 8, 4);
        
        lv_obj_t *desc = lv_label_create(btn);
        lv_label_set_text(desc, descs[i]);
        lv_obj_add_style(desc, &style_text_secondary, 0);
        lv_obj_set_style_text_font(desc, &lv_font_montserrat_10, 0);
        lv_obj_align(desc, LV_ALIGN_BOTTOM_LEFT, 8, -4);
    }
}

static void create_touch_cal_menu(lv_obj_t *parent) {
    menu_touch_cal = lv_obj_create(parent);
    lv_obj_set_size(menu_touch_cal, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_touch_cal, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_touch_cal, 0, 0);
    lv_obj_set_style_pad_all(menu_touch_cal, 4, 0);
    lv_obj_add_flag(menu_touch_cal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(menu_touch_cal, LV_OBJ_FLAG_SCROLLABLE);
    
    create_back_header(menu_touch_cal, "Touch Calibration");
    
    touch_cal_label = lv_label_create(menu_touch_cal);
    lv_label_set_text(touch_cal_label, "Press Start, then tap the\ncrosshairs that appear.");
    lv_obj_add_style(touch_cal_label, &style_text_secondary, 0);
    lv_obj_set_style_text_align(touch_cal_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(touch_cal_label, LV_ALIGN_CENTER, 0, -15);
    
    lv_obj_t *start_btn = lv_button_create(menu_touch_cal);
    lv_obj_set_size(start_btn, 100, 28);
    lv_obj_align(start_btn, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_add_event_cb(start_btn, touch_cal_start_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *start_lbl = lv_label_create(start_btn);
    lv_label_set_text(start_lbl, LV_SYMBOL_PLAY " Start");
    lv_obj_center(start_lbl);
    
    // Calibration target crosshair (hidden until calibration starts)
    touch_cal_target = lv_obj_create(menu_touch_cal);
    lv_obj_set_size(touch_cal_target, 20, 20);
    lv_obj_set_style_bg_opa(touch_cal_target, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(touch_cal_target, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(touch_cal_target, 2, 0);
    lv_obj_set_style_radius(touch_cal_target, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
}

static void create_gimbal_cal_menu(lv_obj_t *parent) {
    menu_gimbal_cal = lv_obj_create(parent);
    lv_obj_set_size(menu_gimbal_cal, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(menu_gimbal_cal, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_gimbal_cal, 0, 0);
    lv_obj_set_style_pad_all(menu_gimbal_cal, 4, 0);
    lv_obj_add_flag(menu_gimbal_cal, LV_OBJ_FLAG_HIDDEN);
    
    create_back_header(menu_gimbal_cal, "Gimbal Calibration");
    
    const char *names[] = {"Left X", "Left Y", "Right X", "Right Y"};
    
    lv_obj_t *info = lv_label_create(menu_gimbal_cal);
    lv_label_set_text(info, "Move sticks to full range:");
    lv_obj_add_style(info, &style_text_secondary, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 28);
    
    for (int i = 0; i < 4; i++) {
        int y = 46 + i * 24;
        
        lv_obj_t *name = lv_label_create(menu_gimbal_cal);
        lv_label_set_text(name, names[i]);
        lv_obj_add_style(name, &style_text_secondary, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 4, y);
        
        gimbal_bars[i] = lv_bar_create(menu_gimbal_cal);
        lv_obj_set_size(gimbal_bars[i], 140, 12);
        lv_obj_align(gimbal_bars[i], LV_ALIGN_TOP_MID, 10, y);
        lv_bar_set_range(gimbal_bars[i], 0, 32767);
        lv_bar_set_value(gimbal_bars[i], 16383, LV_ANIM_OFF);
        
        gimbal_labels[i] = lv_label_create(menu_gimbal_cal);
        lv_label_set_text(gimbal_labels[i], "16383");
        lv_obj_add_style(gimbal_labels[i], &style_text_small, 0);
        lv_obj_align(gimbal_labels[i], LV_ALIGN_TOP_RIGHT, -4, y);
    }
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
    ui_SettingsScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_SettingsScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_SettingsScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_SettingsScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_SettingsScreen, 0, 0);
    lv_obj_remove_flag(ui_SettingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
    
    create_main_menu(ui_SettingsScreen);
    create_radio_menu(ui_SettingsScreen);
    create_robot_menu(ui_SettingsScreen);
    create_touch_cal_menu(ui_SettingsScreen);
    create_gimbal_cal_menu(ui_SettingsScreen);
    create_about_menu(ui_SettingsScreen);
    
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
    
    // Show first target (top-left)
    lv_obj_remove_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(touch_cal_target, 20, 30);
    lv_label_set_text(touch_cal_label, "Tap point 1 of 4\n(top-left)");
}

void ui_touch_cal_record_point(int16_t raw_x, int16_t raw_y) {
    if (!touch_cal_active || touch_cal_point >= 4) return;
    
    touch_cal_raw[touch_cal_point][0] = raw_x;
    touch_cal_raw[touch_cal_point][1] = raw_y;
    touch_cal_point++;
    
    // Position targets: TL, TR, BR, BL
    int16_t positions[4][2] = {
        {20, 30},                               // Top-left
        {UI_SCREEN_WIDTH - 40, 30},             // Top-right
        {UI_SCREEN_WIDTH - 40, UI_CONTENT_HEIGHT - 20}, // Bottom-right
        {20, UI_CONTENT_HEIGHT - 20}            // Bottom-left
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
    } else {
        // Calibration complete
        touch_cal_active = false;
        lv_obj_add_flag(touch_cal_target, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(touch_cal_label, "Calibration complete!\nData saved.");
        
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

void ui_gimbal_cal_update(int16_t values[4]) {
    char buf[16];
    for (int i = 0; i < 4; i++) {
        if (gimbal_bars[i]) {
            lv_bar_set_value(gimbal_bars[i], values[i], LV_ANIM_OFF);
        }
        if (gimbal_labels[i]) {
            snprintf(buf, sizeof(buf), "%d", values[i]);
            lv_label_set_text(gimbal_labels[i], buf);
        }
    }
}
