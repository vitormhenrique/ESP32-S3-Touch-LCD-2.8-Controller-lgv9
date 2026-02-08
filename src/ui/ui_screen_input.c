/**
 * @file ui_screen_input.c
 * @brief Input Screen Implementation - Gimbals, Nav Switches, Encoders
 */

#include "ui_screen_input.h"
#include <stdio.h>

//=============================================================================
// Screen Objects
//=============================================================================

lv_obj_t *ui_InputScreen = NULL;

// Page containers
static lv_obj_t *ui_GimbalPanel = NULL;
static lv_obj_t *ui_InputScrollPanel = NULL;
static lv_obj_t *ui_GimbalPage = NULL;
static lv_obj_t *ui_NavSwitchPage = NULL;

// Gimbal displays
lv_obj_t *ui_GimbalLeft = NULL;
lv_obj_t *ui_GimbalLeftDot = NULL;
lv_obj_t *ui_GimbalRight = NULL;
lv_obj_t *ui_GimbalRightDot = NULL;

// Navigation switch displays
lv_obj_t *ui_NavSwitch1Btns[5] = {NULL};
lv_obj_t *ui_NavSwitch2Btns[5] = {NULL};

// Encoder displays
lv_obj_t *ui_Encoder1Value = NULL;
lv_obj_t *ui_Encoder2Value = NULL;
static lv_obj_t *ui_Encoder1Panel = NULL;
static lv_obj_t *ui_Encoder2Panel = NULL;

// Button displays
lv_obj_t *ui_Button1 = NULL;
lv_obj_t *ui_Button2 = NULL;

// Potentiometer displays
lv_obj_t *ui_Pot1Bar = NULL;
lv_obj_t *ui_Pot1Value = NULL;
lv_obj_t *ui_Pot2Bar = NULL;
lv_obj_t *ui_Pot2Value = NULL;

// Layout constants
#define GIMBAL_PANEL_HEIGHT  (UI_CONTENT_HEIGHT - 60)
#define INPUT_PANEL_HEIGHT   60

//=============================================================================
// Create Gimbal Page (Page 1 of scroll)
//=============================================================================

static void create_gimbal_page(lv_obj_t *page)
{
    // Create gimbal row
    lv_obj_t *gimbal_row = lv_obj_create(page);
    lv_obj_set_size(gimbal_row, UI_SCREEN_WIDTH - 8, GIMBAL_PANEL_HEIGHT - 8);
    lv_obj_align(gimbal_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(gimbal_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gimbal_row, 0, 0);
    lv_obj_set_style_pad_all(gimbal_row, 0, 0);
    lv_obj_remove_flag(gimbal_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(gimbal_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(gimbal_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create left gimbal
    ui_GimbalLeft = ui_create_gimbal(gimbal_row, &ui_GimbalLeftDot, "Gimbal 1");
    
    // Create right gimbal
    ui_GimbalRight = ui_create_gimbal(gimbal_row, &ui_GimbalRightDot, "Gimbal 2");
}

//=============================================================================
// Create Nav Switch Page (Page 2 of scroll)
//=============================================================================

static void create_nav_switch_page(lv_obj_t *page)
{
    // Create nav switch row
    lv_obj_t *nav_row = lv_obj_create(page);
    lv_obj_set_size(nav_row, UI_SCREEN_WIDTH - 8, GIMBAL_PANEL_HEIGHT - 8);
    lv_obj_align(nav_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(nav_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_row, 0, 0);
    lv_obj_set_style_pad_all(nav_row, 0, 0);
    lv_obj_remove_flag(nav_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(nav_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create nav switch 1
    ui_create_nav_switch_widget(nav_row, ui_NavSwitch1Btns, "Nav 1");
    
    // Create nav switch 2
    ui_create_nav_switch_widget(nav_row, ui_NavSwitch2Btns, "Nav 2");
}

//=============================================================================
// Create Bottom Control Panel
//=============================================================================

static void create_control_panel(lv_obj_t *parent)
{
    // Create control row
    lv_obj_t *control_row = lv_obj_create(parent);
    lv_obj_set_size(control_row, UI_SCREEN_WIDTH - 8, INPUT_PANEL_HEIGHT - 8);
    lv_obj_align(control_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(control_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(control_row, 0, 0);
    lv_obj_set_style_pad_all(control_row, 0, 0);
    lv_obj_remove_flag(control_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(control_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(control_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Button 1
    lv_obj_t *btn1_cont = lv_obj_create(control_row);
    lv_obj_set_size(btn1_cont, 34, INPUT_PANEL_HEIGHT - 12);
    lv_obj_set_style_bg_opa(btn1_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn1_cont, 0, 0);
    lv_obj_set_style_pad_all(btn1_cont, 0, 0);
    lv_obj_remove_flag(btn1_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_Button1 = lv_obj_create(btn1_cont);
    lv_obj_set_size(ui_Button1, 30, 28);
    lv_obj_align(ui_Button1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_Button1, &ui_styles.btn_default, 0);
    lv_obj_remove_flag(ui_Button1, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *btn1_lbl = ui_create_label_small(btn1_cont, "B1");
    lv_obj_align(btn1_lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Button 2
    lv_obj_t *btn2_cont = lv_obj_create(control_row);
    lv_obj_set_size(btn2_cont, 34, INPUT_PANEL_HEIGHT - 12);
    lv_obj_set_style_bg_opa(btn2_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn2_cont, 0, 0);
    lv_obj_set_style_pad_all(btn2_cont, 0, 0);
    lv_obj_remove_flag(btn2_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_Button2 = lv_obj_create(btn2_cont);
    lv_obj_set_size(ui_Button2, 30, 28);
    lv_obj_align(ui_Button2, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_Button2, &ui_styles.btn_default, 0);
    lv_obj_remove_flag(ui_Button2, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *btn2_lbl = ui_create_label_small(btn2_cont, "B2");
    lv_obj_align(btn2_lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Encoder 1
    ui_Encoder1Panel = lv_obj_create(control_row);
    lv_obj_set_size(ui_Encoder1Panel, 50, INPUT_PANEL_HEIGHT - 12);
    lv_obj_add_style(ui_Encoder1Panel, &ui_styles.card, 0);
    lv_obj_remove_flag(ui_Encoder1Panel, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *enc1_lbl = ui_create_label_small(ui_Encoder1Panel, "E1");
    lv_obj_align(enc1_lbl, LV_ALIGN_TOP_MID, 0, 0);
    
    ui_Encoder1Value = ui_create_label(ui_Encoder1Panel, "0");
    lv_obj_set_style_text_font(ui_Encoder1Value, &lv_font_montserrat_12, 0);
    lv_obj_align(ui_Encoder1Value, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Encoder 2
    ui_Encoder2Panel = lv_obj_create(control_row);
    lv_obj_set_size(ui_Encoder2Panel, 50, INPUT_PANEL_HEIGHT - 12);
    lv_obj_add_style(ui_Encoder2Panel, &ui_styles.card, 0);
    lv_obj_remove_flag(ui_Encoder2Panel, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *enc2_lbl = ui_create_label_small(ui_Encoder2Panel, "E2");
    lv_obj_align(enc2_lbl, LV_ALIGN_TOP_MID, 0, 0);
    
    ui_Encoder2Value = ui_create_label(ui_Encoder2Panel, "0");
    lv_obj_set_style_text_font(ui_Encoder2Value, &lv_font_montserrat_12, 0);
    lv_obj_align(ui_Encoder2Value, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Pot 1
    lv_obj_t *pot1_cont = lv_obj_create(control_row);
    lv_obj_set_size(pot1_cont, 50, INPUT_PANEL_HEIGHT - 12);
    lv_obj_set_style_bg_opa(pot1_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pot1_cont, 0, 0);
    lv_obj_set_style_pad_all(pot1_cont, 0, 0);
    lv_obj_remove_flag(pot1_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *pot1_lbl = ui_create_label_small(pot1_cont, "P1");
    lv_obj_align(pot1_lbl, LV_ALIGN_TOP_MID, 0, 0);
    
    ui_Pot1Bar = lv_bar_create(pot1_cont);
    lv_obj_set_size(ui_Pot1Bar, 44, 8);
    lv_obj_align(ui_Pot1Bar, LV_ALIGN_CENTER, 0, 2);
    lv_bar_set_range(ui_Pot1Bar, 0, 1000);
    lv_bar_set_value(ui_Pot1Bar, 500, LV_ANIM_OFF);
    lv_obj_add_style(ui_Pot1Bar, &ui_styles.bar_bg, LV_PART_MAIN);
    lv_obj_add_style(ui_Pot1Bar, &ui_styles.bar_indicator, LV_PART_INDICATOR);
    
    ui_Pot1Value = ui_create_label_small(pot1_cont, "50%");
    lv_obj_align(ui_Pot1Value, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Pot 2
    lv_obj_t *pot2_cont = lv_obj_create(control_row);
    lv_obj_set_size(pot2_cont, 50, INPUT_PANEL_HEIGHT - 12);
    lv_obj_set_style_bg_opa(pot2_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pot2_cont, 0, 0);
    lv_obj_set_style_pad_all(pot2_cont, 0, 0);
    lv_obj_remove_flag(pot2_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *pot2_lbl = ui_create_label_small(pot2_cont, "P2");
    lv_obj_align(pot2_lbl, LV_ALIGN_TOP_MID, 0, 0);
    
    ui_Pot2Bar = lv_bar_create(pot2_cont);
    lv_obj_set_size(ui_Pot2Bar, 44, 8);
    lv_obj_align(ui_Pot2Bar, LV_ALIGN_CENTER, 0, 2);
    lv_bar_set_range(ui_Pot2Bar, 0, 1000);
    lv_bar_set_value(ui_Pot2Bar, 500, LV_ANIM_OFF);
    lv_obj_add_style(ui_Pot2Bar, &ui_styles.bar_bg, LV_PART_MAIN);
    lv_obj_add_style(ui_Pot2Bar, &ui_styles.bar_indicator, LV_PART_INDICATOR);
    
    ui_Pot2Value = ui_create_label_small(pot2_cont, "50%");
    lv_obj_align(ui_Pot2Value, LV_ALIGN_BOTTOM_MID, 0, 0);
}

//=============================================================================
// Public Functions
//=============================================================================

void ui_input_screen_create(lv_obj_t *parent)
{
    ui_InputScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_InputScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_InputScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_InputScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_InputScreen, 0, 0);
    lv_obj_remove_flag(ui_InputScreen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create scrollable panel for gimbals/nav switches
    ui_InputScrollPanel = ui_create_scroll_container(ui_InputScreen, UI_SCREEN_WIDTH, GIMBAL_PANEL_HEIGHT, 2);
    lv_obj_align(ui_InputScrollPanel, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create page 1 (Gimbals)
    ui_GimbalPage = lv_obj_create(ui_InputScrollPanel);
    lv_obj_set_size(ui_GimbalPage, UI_SCREEN_WIDTH, GIMBAL_PANEL_HEIGHT - 4);
    lv_obj_set_pos(ui_GimbalPage, 0, 0);
    lv_obj_set_style_bg_opa(ui_GimbalPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_GimbalPage, 0, 0);
    lv_obj_set_style_pad_all(ui_GimbalPage, 2, 0);
    lv_obj_remove_flag(ui_GimbalPage, LV_OBJ_FLAG_SCROLLABLE);
    create_gimbal_page(ui_GimbalPage);
    
    // Create page 2 (Nav Switches)
    ui_NavSwitchPage = lv_obj_create(ui_InputScrollPanel);
    lv_obj_set_size(ui_NavSwitchPage, UI_SCREEN_WIDTH, GIMBAL_PANEL_HEIGHT - 4);
    lv_obj_set_pos(ui_NavSwitchPage, UI_SCREEN_WIDTH, 0);
    lv_obj_set_style_bg_opa(ui_NavSwitchPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_NavSwitchPage, 0, 0);
    lv_obj_set_style_pad_all(ui_NavSwitchPage, 2, 0);
    lv_obj_remove_flag(ui_NavSwitchPage, LV_OBJ_FLAG_SCROLLABLE);
    create_nav_switch_page(ui_NavSwitchPage);
    
    // Create bottom control panel
    ui_GimbalPanel = ui_create_panel(ui_InputScreen, UI_SCREEN_WIDTH, INPUT_PANEL_HEIGHT);
    lv_obj_align(ui_GimbalPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_side(ui_GimbalPanel, LV_BORDER_SIDE_TOP, 0);
    create_control_panel(ui_GimbalPanel);
}

void ui_input_screen_show(bool show)
{
    if (ui_InputScreen) {
        if (show) {
            lv_obj_remove_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_input_set_gimbal(uint8_t index, int16_t x, int16_t y)
{
    lv_obj_t *dot = (index == 0) ? ui_GimbalLeftDot : ui_GimbalRightDot;
    if (!dot) return;
    
    int16_t gimbal_size = UI_GIMBAL_SIZE;
    int16_t dot_size = UI_GIMBAL_DOT_SIZE;
    int16_t max_offset = (gimbal_size - dot_size) / 2 - 2;
    int16_t px = (x * max_offset) / 1000;
    int16_t py = (-y * max_offset) / 1000;
    lv_obj_align(dot, LV_ALIGN_CENTER, px, py);
}

void ui_input_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center)
{
    lv_obj_t **btns = (index == 0) ? ui_NavSwitch1Btns : ui_NavSwitch2Btns;
    
    if (!btns[0]) return;
    
    // Update each button's style based on state
    // Order: up, down, left, right, center
    bool states[5] = {up, down, left, right, center};
    
    for (int i = 0; i < 5; i++) {
        if (btns[i]) {
            lv_obj_remove_style(btns[i], &ui_styles.btn_default, 0);
            lv_obj_remove_style(btns[i], &ui_styles.btn_highlight, 0);
            
            if (states[i]) {
                lv_obj_add_style(btns[i], &ui_styles.btn_highlight, 0);
            } else {
                lv_obj_add_style(btns[i], &ui_styles.btn_default, 0);
            }
            
            // Keep center button circular
            if (i == 4) {
                lv_obj_set_style_radius(btns[i], LV_RADIUS_CIRCLE, 0);
            }
        }
    }
}

void ui_input_set_encoder(uint8_t index, int32_t value)
{
    lv_obj_t *val_label = (index == 0) ? ui_Encoder1Value : ui_Encoder2Value;
    if (!val_label) return;
    
    char buf[12];
    snprintf(buf, sizeof(buf), "%ld", (long)value);
    lv_label_set_text(val_label, buf);
}

void ui_input_set_button(uint8_t index, bool pressed)
{
    lv_obj_t *btn = (index == 0) ? ui_Button1 : ui_Button2;
    if (!btn) return;
    
    lv_obj_remove_style(btn, &ui_styles.btn_default, 0);
    lv_obj_remove_style(btn, &ui_styles.btn_pressed, 0);
    
    if (pressed) {
        lv_obj_add_style(btn, &ui_styles.btn_pressed, 0);
    } else {
        lv_obj_add_style(btn, &ui_styles.btn_default, 0);
    }
}

void ui_input_set_pot(uint8_t index, int16_t value)
{
    lv_obj_t *bar = (index == 0) ? ui_Pot1Bar : ui_Pot2Bar;
    lv_obj_t *val_label = (index == 0) ? ui_Pot1Value : ui_Pot2Value;
    
    if (!bar) return;
    
    if (value < 0) value = 0;
    if (value > 1000) value = 1000;
    
    lv_bar_set_value(bar, value, LV_ANIM_OFF);
    
    if (val_label) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", value / 10);
        lv_label_set_text(val_label, buf);
    }
}
