/**
 * @file ui_screen_telemetry.c
 * @brief Telemetry Screen Implementation
 */

#include "ui_screen_telemetry.h"
#include <stdio.h>
#include <math.h>

//=============================================================================
// Screen Objects
//=============================================================================

lv_obj_t *ui_TelemetryScreen = NULL;

// Scroll container and pages
static lv_obj_t *ui_TelemetryScroll = NULL;
static lv_obj_t *ui_StatusPage = NULL;
static lv_obj_t *ui_LogPage = NULL;
static lv_obj_t *ui_ServoPage = NULL;
static lv_obj_t *ui_IMUPage = NULL;

// Status panel
static lv_obj_t *ui_StatusLabels[4] = {NULL};
static lv_obj_t *ui_StatusValues[4] = {NULL};

// Log panel
static lv_obj_t *ui_LogTextArea = NULL;

// Servo panel (for hexapod)
static lv_obj_t *ui_ServoLabels[18] = {NULL};
static lv_obj_t *ui_ServoValues[18] = {NULL};

// IMU panel
static lv_obj_t *ui_HorizonCanvas = NULL;
static lv_obj_t *ui_RollLabel = NULL;
static lv_obj_t *ui_PitchLabel = NULL;
static lv_obj_t *ui_YawLabel = NULL;
static lv_obj_t *ui_AccelLabels[3] = {NULL};

// Current robot type
static uint8_t current_robot_type = 0;

#define PAGE_HEIGHT (UI_CONTENT_HEIGHT - 4)

//=============================================================================
// Create Status Page (Page 1 - All robots)
//=============================================================================

static void create_status_page(lv_obj_t *page)
{
    lv_obj_t *panel = ui_create_card(page, UI_SCREEN_WIDTH - 16, PAGE_HEIGHT - 8);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    
    lv_obj_t *title = ui_create_label(panel, "Status");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    const char *labels[] = {"RSSI:", "Latency:", "Errors:", "Uptime:"};
    const char *values[] = {"--dBm", "--ms", "0", "--"};
    
    for (int i = 0; i < 4; i++) {
        ui_StatusLabels[i] = ui_create_label_secondary(panel, labels[i]);
        lv_obj_set_style_text_font(ui_StatusLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_StatusLabels[i], LV_ALIGN_TOP_LEFT, 8, 24 + i * 28);
        
        ui_StatusValues[i] = ui_create_label(panel, values[i]);
        lv_obj_set_style_text_font(ui_StatusValues[i], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_StatusValues[i], LV_ALIGN_TOP_RIGHT, -8, 24 + i * 28);
    }
}

//=============================================================================
// Create Log Page (Page 2 - All robots)
//=============================================================================

static void create_log_page(lv_obj_t *page)
{
    lv_obj_t *panel = ui_create_card(page, UI_SCREEN_WIDTH - 16, PAGE_HEIGHT - 8);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(panel, 4, 0);
    
    lv_obj_t *title = ui_create_label(panel, "Log");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create scrollable text area for logs
    ui_LogTextArea = lv_textarea_create(panel);
    lv_obj_set_size(ui_LogTextArea, UI_SCREEN_WIDTH - 32, PAGE_HEIGHT - 36);
    lv_obj_align(ui_LogTextArea, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_textarea_set_text(ui_LogTextArea, "System ready\n");
    lv_obj_set_style_bg_color(ui_LogTextArea, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_text_color(ui_LogTextArea, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_set_style_text_font(ui_LogTextArea, &lv_font_montserrat_10, 0);
    lv_obj_set_style_border_color(ui_LogTextArea, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_textarea_set_cursor_click_pos(ui_LogTextArea, false);
    lv_obj_remove_flag(ui_LogTextArea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

//=============================================================================
// Create Servo Page (Page 3 - Hexapod only)
//=============================================================================

static void create_servo_page(lv_obj_t *page)
{
    lv_obj_t *panel = ui_create_card(page, UI_SCREEN_WIDTH - 16, PAGE_HEIGHT - 8);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(panel, 4, 0);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *title = ui_create_label(panel, "Dynamixel MX-28AR Servos");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create servo info grid (6 legs x 3 servos = 18 servos)
    const char *leg_names[] = {"L1", "L2", "L3", "R1", "R2", "R3"};
    const char *joint_names[] = {"C", "F", "T"};  // Coxa, Femur, Tibia
    
    for (int leg = 0; leg < 6; leg++) {
        for (int joint = 0; joint < 3; joint++) {
            int idx = leg * 3 + joint;
            int col = leg % 3;
            int row = (leg / 3) * 3 + joint;
            
            char name[8];
            snprintf(name, sizeof(name), "%s%s:", leg_names[leg], joint_names[joint]);
            
            ui_ServoLabels[idx] = ui_create_label_small(panel, name);
            lv_obj_align(ui_ServoLabels[idx], LV_ALIGN_TOP_LEFT, 
                        8 + col * 100, 18 + row * 14);
            
            ui_ServoValues[idx] = ui_create_label_small(panel, "---");
            lv_obj_set_style_text_color(ui_ServoValues[idx], lv_color_hex(UI_COLOR_TEXT_PRIMARY), 0);
            lv_obj_align(ui_ServoValues[idx], LV_ALIGN_TOP_LEFT, 
                        48 + col * 100, 18 + row * 14);
        }
    }
}

//=============================================================================
// Draw Virtual Horizon
//=============================================================================

static void draw_horizon(float roll, float pitch)
{
    if (!ui_HorizonCanvas) return;
    
    // Clear and redraw horizon indicator
    // This is a simplified implementation
    lv_obj_set_style_bg_color(ui_HorizonCanvas, lv_color_hex(UI_COLOR_BG_DARK), 0);
    
    // In a full implementation, you'd draw:
    // - Sky (blue) on top half
    // - Ground (brown) on bottom half
    // - Rotate based on roll
    // - Translate based on pitch
}

//=============================================================================
// Create IMU Page (Page 4 - Hexapod only)
//=============================================================================

static void create_imu_page(lv_obj_t *page)
{
    lv_obj_t *panel = ui_create_card(page, UI_SCREEN_WIDTH - 16, PAGE_HEIGHT - 8);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(panel, 4, 0);
    
    lv_obj_t *title = ui_create_label(panel, "IMU / Orientation");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // Virtual horizon display
    ui_HorizonCanvas = lv_obj_create(panel);
    lv_obj_set_size(ui_HorizonCanvas, 80, 80);
    lv_obj_align(ui_HorizonCanvas, LV_ALIGN_TOP_LEFT, 8, 20);
    lv_obj_set_style_bg_color(ui_HorizonCanvas, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_border_color(ui_HorizonCanvas, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_border_width(ui_HorizonCanvas, 2, 0);
    lv_obj_set_style_radius(ui_HorizonCanvas, LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(ui_HorizonCanvas, LV_OBJ_FLAG_SCROLLABLE);
    
    // Horizon center line
    lv_obj_t *h_line = lv_obj_create(ui_HorizonCanvas);
    lv_obj_set_size(h_line, 60, 2);
    lv_obj_align(h_line, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(h_line, lv_color_hex(UI_COLOR_ACCENT_ORANGE), 0);
    lv_obj_set_style_border_width(h_line, 0, 0);
    lv_obj_remove_flag(h_line, LV_OBJ_FLAG_SCROLLABLE);
    
    // Center indicator
    lv_obj_t *center = lv_obj_create(ui_HorizonCanvas);
    lv_obj_set_size(center, 8, 8);
    lv_obj_align(center, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(center, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(center, 0, 0);
    lv_obj_set_style_radius(center, LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(center, LV_OBJ_FLAG_SCROLLABLE);
    
    // Attitude values
    lv_obj_t *roll_title = ui_create_label_secondary(panel, "Roll:");
    lv_obj_align(roll_title, LV_ALIGN_TOP_LEFT, 100, 24);
    ui_RollLabel = ui_create_label(panel, "0.0°");
    lv_obj_align(ui_RollLabel, LV_ALIGN_TOP_LEFT, 140, 24);
    
    lv_obj_t *pitch_title = ui_create_label_secondary(panel, "Pitch:");
    lv_obj_align(pitch_title, LV_ALIGN_TOP_LEFT, 100, 44);
    ui_PitchLabel = ui_create_label(panel, "0.0°");
    lv_obj_align(ui_PitchLabel, LV_ALIGN_TOP_LEFT, 145, 44);
    
    lv_obj_t *yaw_title = ui_create_label_secondary(panel, "Yaw:");
    lv_obj_align(yaw_title, LV_ALIGN_TOP_LEFT, 100, 64);
    ui_YawLabel = ui_create_label(panel, "0.0°");
    lv_obj_align(ui_YawLabel, LV_ALIGN_TOP_LEFT, 140, 64);
    
    // Accelerometer values
    lv_obj_t *accel_title = ui_create_label_secondary(panel, "Accel (m/s²):");
    lv_obj_align(accel_title, LV_ALIGN_TOP_LEFT, 200, 24);
    
    const char *accel_labels[] = {"X:", "Y:", "Z:"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *lbl = ui_create_label_small(panel, accel_labels[i]);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 200, 44 + i * 18);
        
        ui_AccelLabels[i] = ui_create_label(panel, "0.00");
        lv_obj_set_style_text_font(ui_AccelLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_align(ui_AccelLabels[i], LV_ALIGN_TOP_LEFT, 220, 44 + i * 18);
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void ui_telemetry_screen_create(lv_obj_t *parent)
{
    ui_TelemetryScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_TelemetryScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_TelemetryScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_TelemetryScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_TelemetryScreen, 0, 0);
    lv_obj_remove_flag(ui_TelemetryScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Create horizontal scroll container
    ui_TelemetryScroll = ui_create_scroll_container(ui_TelemetryScreen, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT, 4);
    lv_obj_align(ui_TelemetryScroll, LV_ALIGN_TOP_MID, 0, 0);
    
    // Page 1: Status (all robots)
    ui_StatusPage = lv_obj_create(ui_TelemetryScroll);
    lv_obj_set_size(ui_StatusPage, UI_SCREEN_WIDTH, PAGE_HEIGHT);
    lv_obj_set_pos(ui_StatusPage, 0, 0);
    lv_obj_set_style_bg_opa(ui_StatusPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_StatusPage, 0, 0);
    lv_obj_remove_flag(ui_StatusPage, LV_OBJ_FLAG_SCROLLABLE);
    create_status_page(ui_StatusPage);
    
    // Page 2: Log (all robots)
    ui_LogPage = lv_obj_create(ui_TelemetryScroll);
    lv_obj_set_size(ui_LogPage, UI_SCREEN_WIDTH, PAGE_HEIGHT);
    lv_obj_set_pos(ui_LogPage, UI_SCREEN_WIDTH, 0);
    lv_obj_set_style_bg_opa(ui_LogPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_LogPage, 0, 0);
    lv_obj_remove_flag(ui_LogPage, LV_OBJ_FLAG_SCROLLABLE);
    create_log_page(ui_LogPage);
    
    // Page 3: Servo info (hexapod only)
    ui_ServoPage = lv_obj_create(ui_TelemetryScroll);
    lv_obj_set_size(ui_ServoPage, UI_SCREEN_WIDTH, PAGE_HEIGHT);
    lv_obj_set_pos(ui_ServoPage, UI_SCREEN_WIDTH * 2, 0);
    lv_obj_set_style_bg_opa(ui_ServoPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_ServoPage, 0, 0);
    lv_obj_remove_flag(ui_ServoPage, LV_OBJ_FLAG_SCROLLABLE);
    create_servo_page(ui_ServoPage);
    
    // Page 4: IMU (hexapod only)
    ui_IMUPage = lv_obj_create(ui_TelemetryScroll);
    lv_obj_set_size(ui_IMUPage, UI_SCREEN_WIDTH, PAGE_HEIGHT);
    lv_obj_set_pos(ui_IMUPage, UI_SCREEN_WIDTH * 3, 0);
    lv_obj_set_style_bg_opa(ui_IMUPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_IMUPage, 0, 0);
    lv_obj_remove_flag(ui_IMUPage, LV_OBJ_FLAG_SCROLLABLE);
    create_imu_page(ui_IMUPage);
    
    // Set default robot type
    ui_telemetry_set_robot_type(0);
}

void ui_telemetry_screen_show(bool show)
{
    if (ui_TelemetryScreen) {
        if (show) {
            lv_obj_remove_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_telemetry_set_robot_type(uint8_t robot_type)
{
    current_robot_type = robot_type;
    
    // Show/hide hexapod-specific pages
    if (robot_type == 1) {  // Hexapod
        if (ui_ServoPage) lv_obj_remove_flag(ui_ServoPage, LV_OBJ_FLAG_HIDDEN);
        if (ui_IMUPage) lv_obj_remove_flag(ui_IMUPage, LV_OBJ_FLAG_HIDDEN);
    } else {  // Generic
        if (ui_ServoPage) lv_obj_add_flag(ui_ServoPage, LV_OBJ_FLAG_HIDDEN);
        if (ui_IMUPage) lv_obj_add_flag(ui_IMUPage, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_telemetry_set_status(const char *rssi, const char *latency, const char *errors, const char *uptime)
{
    const char *values[] = {rssi, latency, errors, uptime};
    for (int i = 0; i < 4; i++) {
        if (ui_StatusValues[i] && values[i]) {
            lv_label_set_text(ui_StatusValues[i], values[i]);
        }
    }
}

void ui_telemetry_add_log(const char *message)
{
    if (ui_LogTextArea) {
        lv_textarea_add_text(ui_LogTextArea, message);
        lv_textarea_add_text(ui_LogTextArea, "\n");
        // Auto-scroll to bottom
        lv_textarea_set_cursor_pos(ui_LogTextArea, LV_TEXTAREA_CURSOR_LAST);
    }
}

void ui_telemetry_clear_log(void)
{
    if (ui_LogTextArea) {
        lv_textarea_set_text(ui_LogTextArea, "");
    }
}

void ui_telemetry_set_servo(uint8_t servo_id, int16_t position, int16_t load, uint8_t temp, float voltage)
{
    if (servo_id >= 18 || !ui_ServoValues[servo_id]) return;
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d%%/%d°", position, load / 10, temp);
    lv_label_set_text(ui_ServoValues[servo_id], buf);
    
    // Color code based on temperature
    lv_color_t color;
    if (temp > 60) {
        color = lv_color_hex(UI_COLOR_ACCENT_RED);
    } else if (temp > 45) {
        color = lv_color_hex(UI_COLOR_ACCENT_ORANGE);
    } else {
        color = lv_color_hex(UI_COLOR_TEXT_PRIMARY);
    }
    lv_obj_set_style_text_color(ui_ServoValues[servo_id], color, 0);
}

void ui_telemetry_set_imu(float roll, float pitch, float yaw, float accel_x, float accel_y, float accel_z)
{
    char buf[16];
    
    if (ui_RollLabel) {
        snprintf(buf, sizeof(buf), "%.1f°", roll);
        lv_label_set_text(ui_RollLabel, buf);
    }
    
    if (ui_PitchLabel) {
        snprintf(buf, sizeof(buf), "%.1f°", pitch);
        lv_label_set_text(ui_PitchLabel, buf);
    }
    
    if (ui_YawLabel) {
        snprintf(buf, sizeof(buf), "%.1f°", yaw);
        lv_label_set_text(ui_YawLabel, buf);
    }
    
    float accels[] = {accel_x, accel_y, accel_z};
    for (int i = 0; i < 3; i++) {
        if (ui_AccelLabels[i]) {
            snprintf(buf, sizeof(buf), "%.2f", accels[i]);
            lv_label_set_text(ui_AccelLabels[i], buf);
        }
    }
    
    // Update horizon display
    draw_horizon(roll, pitch);
}
