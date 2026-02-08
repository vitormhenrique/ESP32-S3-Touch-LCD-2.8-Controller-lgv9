/**
 * @file ui_screen_calibration.c
 * @brief Calibration Screens Implementation
 */

#include "ui_screen_calibration.h"
#include <stdio.h>

//=============================================================================
// Touch Calibration Screen Objects
//=============================================================================

lv_obj_t *ui_TouchCalibScreen = NULL;

static lv_obj_t *ui_TouchCalibPoint = NULL;
static lv_obj_t *ui_TouchCalibInstructions = NULL;
static lv_obj_t *ui_TouchCalibProgress = NULL;

static TouchCalibPoint_t current_touch_point = TOUCH_CALIB_POINT_TOP_LEFT;
static touch_calib_point_cb_t touch_point_callback = NULL;
static calib_complete_cb_t touch_complete_callback = NULL;

// Calibration point positions (margin from edge)
#define CALIB_MARGIN 20

static const int16_t calib_points_x[] = {CALIB_MARGIN, UI_SCREEN_WIDTH - CALIB_MARGIN, UI_SCREEN_WIDTH - CALIB_MARGIN, CALIB_MARGIN};
static const int16_t calib_points_y[] = {CALIB_MARGIN, CALIB_MARGIN, UI_SCREEN_HEIGHT - CALIB_MARGIN, UI_SCREEN_HEIGHT - CALIB_MARGIN};
static const char *calib_point_names[] = {"TOP LEFT", "TOP RIGHT", "BOTTOM RIGHT", "BOTTOM LEFT"};

//=============================================================================
// Gimbal Calibration Screen Objects
//=============================================================================

lv_obj_t *ui_GimbalCalibScreen = NULL;

static lv_obj_t *ui_GimbalCalibTitle = NULL;
static lv_obj_t *ui_GimbalCalibInstructions = NULL;
static lv_obj_t *ui_GimbalCalibViz = NULL;
static lv_obj_t *ui_GimbalCalibDot = NULL;
static lv_obj_t *ui_GimbalCalibValueX = NULL;
static lv_obj_t *ui_GimbalCalibValueY = NULL;
static lv_obj_t *ui_GimbalCalibBtnConfirm = NULL;
static lv_obj_t *ui_GimbalCalibBtnCancel = NULL;

static GimbalCalibState_t gimbal_calib_state = GIMBAL_CALIB_IDLE;
static uint8_t current_gimbal_id = 0;
static int16_t calib_center_x, calib_center_y;
static int16_t calib_min_x, calib_min_y, calib_max_x, calib_max_y;

static gimbal_calib_cb_t gimbal_calib_callback = NULL;
static calib_complete_cb_t gimbal_complete_callback = NULL;

//=============================================================================
// Touch Calibration Implementation
//=============================================================================

static void update_touch_calib_point(void)
{
    if (current_touch_point >= TOUCH_CALIB_COMPLETE) {
        // Calibration complete
        if (ui_TouchCalibPoint) lv_obj_add_flag(ui_TouchCalibPoint, LV_OBJ_FLAG_HIDDEN);
        if (ui_TouchCalibInstructions) {
            lv_label_set_text(ui_TouchCalibInstructions, "Calibration Complete!");
            lv_obj_set_style_text_color(ui_TouchCalibInstructions, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
        }
        return;
    }
    
    // Move calibration point
    if (ui_TouchCalibPoint) {
        lv_obj_set_pos(ui_TouchCalibPoint, 
                      calib_points_x[current_touch_point] - 15, 
                      calib_points_y[current_touch_point] - 15);
        lv_obj_remove_flag(ui_TouchCalibPoint, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Update instructions
    if (ui_TouchCalibInstructions) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Touch the point at %s", calib_point_names[current_touch_point]);
        lv_label_set_text(ui_TouchCalibInstructions, buf);
    }
    
    // Update progress
    if (ui_TouchCalibProgress) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Point %d of 4", (int)current_touch_point + 1);
        lv_label_set_text(ui_TouchCalibProgress, buf);
    }
}

void ui_touch_calib_screen_create(lv_obj_t *parent)
{
    ui_TouchCalibScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_TouchCalibScreen, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(ui_TouchCalibScreen, 0, 0);
    lv_obj_set_style_bg_color(ui_TouchCalibScreen, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_border_width(ui_TouchCalibScreen, 0, 0);
    lv_obj_remove_flag(ui_TouchCalibScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_TouchCalibScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Title
    lv_obj_t *title = ui_create_label(ui_TouchCalibScreen, "Touch Screen Calibration");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Instructions
    ui_TouchCalibInstructions = ui_create_label_secondary(ui_TouchCalibScreen, "Touch the calibration point");
    lv_obj_align(ui_TouchCalibInstructions, LV_ALIGN_CENTER, 0, 0);
    
    // Progress
    ui_TouchCalibProgress = ui_create_label_small(ui_TouchCalibScreen, "Point 1 of 4");
    lv_obj_align(ui_TouchCalibProgress, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    // Calibration point indicator (crosshair style)
    ui_TouchCalibPoint = lv_obj_create(ui_TouchCalibScreen);
    lv_obj_set_size(ui_TouchCalibPoint, 30, 30);
    lv_obj_set_style_bg_opa(ui_TouchCalibPoint, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ui_TouchCalibPoint, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(ui_TouchCalibPoint, 2, 0);
    lv_obj_set_style_radius(ui_TouchCalibPoint, LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(ui_TouchCalibPoint, LV_OBJ_FLAG_SCROLLABLE);
    
    // Center dot
    lv_obj_t *dot = lv_obj_create(ui_TouchCalibPoint);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    
    // Crosshair lines
    lv_obj_t *h_line = lv_obj_create(ui_TouchCalibPoint);
    lv_obj_set_size(h_line, 30, 1);
    lv_obj_align(h_line, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(h_line, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(h_line, 0, 0);
    
    lv_obj_t *v_line = lv_obj_create(ui_TouchCalibPoint);
    lv_obj_set_size(v_line, 1, 30);
    lv_obj_align(v_line, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(v_line, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_set_style_border_width(v_line, 0, 0);
}

void ui_touch_calib_screen_show(bool show)
{
    if (ui_TouchCalibScreen) {
        if (show) {
            lv_obj_remove_flag(ui_TouchCalibScreen, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ui_TouchCalibScreen);
        } else {
            lv_obj_add_flag(ui_TouchCalibScreen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_touch_calib_start(void)
{
    current_touch_point = TOUCH_CALIB_POINT_TOP_LEFT;
    update_touch_calib_point();
    ui_touch_calib_screen_show(true);
}

void ui_touch_calib_report_touch(int16_t x, int16_t y)
{
    if (current_touch_point >= TOUCH_CALIB_COMPLETE) return;
    
    // Report the touch point
    if (touch_point_callback) {
        touch_point_callback(current_touch_point, x, y);
    }
    
    // Move to next point
    current_touch_point++;
    update_touch_calib_point();
    
    // Check if complete
    if (current_touch_point >= TOUCH_CALIB_COMPLETE) {
        if (touch_complete_callback) {
            touch_complete_callback(CALIB_TYPE_TOUCH, true);
        }
    }
}

TouchCalibPoint_t ui_touch_calib_get_current_point(void)
{
    return current_touch_point;
}

void ui_touch_calib_set_point_callback(touch_calib_point_cb_t cb)
{
    touch_point_callback = cb;
}

void ui_touch_calib_set_complete_callback(calib_complete_cb_t cb)
{
    touch_complete_callback = cb;
}

//=============================================================================
// Gimbal Calibration Implementation
//=============================================================================

static void gimbal_confirm_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_gimbal_calib_confirm_step();
    }
}

static void gimbal_cancel_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_gimbal_calib_cancel();
    }
}

static void update_gimbal_calib_state(void)
{
    const char *instructions[] = {
        "Release gimbal to center position\nPress Confirm when ready",
        "Move gimbal to all corners\nThen press Confirm",
        "Calibration Complete!"
    };
    
    int state_idx = (gimbal_calib_state == GIMBAL_CALIB_CENTER) ? 0 :
                    (gimbal_calib_state == GIMBAL_CALIB_MIN_MAX) ? 1 : 2;
    
    if (ui_GimbalCalibInstructions) {
        lv_label_set_text(ui_GimbalCalibInstructions, instructions[state_idx]);
    }
    
    if (ui_GimbalCalibTitle) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Calibrate Gimbal %d", current_gimbal_id + 1);
        lv_label_set_text(ui_GimbalCalibTitle, buf);
    }
}

void ui_gimbal_calib_screen_create(lv_obj_t *parent)
{
    ui_GimbalCalibScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_GimbalCalibScreen, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(ui_GimbalCalibScreen, 0, 0);
    lv_obj_set_style_bg_color(ui_GimbalCalibScreen, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_border_width(ui_GimbalCalibScreen, 0, 0);
    lv_obj_remove_flag(ui_GimbalCalibScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_GimbalCalibScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Title
    ui_GimbalCalibTitle = ui_create_label(ui_GimbalCalibScreen, "Gimbal Calibration");
    lv_obj_set_style_text_font(ui_GimbalCalibTitle, &lv_font_montserrat_16, 0);
    lv_obj_align(ui_GimbalCalibTitle, LV_ALIGN_TOP_MID, 0, 8);
    
    // Gimbal visualization box
    ui_GimbalCalibViz = lv_obj_create(ui_GimbalCalibScreen);
    lv_obj_set_size(ui_GimbalCalibViz, 120, 120);
    lv_obj_align(ui_GimbalCalibViz, LV_ALIGN_CENTER, -60, -10);
    lv_obj_set_style_bg_color(ui_GimbalCalibViz, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_border_color(ui_GimbalCalibViz, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_width(ui_GimbalCalibViz, 1, 0);
    lv_obj_set_style_radius(ui_GimbalCalibViz, 8, 0);
    lv_obj_remove_flag(ui_GimbalCalibViz, LV_OBJ_FLAG_SCROLLABLE);
    
    // Cross lines in visualization
    lv_obj_t *h_line = lv_obj_create(ui_GimbalCalibViz);
    lv_obj_set_size(h_line, 100, 1);
    lv_obj_align(h_line, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(h_line, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_width(h_line, 0, 0);
    
    lv_obj_t *v_line = lv_obj_create(ui_GimbalCalibViz);
    lv_obj_set_size(v_line, 1, 100);
    lv_obj_align(v_line, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(v_line, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_width(v_line, 0, 0);
    
    // Position dot
    ui_GimbalCalibDot = lv_obj_create(ui_GimbalCalibViz);
    lv_obj_set_size(ui_GimbalCalibDot, 16, 16);
    lv_obj_align(ui_GimbalCalibDot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ui_GimbalCalibDot, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_border_width(ui_GimbalCalibDot, 0, 0);
    lv_obj_set_style_radius(ui_GimbalCalibDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(ui_GimbalCalibDot, LV_OBJ_FLAG_SCROLLABLE);
    
    // Value labels
    lv_obj_t *x_label = ui_create_label_secondary(ui_GimbalCalibScreen, "X:");
    lv_obj_align(x_label, LV_ALIGN_CENTER, 50, -30);
    
    ui_GimbalCalibValueX = ui_create_label(ui_GimbalCalibScreen, "0");
    lv_obj_align(ui_GimbalCalibValueX, LV_ALIGN_CENTER, 90, -30);
    
    lv_obj_t *y_label = ui_create_label_secondary(ui_GimbalCalibScreen, "Y:");
    lv_obj_align(y_label, LV_ALIGN_CENTER, 50, 0);
    
    ui_GimbalCalibValueY = ui_create_label(ui_GimbalCalibScreen, "0");
    lv_obj_align(ui_GimbalCalibValueY, LV_ALIGN_CENTER, 90, 0);
    
    // Instructions
    ui_GimbalCalibInstructions = ui_create_label_secondary(ui_GimbalCalibScreen, "Release gimbal to center");
    lv_obj_set_style_text_align(ui_GimbalCalibInstructions, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(ui_GimbalCalibInstructions, UI_SCREEN_WIDTH - 20);
    lv_obj_align(ui_GimbalCalibInstructions, LV_ALIGN_BOTTOM_MID, 0, -60);
    
    // Buttons
    ui_GimbalCalibBtnConfirm = ui_create_button(ui_GimbalCalibScreen, "Confirm", 90, 36);
    lv_obj_align(ui_GimbalCalibBtnConfirm, LV_ALIGN_BOTTOM_LEFT, 20, -15);
    lv_obj_add_event_cb(ui_GimbalCalibBtnConfirm, gimbal_confirm_event_cb, LV_EVENT_CLICKED, NULL);
    
    ui_GimbalCalibBtnCancel = ui_create_button(ui_GimbalCalibScreen, "Cancel", 90, 36);
    lv_obj_align(ui_GimbalCalibBtnCancel, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lv_obj_set_style_bg_color(ui_GimbalCalibBtnCancel, lv_color_hex(UI_COLOR_ACCENT_RED), 0);
    lv_obj_add_event_cb(ui_GimbalCalibBtnCancel, gimbal_cancel_event_cb, LV_EVENT_CLICKED, NULL);
}

void ui_gimbal_calib_screen_show(bool show)
{
    if (ui_GimbalCalibScreen) {
        if (show) {
            lv_obj_remove_flag(ui_GimbalCalibScreen, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ui_GimbalCalibScreen);
        } else {
            lv_obj_add_flag(ui_GimbalCalibScreen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_gimbal_calib_start(uint8_t gimbal_id)
{
    current_gimbal_id = gimbal_id;
    gimbal_calib_state = GIMBAL_CALIB_CENTER;
    
    // Reset calibration values
    calib_min_x = 32767;
    calib_min_y = 32767;
    calib_max_x = -32768;
    calib_max_y = -32768;
    calib_center_x = 0;
    calib_center_y = 0;
    
    update_gimbal_calib_state();
    ui_gimbal_calib_screen_show(true);
}

void ui_gimbal_calib_update(uint8_t gimbal_id, int16_t raw_x, int16_t raw_y)
{
    if (gimbal_id != current_gimbal_id) return;
    
    // Update display values
    if (ui_GimbalCalibValueX) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", raw_x);
        lv_label_set_text(ui_GimbalCalibValueX, buf);
    }
    
    if (ui_GimbalCalibValueY) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", raw_y);
        lv_label_set_text(ui_GimbalCalibValueY, buf);
    }
    
    // Update visualization dot position
    // Map raw values (assumed -32768 to 32767) to visualization area (-50 to 50)
    if (ui_GimbalCalibDot) {
        int dot_x = (raw_x * 50) / 32767;
        int dot_y = (raw_y * 50) / 32767;
        lv_obj_align(ui_GimbalCalibDot, LV_ALIGN_CENTER, dot_x, dot_y);
    }
    
    // Track min/max during MIN_MAX state
    if (gimbal_calib_state == GIMBAL_CALIB_MIN_MAX) {
        if (raw_x < calib_min_x) calib_min_x = raw_x;
        if (raw_x > calib_max_x) calib_max_x = raw_x;
        if (raw_y < calib_min_y) calib_min_y = raw_y;
        if (raw_y > calib_max_y) calib_max_y = raw_y;
    }
}

void ui_gimbal_calib_confirm_step(void)
{
    switch (gimbal_calib_state) {
        case GIMBAL_CALIB_CENTER:
            // Store current position as center (would need to get current values)
            calib_center_x = 0;  // In real implementation, get current raw value
            calib_center_y = 0;
            gimbal_calib_state = GIMBAL_CALIB_MIN_MAX;
            break;
            
        case GIMBAL_CALIB_MIN_MAX:
            gimbal_calib_state = GIMBAL_CALIB_COMPLETE;
            
            // Report calibration values
            if (gimbal_calib_callback) {
                // X axis
                gimbal_calib_callback(current_gimbal_id * 2, calib_min_x, calib_center_x, calib_max_x);
                // Y axis
                gimbal_calib_callback(current_gimbal_id * 2 + 1, calib_min_y, calib_center_y, calib_max_y);
            }
            
            if (gimbal_complete_callback) {
                gimbal_complete_callback(CALIB_TYPE_GIMBAL, true);
            }
            break;
            
        case GIMBAL_CALIB_COMPLETE:
            ui_gimbal_calib_screen_show(false);
            break;
            
        default:
            break;
    }
    
    update_gimbal_calib_state();
}

void ui_gimbal_calib_cancel(void)
{
    gimbal_calib_state = GIMBAL_CALIB_IDLE;
    
    if (gimbal_complete_callback) {
        gimbal_complete_callback(CALIB_TYPE_GIMBAL, false);
    }
    
    ui_gimbal_calib_screen_show(false);
}

GimbalCalibState_t ui_gimbal_calib_get_state(void)
{
    return gimbal_calib_state;
}

void ui_gimbal_calib_set_callback(gimbal_calib_cb_t cb)
{
    gimbal_calib_callback = cb;
}

void ui_gimbal_calib_set_complete_callback(calib_complete_cb_t cb)
{
    gimbal_complete_callback = cb;
}
