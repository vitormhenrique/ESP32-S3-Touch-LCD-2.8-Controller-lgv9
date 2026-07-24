#include "ui_screen_telemetry.h"
#include "../ui_styles.h"
#include "../ui_helpers.h"
#include "../../Settings.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *ui_TelemetryScreen = NULL;

//=============================================================================
// Panel Definitions
//=============================================================================

#define MAX_PANELS 6
#define MAX_LOG_LINES 12

// Panel containers
static lv_obj_t *panel_container = NULL;
static lv_obj_t *panels[MAX_PANELS] = {NULL};
static uint8_t num_panels = 0;
static uint8_t current_panel = 0;

// Page indicator dots
static lv_obj_t *page_indicator = NULL;
static lv_obj_t *page_dots[MAX_PANELS] = {NULL};

// Status panel elements
static lv_obj_t *status_rssi_val = NULL;
static lv_obj_t *status_latency_val = NULL;
static lv_obj_t *status_errors_val = NULL;
static lv_obj_t *status_uptime_val = NULL;
static lv_obj_t *status_battery_val = NULL;
static lv_obj_t *status_link_icon = NULL;

// Log panel elements
static lv_obj_t *log_textarea = NULL;

// 9-DOF IMU panel elements (simplified - 3 labels for display)
static lv_obj_t *imu9_panel = NULL;  // The panel itself (for show/hide)
static lv_obj_t *imu9_accel_label = NULL;
static lv_obj_t *imu9_gyro_label = NULL;
static lv_obj_t *imu9_mag_label = NULL;

// Hexapod profile panel elements
static lv_obj_t *hex_mode_label = NULL;
static lv_obj_t *hex_gait_label = NULL;
static lv_obj_t *hex_control_label = NULL;
static lv_obj_t *hex_motion_label = NULL;
static lv_obj_t *hex_shape_label = NULL;
static lv_obj_t *hex_timing_label = NULL;
static lv_obj_t *hex_fault_label = NULL;
static lv_obj_t *hex_freshness_label = NULL;

// Track which profile panels are active
static RobotProfile_t active_profile = ROBOT_PROFILE_GENERIC;

//=============================================================================
// Scroll Handling
//=============================================================================

static void update_page_indicator(void) {
    for (int i = 0; i < num_panels; i++) {
        if (page_dots[i]) {
            if (i == current_panel) {
                // Active dot becomes an accent pill
                lv_obj_set_size(page_dots[i], 16, 6);
                lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
            } else {
                lv_obj_set_size(page_dots[i], 6, 6);
                lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(UI_COLOR_BORDER), 0);
            }
        }
    }
}

static void telemetry_scroll_event_cb(lv_event_t *e) {
    lv_obj_t *cont = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_SCROLL_END) {
        lv_point_t scroll_end;
        lv_obj_get_scroll_end(cont, &scroll_end);
        
        int32_t page_width = UI_SCREEN_WIDTH;
        int32_t scroll_x = scroll_end.x;
        
        // Calculate which page we're closest to
        current_panel = (scroll_x + page_width / 2) / page_width;
        if (current_panel >= num_panels) current_panel = num_panels - 1;
        
        // Snap to the target page
        lv_obj_scroll_to_x(cont, current_panel * page_width, LV_ANIM_ON);
        update_page_indicator();
    }
}

//=============================================================================
// Panel Creation Functions
//=============================================================================

static lv_obj_t *create_panel_base(const char *title) {
    // Page wrapper is exactly one screen wide so every page (including the
    // last) snaps perfectly centered
    lv_obj_t *page = lv_obj_create(panel_container);
    lv_obj_set_size(page, UI_SCREEN_WIDTH, lv_pct(100));
    lv_obj_set_pos(page, num_panels * UI_SCREEN_WIDTH, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *panel = lv_obj_create(page);
    lv_obj_set_size(panel, UI_SCREEN_WIDTH - 16, UI_CONTENT_HEIGHT - 24);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 6);  // breathing room below header
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_radius(panel, 12, 0);
    lv_obj_set_style_pad_all(panel, 10, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title - top-left with accent tick
    lv_obj_t *tick = lv_obj_create(panel);
    lv_obj_set_size(tick, 3, 12);
    lv_obj_align(tick, LV_ALIGN_TOP_LEFT, 0, 1);
    lv_obj_set_style_bg_color(tick, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_radius(tick, 2, 0);
    lv_obj_set_style_border_width(tick, 0, 0);
    lv_obj_remove_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *lbl = lv_label_create(panel);
    lv_label_set_text(lbl, title);
    lv_obj_add_style(lbl, &style_text_primary, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 9, 0);
    
    return panel;
}

static void create_status_panel(void) {
    lv_obj_t *panel = create_panel_base("Status");
    panels[num_panels++] = panel;
    
    int y = 26;
    int row_h = 22;
    
    // Connection status - top-right, on the title row
    status_link_icon = lv_label_create(panel);
    lv_label_set_text(status_link_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_link_icon, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_align(status_link_icon, LV_ALIGN_TOP_RIGHT, -70, 0);
    
    lv_obj_t *link_lbl = lv_label_create(panel);
    lv_label_set_text(link_lbl, "Connected");
    lv_obj_add_style(link_lbl, &style_text_primary, 0);
    lv_obj_set_style_text_font(link_lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(link_lbl, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    // RSSI
    lv_obj_t *rssi_lbl = lv_label_create(panel);
    lv_label_set_text(rssi_lbl, "Signal:");
    lv_obj_add_style(rssi_lbl, &style_text_secondary, 0);
    lv_obj_align(rssi_lbl, LV_ALIGN_TOP_LEFT, 0, y);
    
    status_rssi_val = lv_label_create(panel);
    lv_label_set_text(status_rssi_val, "-45 dBm");
    lv_obj_add_style(status_rssi_val, &style_text_primary, 0);
    lv_obj_align(status_rssi_val, LV_ALIGN_TOP_RIGHT, 0, y);
    y += row_h;
    
    // Latency
    lv_obj_t *lat_lbl = lv_label_create(panel);
    lv_label_set_text(lat_lbl, "Latency:");
    lv_obj_add_style(lat_lbl, &style_text_secondary, 0);
    lv_obj_align(lat_lbl, LV_ALIGN_TOP_LEFT, 0, y);
    
    status_latency_val = lv_label_create(panel);
    lv_label_set_text(status_latency_val, "-- ms");
    lv_obj_add_style(status_latency_val, &style_text_primary, 0);
    lv_obj_align(status_latency_val, LV_ALIGN_TOP_RIGHT, 0, y);
    y += row_h;
    
    // Errors
    lv_obj_t *err_lbl = lv_label_create(panel);
    lv_label_set_text(err_lbl, "Errors:");
    lv_obj_add_style(err_lbl, &style_text_secondary, 0);
    lv_obj_align(err_lbl, LV_ALIGN_TOP_LEFT, 0, y);
    
    status_errors_val = lv_label_create(panel);
    lv_label_set_text(status_errors_val, "0");
    lv_obj_add_style(status_errors_val, &style_text_primary, 0);
    lv_obj_align(status_errors_val, LV_ALIGN_TOP_RIGHT, 0, y);
    y += row_h;
    
    // Uptime
    lv_obj_t *up_lbl = lv_label_create(panel);
    lv_label_set_text(up_lbl, "Uptime:");
    lv_obj_add_style(up_lbl, &style_text_secondary, 0);
    lv_obj_align(up_lbl, LV_ALIGN_TOP_LEFT, 0, y);
    
    status_uptime_val = lv_label_create(panel);
    lv_label_set_text(status_uptime_val, "00:00:00");
    lv_obj_add_style(status_uptime_val, &style_text_primary, 0);
    lv_obj_align(status_uptime_val, LV_ALIGN_TOP_RIGHT, 0, y);
    y += row_h;
    
    // Battery
    lv_obj_t *bat_lbl = lv_label_create(panel);
    lv_label_set_text(bat_lbl, "Robot Bat:");
    lv_obj_add_style(bat_lbl, &style_text_secondary, 0);
    lv_obj_align(bat_lbl, LV_ALIGN_TOP_LEFT, 0, y);
    
    status_battery_val = lv_label_create(panel);
    lv_label_set_text(status_battery_val, "-- V");
    lv_obj_add_style(status_battery_val, &style_text_primary, 0);
    lv_obj_align(status_battery_val, LV_ALIGN_TOP_RIGHT, 0, y);
}

static void create_log_panel(void) {
    lv_obj_t *panel = create_panel_base("Log");
    panels[num_panels++] = panel;
    
    // Create textarea for log
    log_textarea = lv_textarea_create(panel);
    lv_obj_set_size(log_textarea, lv_pct(100), UI_CONTENT_HEIGHT - 68);
    lv_obj_align(log_textarea, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(log_textarea, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_text_color(log_textarea, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_set_style_text_font(log_textarea, &lv_font_montserrat_10, 0);
    lv_obj_set_style_border_width(log_textarea, 1, 0);
    lv_obj_set_style_border_color(log_textarea, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_radius(log_textarea, 4, 0);
    lv_textarea_set_text(log_textarea, "> System ready\n");
    lv_obj_remove_flag(log_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

//=============================================================================
// 9-DOF IMU Panel (Accel + Gyro + Magnetometer)
// Uses the standard panel system - created as part of panels[]
//=============================================================================

static void create_imu9_panel(const char *title) {
    printf("IMU9: Starting panel creation...\r\n");

    lv_obj_t *panel = create_panel_base(title);
    if (!panel) {
        printf("IMU9: ERROR - panel creation failed!\r\n");
        return;
    }
    panels[num_panels++] = panel;
    imu9_panel = panel;
    printf("IMU9: Base panel created, num_panels=%d\r\n", num_panels);

    lv_obj_t *lbl1 = lv_label_create(panel);
    lv_label_set_text(lbl1, "Euler: ---, ---, ---");
    lv_obj_add_style(lbl1, &style_text_primary, 0);
    lv_obj_set_pos(lbl1, 5, 26);

    lv_obj_t *lbl2 = lv_label_create(panel);
    lv_label_set_text(lbl2, "Cal:   ---, ---, ---");
    lv_obj_add_style(lbl2, &style_text_primary, 0);
    lv_obj_set_pos(lbl2, 5, 48);

    lv_obj_t *lbl3 = lv_label_create(panel);
    lv_label_set_text(lbl3, "Link:  ---, ---, ---");
    lv_obj_add_style(lbl3, &style_text_primary, 0);
    lv_obj_set_pos(lbl3, 5, 70);

    imu9_accel_label = lbl1;
    imu9_gyro_label = lbl2;
    imu9_mag_label = lbl3;

    printf("IMU9: Panel created successfully\r\n");
}

static lv_obj_t *create_hex_value(lv_obj_t *panel, const char *caption,
                                  int16_t y) {
    lv_obj_t *name = lv_label_create(panel);
    lv_label_set_text(name, caption);
    lv_obj_add_style(name, &style_text_secondary, 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(name, 2, y);

    lv_obj_t *value = lv_label_create(panel);
    lv_label_set_text(value, "--");
    lv_obj_add_style(value, &style_text_primary, 0);
    lv_obj_set_style_text_font(value, &lv_font_montserrat_10, 0);
    lv_obj_align(value, LV_ALIGN_TOP_RIGHT, -2, y);
    return value;
}

static void create_hexapod_panel(void) {
    lv_obj_t *panel = create_panel_base("Hexapod");
    panels[num_panels++] = panel;

    hex_freshness_label = lv_label_create(panel);
    lv_label_set_text(hex_freshness_label, "Waiting");
    lv_obj_set_style_text_font(hex_freshness_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(hex_freshness_label,
                                lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
    lv_obj_align(hex_freshness_label, LV_ALIGN_TOP_RIGHT, -2, 0);

    hex_mode_label = create_hex_value(panel, "Mode", 24);
    hex_gait_label = create_hex_value(panel, "Gait", 40);
    hex_control_label = create_hex_value(panel, "Control", 56);
    hex_motion_label = create_hex_value(panel, "Motion", 72);
    hex_shape_label = create_hex_value(panel, "Geometry", 88);
    hex_timing_label = create_hex_value(panel, "Timing", 104);
    hex_fault_label = create_hex_value(panel, "Fault", 120);
}

static void reset_panel_widgets(void) {
    for (uint8_t i = 0; i < MAX_PANELS; ++i) {
        panels[i] = NULL;
        page_dots[i] = NULL;
    }
    num_panels = 0;
    current_panel = 0;
    status_rssi_val = NULL;
    status_latency_val = NULL;
    status_errors_val = NULL;
    status_uptime_val = NULL;
    status_battery_val = NULL;
    status_link_icon = NULL;
    log_textarea = NULL;
    imu9_panel = NULL;
    imu9_accel_label = NULL;
    imu9_gyro_label = NULL;
    imu9_mag_label = NULL;
    hex_mode_label = NULL;
    hex_gait_label = NULL;
    hex_control_label = NULL;
    hex_motion_label = NULL;
    hex_shape_label = NULL;
    hex_timing_label = NULL;
    hex_fault_label = NULL;
    hex_freshness_label = NULL;
}


//=============================================================================
// Page Indicator Creation
//=============================================================================

static void create_page_indicator(void) {
    page_indicator = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(page_indicator, num_panels * 24, 10);
    lv_obj_align(page_indicator, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_opa(page_indicator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page_indicator, 0, 0);
    lv_obj_set_style_pad_all(page_indicator, 0, 0);
    lv_obj_remove_flag(page_indicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(page_indicator, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page_indicator, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(page_indicator, 5, 0);
    
    for (int i = 0; i < num_panels; i++) {
        page_dots[i] = lv_obj_create(page_indicator);
        lv_obj_set_size(page_dots[i], 6, 6);
        lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(UI_COLOR_BORDER), 0);
        lv_obj_set_style_radius(page_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(page_dots[i], 0, 0);
        lv_obj_remove_flag(page_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    
    update_page_indicator();
}

//=============================================================================
// Public API
//=============================================================================

void ui_create_telemetry_screen(lv_obj_t *parent) {
    printf("Telemetry: Creating screen...\r\n");
    ui_TelemetryScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_TelemetryScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_TelemetryScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_TelemetryScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_TelemetryScreen, 0, 0);
    lv_obj_remove_flag(ui_TelemetryScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
    printf("Telemetry: Screen obj created\r\n");
    
    // Main scrollable container - no padding so pages align exactly to
    // UI_SCREEN_WIDTH multiples (keeps panels centered when snapping)
    panel_container = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(panel_container, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT - 10);
    lv_obj_align(panel_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(panel_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel_container, 0, 0);
    lv_obj_set_style_pad_all(panel_container, 0, 0);
    printf("Telemetry: Panel container created\r\n");
    
    // Enable horizontal scrolling
    lv_obj_add_flag(panel_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(panel_container, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(panel_container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(panel_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(panel_container, telemetry_scroll_event_cb, LV_EVENT_SCROLL_END, NULL);
    printf("Telemetry: Scroll configured\r\n");
    
    // Build panels based on current robot profile
    const Settings_t *s = Settings_Get();
    printf("Telemetry: Building panels for profile %d (%s)\r\n", s->robot_profile, Settings_GetRobotProfileName(s->robot_profile));
    ui_telemetry_refresh_for_profile(s->robot_profile);
    printf("Telemetry: Screen creation complete\r\n");
}

void ui_telemetry_refresh_for_profile(RobotProfile_t profile) {
    printf("Telemetry refresh: Starting for profile %d\r\n", profile);
    
    // Don't do anything if the screen hasn't been created yet
    if (!ui_TelemetryScreen) {
        printf("Telemetry refresh: Skipped - screen not created\r\n");
        return;
    }
    
    if (profile == active_profile && num_panels > 0) {
        return;
    }

    active_profile = profile;
    if (panel_container) {
        lv_obj_clean(panel_container);
    }
    if (page_indicator) {
        lv_obj_delete(page_indicator);
        page_indicator = NULL;
    }
    reset_panel_widgets();

    create_status_panel();
    if (profile == ROBOT_PROFILE_HEXAPOD) {
        create_hexapod_panel();
    }
    create_imu9_panel(profile == ROBOT_PROFILE_HEXAPOD ? "Robot IMU" :
                                                            "Remote IMU");
    create_log_panel();
    create_page_indicator();
    lv_obj_scroll_to_x(panel_container, 0, LV_ANIM_OFF);
    
    printf("Telemetry: Configured for %s profile\r\n", 
           Settings_GetRobotProfileName(profile));
}

void ui_telemetry_update_status(int rssi, int latency, int errors, uint32_t uptime) {
    char buf[32];
    
    if (status_rssi_val) {
        snprintf(buf, sizeof(buf), "%d dBm", rssi);
        lv_label_set_text(status_rssi_val, buf);
        
        // Color code signal strength
        if (rssi > -50) {
            lv_obj_set_style_text_color(status_rssi_val, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        } else if (rssi > -70) {
            lv_obj_set_style_text_color(status_rssi_val, lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
        } else {
            lv_obj_set_style_text_color(status_rssi_val, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
        }
    }
    
    if (status_latency_val) {
        snprintf(buf, sizeof(buf), "%d ms", latency);
        lv_label_set_text(status_latency_val, buf);
    }
    
    if (status_errors_val) {
        snprintf(buf, sizeof(buf), "%d", errors);
        lv_label_set_text(status_errors_val, buf);
        if (errors > 0) {
            lv_obj_set_style_text_color(status_errors_val, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
        } else {
            lv_obj_set_style_text_color(status_errors_val, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        }
    }
    
    if (status_uptime_val) {
        uint32_t hours = uptime / 3600;
        uint32_t mins = (uptime % 3600) / 60;
        uint32_t secs = uptime % 60;
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)hours, (unsigned long)mins, (unsigned long)secs);
        lv_label_set_text(status_uptime_val, buf);
    }
}

void ui_telemetry_update_battery(float voltage, bool valid) {
    if (!status_battery_val) return;
    char buf[20];
    if (!valid) {
        lv_label_set_text(status_battery_val, "-- V");
        lv_obj_set_style_text_color(status_battery_val,
                                    lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
        return;
    }
    snprintf(buf, sizeof(buf), "%.1f V", (double)voltage);
    lv_label_set_text(status_battery_val, buf);
    lv_obj_set_style_text_color(status_battery_val,
                                voltage >= 10.5f
                                    ? lv_color_hex(UI_COLOR_ACCENT_GREEN)
                                    : lv_color_hex(UI_COLOR_ACCENT_RED), 0);
}

void ui_telemetry_add_log(const char *message) {
    if (!log_textarea) return;
    
    // Get current text length to check if we need to trim
    const char *current = lv_textarea_get_text(log_textarea);
    int lines = 0;
    const char *p = current;
    while (*p) {
        if (*p == '\n') lines++;
        p++;
    }
    
    // If too many lines, clear and start fresh
    if (lines > MAX_LOG_LINES) {
        lv_textarea_set_text(log_textarea, "");
    }
    
    // Add timestamp and message
    char buf[128];
    snprintf(buf, sizeof(buf), "> %s\n", message);
    lv_textarea_add_text(log_textarea, buf);
    
    // Scroll to bottom
    lv_textarea_set_cursor_pos(log_textarea, LV_TEXTAREA_CURSOR_LAST);
}

void ui_telemetry_update_imu9(float ax, float ay, float az,
                              float gx, float gy, float gz,
                              float mx, float my, float mz) {
    char buf[48];

    // Row 1: BNO055 Euler angles (pitch, roll, yaw)
    if (imu9_accel_label) {
        snprintf(buf, sizeof(buf), "Euler: P%.1f R%.1f Y%.1f",
                 (double)ax, (double)ay, (double)az);
        lv_label_set_text(imu9_accel_label, buf);
    }

    // Row 2: BNO055 calibration (sys, gyro, accel)
    if (imu9_gyro_label) {
        snprintf(buf, sizeof(buf), "Cal:   S%.0f G%.0f A%.0f",
                 (double)gx, (double)gy, (double)gz);
        lv_label_set_text(imu9_gyro_label, buf);
    }

    // Row 3: Link info (voltage, LQ%, SNR)
    if (imu9_mag_label) {
        snprintf(buf, sizeof(buf), "Link:  %.1fV LQ%.0f%% SNR%.0f",
                 (double)mx, (double)my, (double)mz);
        lv_label_set_text(imu9_mag_label, buf);
    }
}

void ui_telemetry_set_imu_state(bool present, bool fresh) {
    if (!imu9_accel_label) return;
    if (fresh) return;
    const char *missing = active_profile == ROBOT_PROFILE_HEXAPOD
                              ? "No robot IMU detected"
                              : "No IMU telemetry";
    lv_label_set_text(imu9_accel_label, present ? "IMU data stale" : missing);
    if (imu9_gyro_label) lv_label_set_text(imu9_gyro_label, "Cal:   -- -- --");
    if (imu9_mag_label) lv_label_set_text(imu9_mag_label, "Link:  waiting");
}

void ui_telemetry_update_hexapod(const HexapodTelemetryStatus *status,
                                 bool fresh, uint32_t age_ms) {
    if (active_profile != ROBOT_PROFILE_HEXAPOD || !hex_freshness_label) {
        return;
    }

    char buf[64];
    if (!status || !fresh) {
        lv_label_set_text(hex_freshness_label, status ? "Stale" : "Waiting");
        lv_obj_set_style_text_color(hex_freshness_label,
                                    lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
        lv_label_set_text(hex_mode_label, "--");
        lv_label_set_text(hex_gait_label, "--");
        lv_label_set_text(hex_control_label, "--");
        lv_label_set_text(hex_motion_label, "--");
        lv_label_set_text(hex_shape_label, "--");
        lv_label_set_text(hex_timing_label, "--");
        lv_label_set_text(hex_fault_label, "--");
        return;
    }

    snprintf(buf, sizeof(buf), "%lums", (unsigned long)age_ms);
    lv_label_set_text(hex_freshness_label, buf);
    lv_obj_set_style_text_color(hex_freshness_label,
                                lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_label_set_text(hex_mode_label,
                      hexapod_safety_state_name(status->safety_state));
    lv_label_set_text(hex_gait_label, hexapod_gait_name(status->gait));
    snprintf(buf, sizeof(buf), "%s / %s",
             hexapod_control_mode_name(status->control_mode),
             hexapod_command_source_name(status->command_source));
    lv_label_set_text(hex_control_label, buf);
    snprintf(buf, sizeof(buf), "%s / %s",
             (status->flags & HEXAPOD_FLAG_ARMED) ? "Armed" : "Disarmed",
             (status->flags & HEXAPOD_FLAG_MOTION_GATE) ? "Active" : "Held");
    lv_label_set_text(hex_motion_label, buf);
    snprintf(buf, sizeof(buf), "H%u S%u Z%u mm", status->body_height_mm,
             status->stride_mm, status->step_height_mm);
    lv_label_set_text(hex_shape_label, buf);
    snprintf(buf, sizeof(buf), "%u%% / duty %u%%",
             (unsigned)((status->speed_x255 * 100u + 127u) / 255u),
             (unsigned)((status->duty_x255 * 100u + 127u) / 255u));
    lv_label_set_text(hex_timing_label, buf);
    if ((status->flags & HEXAPOD_FLAG_FAULT) != 0 || status->fault_reason != 0) {
        lv_label_set_text(hex_fault_label,
                          hexapod_fault_name(status->fault_reason));
        lv_obj_set_style_text_color(hex_fault_label,
                                    lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    } else {
        if ((status->flags & HEXAPOD_FLAG_BATTERY_VALID) != 0) {
            snprintf(buf, sizeof(buf), "None / %.1fV",
                     (double)status->battery_mv / 1000.0);
        } else {
            snprintf(buf, sizeof(buf), "None / --V");
        }
        lv_label_set_text(hex_fault_label, buf);
        lv_obj_set_style_text_color(hex_fault_label,
                                    lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    }
}

uint8_t ui_telemetry_get_panel(void) {
    return current_panel;
}
