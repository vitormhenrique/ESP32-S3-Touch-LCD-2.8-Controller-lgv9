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
#define MAX_SERVOS 18

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

// Hexapod servo panel elements (3 pages of 6 servos each)
static lv_obj_t *servo_panels[3] = {NULL};  // 3 panels for 18 servos
static lv_obj_t *servo_pos_vals[MAX_SERVOS] = {NULL};
static lv_obj_t *servo_load_bars[MAX_SERVOS] = {NULL};
static lv_obj_t *servo_temp_vals[MAX_SERVOS] = {NULL};
static lv_obj_t *servo_status_icons[MAX_SERVOS] = {NULL};

// IMU panel elements
static lv_obj_t *imu_roll_val = NULL;
static lv_obj_t *imu_pitch_val = NULL;
static lv_obj_t *imu_yaw_val = NULL;
static lv_obj_t *imu_horizon = NULL;
static lv_obj_t *imu_horizon_line = NULL;
static lv_obj_t *imu_accel_x = NULL;
static lv_obj_t *imu_accel_y = NULL;
static lv_obj_t *imu_accel_z = NULL;

// 9-DOF IMU panel elements
static lv_obj_t *imu9_panel = NULL;  // The panel itself (for show/hide)
static lv_obj_t *imu9_accel_x = NULL;
static lv_obj_t *imu9_accel_y = NULL;
static lv_obj_t *imu9_accel_z = NULL;
static lv_obj_t *imu9_gyro_x = NULL;
static lv_obj_t *imu9_gyro_y = NULL;
static lv_obj_t *imu9_gyro_z = NULL;
static lv_obj_t *imu9_mag_x = NULL;
static lv_obj_t *imu9_mag_y = NULL;
static lv_obj_t *imu9_mag_z = NULL;

// Track which profile panels are active
static RobotProfile_t active_profile = ROBOT_PROFILE_GENERIC;

//=============================================================================
// Scroll Handling
//=============================================================================

static void update_page_indicator(void) {
    for (int i = 0; i < num_panels; i++) {
        if (page_dots[i]) {
            if (i == current_panel) {
                lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
            } else {
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
    lv_obj_t *panel = lv_obj_create(panel_container);
    lv_obj_set_size(panel, UI_SCREEN_WIDTH - 8, UI_CONTENT_HEIGHT - 20);
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 6, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    lv_obj_t *lbl = lv_label_create(panel);
    lv_label_set_text(lbl, title);
    lv_obj_add_style(lbl, &style_text_primary, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);
    
    return panel;
}

static void create_status_panel(void) {
    lv_obj_t *panel = create_panel_base("Status");
    panels[num_panels++] = panel;
    
    int y = 20;
    int row_h = 22;
    
    // Connection status icon
    status_link_icon = lv_label_create(panel);
    lv_label_set_text(status_link_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_link_icon, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_align(status_link_icon, LV_ALIGN_TOP_LEFT, 0, y);
    
    lv_obj_t *link_lbl = lv_label_create(panel);
    lv_label_set_text(link_lbl, "Connected");
    lv_obj_add_style(link_lbl, &style_text_primary, 0);
    lv_obj_align(link_lbl, LV_ALIGN_TOP_LEFT, 22, y);
    y += row_h + 4;
    
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
    lv_obj_set_size(log_textarea, lv_pct(100), UI_CONTENT_HEIGHT - 50);
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
// Hexapod-specific Panels
//=============================================================================

static void create_servo_panel(int page, int start_servo) {
    char title[32];
    snprintf(title, sizeof(title), "Servos %d-%d", start_servo + 1, start_servo + 6);
    
    lv_obj_t *panel = create_panel_base(title);
    panels[num_panels++] = panel;
    servo_panels[page] = panel;
    
    // Leg names for hexapod (FL, ML, RL on left; FR, MR, RR on right)
    // Each leg has 3 servos: Coxa, Femur, Tibia
    static const char *servo_names[18] = {
        "FL-C", "FL-F", "FL-T",  // Front Left
        "ML-C", "ML-F", "ML-T",  // Mid Left
        "RL-C", "RL-F", "RL-T",  // Rear Left
        "FR-C", "FR-F", "FR-T",  // Front Right
        "MR-C", "MR-F", "MR-T",  // Mid Right
        "RR-C", "RR-F", "RR-T"   // Rear Right
    };
    
    // Create 6 servo displays in 2 columns x 3 rows
    for (int i = 0; i < 6; i++) {
        int servo_idx = start_servo + i;
        int col = i % 2;
        int row = i / 2;
        int x = col * 150 + 4;
        int y = 18 + row * 40;
        
        // Container for each servo
        lv_obj_t *servo_cont = lv_obj_create(panel);
        lv_obj_set_size(servo_cont, 142, 36);
        lv_obj_set_pos(servo_cont, x, y);
        lv_obj_set_style_bg_color(servo_cont, lv_color_hex(UI_COLOR_BG_CARD), 0);
        lv_obj_set_style_radius(servo_cont, 4, 0);
        lv_obj_set_style_border_width(servo_cont, 0, 0);
        lv_obj_set_style_pad_all(servo_cont, 2, 0);
        lv_obj_remove_flag(servo_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        // Servo name
        lv_obj_t *name = lv_label_create(servo_cont);
        lv_label_set_text(name, servo_names[servo_idx]);
        lv_obj_add_style(name, &style_text_small, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 2, 0);
        
        // Status icon
        servo_status_icons[servo_idx] = lv_label_create(servo_cont);
        lv_label_set_text(servo_status_icons[servo_idx], LV_SYMBOL_OK);
        lv_obj_set_style_text_color(servo_status_icons[servo_idx], lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        lv_obj_set_style_text_font(servo_status_icons[servo_idx], &lv_font_montserrat_10, 0);
        lv_obj_align(servo_status_icons[servo_idx], LV_ALIGN_TOP_RIGHT, -2, 0);
        
        // Position value
        servo_pos_vals[servo_idx] = lv_label_create(servo_cont);
        lv_label_set_text(servo_pos_vals[servo_idx], "512");
        lv_obj_add_style(servo_pos_vals[servo_idx], &style_text_primary, 0);
        lv_obj_set_style_text_font(servo_pos_vals[servo_idx], &lv_font_montserrat_10, 0);
        lv_obj_align(servo_pos_vals[servo_idx], LV_ALIGN_TOP_LEFT, 40, 0);
        
        // Load bar
        servo_load_bars[servo_idx] = lv_bar_create(servo_cont);
        lv_obj_set_size(servo_load_bars[servo_idx], 100, 6);
        lv_obj_align(servo_load_bars[servo_idx], LV_ALIGN_BOTTOM_LEFT, 2, -4);
        lv_bar_set_range(servo_load_bars[servo_idx], 0, 100);
        lv_bar_set_value(servo_load_bars[servo_idx], 0, LV_ANIM_OFF);
        lv_obj_add_style(servo_load_bars[servo_idx], &style_bar_bg, 0);
        lv_obj_add_style(servo_load_bars[servo_idx], &style_bar_indicator, LV_PART_INDICATOR);
        
        // Temp value
        servo_temp_vals[servo_idx] = lv_label_create(servo_cont);
        lv_label_set_text(servo_temp_vals[servo_idx], "--°");
        lv_obj_add_style(servo_temp_vals[servo_idx], &style_text_small, 0);
        lv_obj_align(servo_temp_vals[servo_idx], LV_ALIGN_BOTTOM_RIGHT, -2, -2);
    }
}

//=============================================================================
// 9-DOF IMU Panel (Accel + Gyro + Magnetometer)
// Uses the standard panel system - created as part of panels[]
//=============================================================================

static void create_imu9_panel(void) {
    printf("IMU9: Starting panel creation...\r\n");
    
    lv_obj_t *panel = create_panel_base("9-DOF IMU");
    if (!panel) {
        printf("IMU9: ERROR - panel creation failed!\r\n");
        return;
    }
    panels[num_panels++] = panel;
    imu9_panel = panel;
    printf("IMU9: Base panel created, num_panels=%d\r\n", num_panels);
    
    // Simple test: just 3 static labels
    lv_obj_t *lbl1 = lv_label_create(panel);
    lv_label_set_text(lbl1, "Accel: 0.00, 0.00, 0.00");
    lv_obj_set_pos(lbl1, 5, 20);
    
    lv_obj_t *lbl2 = lv_label_create(panel);
    lv_label_set_text(lbl2, "Gyro:  0.00, 0.00, 0.00");
    lv_obj_set_pos(lbl2, 5, 40);
    
    lv_obj_t *lbl3 = lv_label_create(panel);
    lv_label_set_text(lbl3, "Mag:   0.00, 0.00, 0.00");
    lv_obj_set_pos(lbl3, 5, 60);
    
    // Store references for updates (reusing existing statics)
    imu9_accel_x = lbl1;
    imu9_gyro_x = lbl2;
    imu9_mag_x = lbl3;
    
    printf("IMU9: Panel created successfully\r\n");
}

static void create_imu_panel(void) {
    lv_obj_t *panel = create_panel_base("IMU / Orientation");
    panels[num_panels++] = panel;
    
    // === LEFT SIDE: Simple Artificial Horizon ===
    imu_horizon = lv_obj_create(panel);
    lv_obj_set_size(imu_horizon, 100, 100);
    lv_obj_align(imu_horizon, LV_ALIGN_LEFT_MID, 10, 5);
    lv_obj_set_style_bg_color(imu_horizon, lv_color_hex(0x0077be), 0);  // Sky blue
    lv_obj_set_style_radius(imu_horizon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(imu_horizon, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_border_width(imu_horizon, 2, 0);
    lv_obj_remove_flag(imu_horizon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(imu_horizon, true, 0);
    
    // Ground half
    lv_obj_t *ground = lv_obj_create(imu_horizon);
    lv_obj_set_size(ground, 100, 50);
    lv_obj_align(ground, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(ground, lv_color_hex(0x8B4513), 0);
    lv_obj_set_style_border_width(ground, 0, 0);
    lv_obj_set_style_radius(ground, 0, 0);
    lv_obj_remove_flag(ground, LV_OBJ_FLAG_SCROLLABLE);
    
    // Center dot
    lv_obj_t *center_dot = lv_obj_create(imu_horizon);
    lv_obj_set_size(center_dot, 8, 8);
    lv_obj_center(center_dot);
    lv_obj_set_style_bg_color(center_dot, lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
    lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_remove_flag(center_dot, LV_OBJ_FLAG_SCROLLABLE);
    
    // === RIGHT SIDE: Attitude Values ===
    int x = 120;
    int y = 18;
    int row_h = 26;
    
    // Roll
    lv_obj_t *roll_lbl = lv_label_create(panel);
    lv_label_set_text(roll_lbl, "Roll:");
    lv_obj_add_style(roll_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(roll_lbl, x, y);
    
    imu_roll_val = lv_label_create(panel);
    lv_label_set_text(imu_roll_val, "0.0°");
    lv_obj_add_style(imu_roll_val, &style_text_primary, 0);
    lv_obj_set_pos(imu_roll_val, x + 60, y);
    y += row_h;
    
    // Pitch
    lv_obj_t *pitch_lbl = lv_label_create(panel);
    lv_label_set_text(pitch_lbl, "Pitch:");
    lv_obj_add_style(pitch_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(pitch_lbl, x, y);
    
    imu_pitch_val = lv_label_create(panel);
    lv_label_set_text(imu_pitch_val, "0.0°");
    lv_obj_add_style(imu_pitch_val, &style_text_primary, 0);
    lv_obj_set_pos(imu_pitch_val, x + 60, y);
    y += row_h;
    
    // Yaw
    lv_obj_t *yaw_lbl = lv_label_create(panel);
    lv_label_set_text(yaw_lbl, "Yaw:");
    lv_obj_add_style(yaw_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(yaw_lbl, x, y);
    
    imu_yaw_val = lv_label_create(panel);
    lv_label_set_text(imu_yaw_val, "0.0°");
    lv_obj_add_style(imu_yaw_val, &style_text_primary, 0);
    lv_obj_set_pos(imu_yaw_val, x + 60, y);
    y += row_h + 10;
    
    // Accelerometer
    lv_obj_t *accel_lbl = lv_label_create(panel);
    lv_label_set_text(accel_lbl, "Accel:");
    lv_obj_add_style(accel_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(accel_lbl, x, y);
    y += 18;
    
    imu_accel_x = lv_label_create(panel);
    lv_label_set_text(imu_accel_x, "X:0.00");
    lv_obj_add_style(imu_accel_x, &style_text_small, 0);
    lv_obj_set_pos(imu_accel_x, x, y);
    
    imu_accel_y = lv_label_create(panel);
    lv_label_set_text(imu_accel_y, "Y:0.00");
    lv_obj_add_style(imu_accel_y, &style_text_small, 0);
    lv_obj_set_pos(imu_accel_y, x + 55, y);
    
    imu_accel_z = lv_label_create(panel);
    lv_label_set_text(imu_accel_z, "Z:1.00");
    lv_obj_add_style(imu_accel_z, &style_text_small, 0);
    lv_obj_set_pos(imu_accel_z, x + 110, y);
    
    // Set horizon line to NULL since we removed it
    imu_horizon_line = NULL;
}

static void create_hexapod_gait_panel(void) {
    lv_obj_t *panel = create_panel_base("Gait Control");
    panels[num_panels++] = panel;
    
    // === LEFT SIDE: Simple Hexapod View ===
    lv_obj_t *hex_view = lv_obj_create(panel);
    lv_obj_set_size(hex_view, 120, 150);
    lv_obj_align(hex_view, LV_ALIGN_LEFT_MID, 5, 5);
    lv_obj_set_style_bg_color(hex_view, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_set_style_radius(hex_view, 6, 0);
    lv_obj_set_style_border_width(hex_view, 1, 0);
    lv_obj_set_style_border_color(hex_view, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_remove_flag(hex_view, LV_OBJ_FLAG_SCROLLABLE);
    
    // Body - centered rectangle
    lv_obj_t *body = lv_obj_create(hex_view);
    lv_obj_set_size(body, 40, 70);
    lv_obj_center(body);
    lv_obj_set_style_bg_color(body, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_radius(body, 8, 0);
    lv_obj_set_style_border_color(body, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_border_width(body, 2, 0);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    
    // Direction indicator
    lv_obj_t *dir_arrow = lv_label_create(body);
    lv_label_set_text(dir_arrow, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(dir_arrow, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_align(dir_arrow, LV_ALIGN_TOP_MID, 0, 2);
    
    // Leg dots - simplified as colored dots
    static const int leg_x[] = {10, 10, 10, 90, 90, 90};
    static const int leg_y[] = {25, 65, 105, 25, 65, 105};
    static const char* leg_names[] = {"FL", "ML", "RL", "FR", "MR", "RR"};
    
    for (int i = 0; i < 6; i++) {
        lv_obj_t *leg = lv_obj_create(hex_view);
        lv_obj_set_size(leg, 20, 16);
        lv_obj_set_pos(leg, leg_x[i], leg_y[i]);
        lv_obj_set_style_bg_color(leg, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        lv_obj_set_style_radius(leg, 4, 0);
        lv_obj_set_style_border_width(leg, 0, 0);
        lv_obj_remove_flag(leg, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t *lbl = lv_label_create(leg);
        lv_label_set_text(lbl, leg_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);
        lv_obj_center(lbl);
    }
    
    // === RIGHT SIDE: Gait Parameters (simplified) ===
    int x = 135;
    int y = 14;
    int row_h = 30;
    
    // Gait Mode
    lv_obj_t *gait_lbl = lv_label_create(panel);
    lv_label_set_text(gait_lbl, "Mode:");
    lv_obj_add_style(gait_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(gait_lbl, x, y);
    
    lv_obj_t *gait_val = lv_label_create(panel);
    lv_label_set_text(gait_val, "Tripod");
    lv_obj_add_style(gait_val, &style_text_primary, 0);
    lv_obj_set_pos(gait_val, x + 65, y);
    y += row_h;
    
    // Speed
    lv_obj_t *speed_lbl = lv_label_create(panel);
    lv_label_set_text(speed_lbl, "Speed:");
    lv_obj_add_style(speed_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(speed_lbl, x, y);
    
    lv_obj_t *speed_val = lv_label_create(panel);
    lv_label_set_text(speed_val, "50%");
    lv_obj_add_style(speed_val, &style_text_primary, 0);
    lv_obj_set_pos(speed_val, x + 65, y);
    y += row_h;
    
    // Height  
    lv_obj_t *height_lbl = lv_label_create(panel);
    lv_label_set_text(height_lbl, "Height:");
    lv_obj_add_style(height_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(height_lbl, x, y);
    
    lv_obj_t *height_val = lv_label_create(panel);
    lv_label_set_text(height_val, "80mm");
    lv_obj_add_style(height_val, &style_text_primary, 0);
    lv_obj_set_pos(height_val, x + 65, y);
    y += row_h;
    
    // Stability
    lv_obj_t *stab_lbl = lv_label_create(panel);
    lv_label_set_text(stab_lbl, "Stability:");
    lv_obj_add_style(stab_lbl, &style_text_secondary, 0);
    lv_obj_set_pos(stab_lbl, x, y);
    
    lv_obj_t *stab_val = lv_label_create(panel);
    lv_label_set_text(stab_val, "6/6 " LV_SYMBOL_OK);
    lv_obj_set_style_text_color(stab_val, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_set_pos(stab_val, x + 65, y);
}

//=============================================================================
// Page Indicator Creation
//=============================================================================

static void create_page_indicator(void) {
    page_indicator = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(page_indicator, num_panels * 12, 8);
    lv_obj_align(page_indicator, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_opa(page_indicator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page_indicator, 0, 0);
    lv_obj_set_style_pad_all(page_indicator, 0, 0);
    lv_obj_remove_flag(page_indicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(page_indicator, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page_indicator, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
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
    
    // Main scrollable container
    panel_container = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(panel_container, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT - 10);
    lv_obj_align(panel_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(panel_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel_container, 0, 0);
    lv_obj_set_style_pad_all(panel_container, 4, 0);
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
    
    active_profile = profile;
    
    // Only rebuild panels if they haven't been created yet
    // (first time initialization)
    if (num_panels == 0 && panel_container) {
        printf("Telemetry refresh: Creating common panels...\r\n");
        
        // Reset element pointers
        status_rssi_val = NULL;
        status_latency_val = NULL;
        status_errors_val = NULL;
        status_uptime_val = NULL;
        status_battery_val = NULL;
        status_link_icon = NULL;
        log_textarea = NULL;
        memset(servo_panels, 0, sizeof(servo_panels));
        memset(servo_pos_vals, 0, sizeof(servo_pos_vals));
        memset(servo_load_bars, 0, sizeof(servo_load_bars));
        memset(servo_temp_vals, 0, sizeof(servo_temp_vals));
        memset(servo_status_icons, 0, sizeof(servo_status_icons));
        imu_roll_val = NULL;
        imu_pitch_val = NULL;
        imu_yaw_val = NULL;
        imu_horizon = NULL;
        imu_horizon_line = NULL;
        imu_accel_x = NULL;
        imu_accel_y = NULL;
        imu_accel_z = NULL;
        
        // Common panels for all profiles
        create_status_panel();
        create_log_panel();
        
        // Always create IMU9 panel (will be shown/hidden based on profile)
        create_imu9_panel();
        
        // Position panels (initially position all)
        for (int i = 0; i < num_panels; i++) {
            if (panels[i]) {
                lv_obj_set_pos(panels[i], i * UI_SCREEN_WIDTH + 4, 0);
            }
        }
        
        // Create page indicator
        create_page_indicator();
    }
    
    // Show/hide IMU9 panel based on profile
    if (imu9_panel) {
        if (profile == ROBOT_PROFILE_HEXAPOD) {
            lv_obj_remove_flag(imu9_panel, LV_OBJ_FLAG_HIDDEN);
            printf("Telemetry: IMU9 panel shown\r\n");
        } else {
            lv_obj_add_flag(imu9_panel, LV_OBJ_FLAG_HIDDEN);
            printf("Telemetry: IMU9 panel hidden\r\n");
        }
    }
    
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

void ui_telemetry_update_servo(uint8_t index, int16_t position, int16_t load, uint8_t temp, bool error) {
    if (index >= MAX_SERVOS) return;
    
    char buf[16];
    
    if (servo_pos_vals[index]) {
        snprintf(buf, sizeof(buf), "%d", position);
        lv_label_set_text(servo_pos_vals[index], buf);
    }
    
    if (servo_load_bars[index]) {
        int load_pct = (load * 100) / 1023;  // Assuming 10-bit load value
        lv_bar_set_value(servo_load_bars[index], load_pct, LV_ANIM_OFF);
        
        // Color code load
        if (load_pct > 80) {
            lv_obj_set_style_bg_color(servo_load_bars[index], lv_color_hex(UI_COLOR_ACCENT_RED), LV_PART_INDICATOR);
        } else if (load_pct > 50) {
            lv_obj_set_style_bg_color(servo_load_bars[index], lv_color_hex(UI_COLOR_ACCENT_ORANGE), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(servo_load_bars[index], lv_color_hex(UI_COLOR_ACCENT_GREEN), LV_PART_INDICATOR);
        }
    }
    
    if (servo_temp_vals[index]) {
        snprintf(buf, sizeof(buf), "%d°", temp);
        lv_label_set_text(servo_temp_vals[index], buf);
        
        // Color code temp
        if (temp > 60) {
            lv_obj_set_style_text_color(servo_temp_vals[index], lv_color_hex(UI_COLOR_ACCENT_RED), 0);
        } else if (temp > 45) {
            lv_obj_set_style_text_color(servo_temp_vals[index], lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
        } else {
            lv_obj_set_style_text_color(servo_temp_vals[index], lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
        }
    }
    
    if (servo_status_icons[index]) {
        if (error) {
            lv_label_set_text(servo_status_icons[index], LV_SYMBOL_WARNING);
            lv_obj_set_style_text_color(servo_status_icons[index], lv_color_hex(UI_COLOR_ACCENT_RED), 0);
        } else {
            lv_label_set_text(servo_status_icons[index], LV_SYMBOL_OK);
            lv_obj_set_style_text_color(servo_status_icons[index], lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        }
    }
}

void ui_telemetry_update_imu(float roll, float pitch, float yaw, float ax, float ay, float az) {
    char buf[32];
    
    if (imu_roll_val) {
        snprintf(buf, sizeof(buf), "%.1f°", (double)roll);
        lv_label_set_text(imu_roll_val, buf);
    }
    
    if (imu_pitch_val) {
        snprintf(buf, sizeof(buf), "%.1f°", (double)pitch);
        lv_label_set_text(imu_pitch_val, buf);
    }
    
    if (imu_yaw_val) {
        snprintf(buf, sizeof(buf), "%.1f°", (double)yaw);
        lv_label_set_text(imu_yaw_val, buf);
    }
    
    if (imu_accel_x) {
        snprintf(buf, sizeof(buf), "X:%.2f", (double)ax);
        lv_label_set_text(imu_accel_x, buf);
    }
    
    if (imu_accel_y) {
        snprintf(buf, sizeof(buf), "Y:%.2f", (double)ay);
        lv_label_set_text(imu_accel_y, buf);
    }
    
    if (imu_accel_z) {
        snprintf(buf, sizeof(buf), "Z:%.2f", (double)az);
        lv_label_set_text(imu_accel_z, buf);
    }
}

void ui_telemetry_update_imu9(float ax, float ay, float az, 
                              float gx, float gy, float gz,
                              float mx, float my, float mz) {
    char buf[16];
    
    // Update Accelerometer values
    if (imu9_accel_x) {
        snprintf(buf, sizeof(buf), "%.3f", (double)ax);
        lv_label_set_text(imu9_accel_x, buf);
    }
    if (imu9_accel_y) {
        snprintf(buf, sizeof(buf), "%.3f", (double)ay);
        lv_label_set_text(imu9_accel_y, buf);
    }
    if (imu9_accel_z) {
        snprintf(buf, sizeof(buf), "%.3f", (double)az);
        lv_label_set_text(imu9_accel_z, buf);
    }
    
    // Update Gyroscope values
    if (imu9_gyro_x) {
        snprintf(buf, sizeof(buf), "%.2f", (double)gx);
        lv_label_set_text(imu9_gyro_x, buf);
    }
    if (imu9_gyro_y) {
        snprintf(buf, sizeof(buf), "%.2f", (double)gy);
        lv_label_set_text(imu9_gyro_y, buf);
    }
    if (imu9_gyro_z) {
        snprintf(buf, sizeof(buf), "%.2f", (double)gz);
        lv_label_set_text(imu9_gyro_z, buf);
    }
    
    // Update Magnetometer values
    if (imu9_mag_x) {
        snprintf(buf, sizeof(buf), "%.2f", (double)mx);
        lv_label_set_text(imu9_mag_x, buf);
    }
    if (imu9_mag_y) {
        snprintf(buf, sizeof(buf), "%.2f", (double)my);
        lv_label_set_text(imu9_mag_y, buf);
    }
    if (imu9_mag_z) {
        snprintf(buf, sizeof(buf), "%.2f", (double)mz);
        lv_label_set_text(imu9_mag_z, buf);
    }
}

uint8_t ui_telemetry_get_panel(void) {
    return current_panel;
}
