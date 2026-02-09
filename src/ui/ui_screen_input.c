/**
 * @file ui_screen_input.c
 * @brief Input Screen Implementation
 */

#include "ui_screen_input.h"
#include <stdio.h>

//=============================================================================
// Objects
//=============================================================================

lv_obj_t *ui_InputScreen = NULL;

// Top Fixed Panel Objects
lv_obj_t *ui_GimbalLeft = NULL;
lv_obj_t *ui_GimbalLeftDot = NULL;
lv_obj_t *ui_GimbalRight = NULL;
lv_obj_t *ui_GimbalRightDot = NULL;
lv_obj_t *ui_NavSwitch1Btns[5] = {NULL};
lv_obj_t *ui_NavSwitch2Btns[5] = {NULL};

// Bottom Scrollable Panel Objects
lv_obj_t *ui_Button1 = NULL;
lv_obj_t *ui_Button2 = NULL;
lv_obj_t *ui_Button3 = NULL;
lv_obj_t *ui_Button4 = NULL;

lv_obj_t *ui_Switch1 = NULL;
lv_obj_t *ui_Switch2 = NULL;
lv_obj_t *ui_Switch3 = NULL;
lv_obj_t *ui_Switch4 = NULL;
lv_obj_t *ui_Switch5 = NULL;
lv_obj_t *ui_Switch6 = NULL;

lv_obj_t *ui_Toggle1 = NULL;
lv_obj_t *ui_Toggle2 = NULL;
static lv_obj_t *ui_Toggle1Ind = NULL;
static lv_obj_t *ui_Toggle2Ind = NULL;

lv_obj_t *ui_Pot1Bar = NULL;
lv_obj_t *ui_Pot1Value = NULL;
lv_obj_t *ui_Pot2Bar = NULL;
lv_obj_t *ui_Pot2Value = NULL;

lv_obj_t *ui_Encoder1Value = NULL;
lv_obj_t *ui_Encoder2Value = NULL;

//=============================================================================
// Helper Functions
//=============================================================================

static lv_obj_t* create_switch_shim(lv_obj_t *parent, const char* label_text)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 60, 40);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *sw = lv_obj_create(cont);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_add_style(sw, &ui_styles.switch_off, 0);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label_text);
    lv_obj_add_style(lbl, &ui_styles.text_small, 0);
    
    return sw;
}

static lv_obj_t* create_toggle_shim(lv_obj_t *parent, const char* label_text, lv_obj_t **indicator)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 40, 70);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *bg = lv_obj_create(cont);
    lv_obj_set_size(bg, 20, 50);
    lv_obj_add_style(bg, &ui_styles.toggle3_bg, 0);

    *indicator = lv_obj_create(bg);
    lv_obj_set_size(*indicator, 14, 14);
    lv_obj_add_style(*indicator, &ui_styles.toggle3_indicator, 0);
    lv_obj_align(*indicator, LV_ALIGN_TOP_MID, 0, 0); // Default Top

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label_text);
    lv_obj_add_style(lbl, &ui_styles.text_small, 0);

    return bg;
}

static lv_obj_t* create_button_shim(lv_obj_t *parent, const char* label_text)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, 60, 40);
    lv_obj_add_style(btn, &ui_styles.btn_default, 0);
    
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label_text);
    lv_obj_center(lbl);
    lv_obj_add_style(lbl, &ui_styles.text_primary, 0);
    
    return btn;
}

static void create_pot_bar_shim(lv_obj_t *parent, const char* label_text, lv_obj_t **bar, lv_obj_t **value_label)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 140, 40);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_gap(cont, 10, 0);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label_text);
    lv_obj_add_style(lbl, &ui_styles.text_small, 0);
    lv_obj_set_width(lbl, 20);

    *bar = lv_bar_create(cont);
    lv_obj_set_size(*bar, 80, 12);
    lv_obj_add_style(*bar, &ui_styles.bar_bg, LV_PART_MAIN);
    lv_obj_add_style(*bar, &ui_styles.bar_indicator, LV_PART_INDICATOR);
    lv_bar_set_range(*bar, 0, 100);

    *value_label = lv_label_create(cont);
    lv_label_set_text(*value_label, "0%");
    lv_obj_add_style(*value_label, &ui_styles.text_small, 0);
}

static void create_encoder_display_shim(lv_obj_t *parent, const char* label_text, lv_obj_t **value_label)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 70, 40);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 0, 0);

    *value_label = lv_label_create(cont);
    lv_label_set_text(*value_label, "0");
    lv_obj_add_style(*value_label, &ui_styles.text_primary, 0);
    
    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label_text);
    lv_obj_add_style(lbl, &ui_styles.text_small, 0);
}

//=============================================================================
// Implementation
//=============================================================================

void ui_input_screen_create(lv_obj_t *parent)
{
    ui_styles_init();

    ui_InputScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_InputScreen, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(ui_InputScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(ui_InputScreen, 0, 0);
    lv_obj_set_style_border_width(ui_InputScreen, 0, 0);
    lv_obj_clear_flag(ui_InputScreen, LV_OBJ_FLAG_SCROLLABLE);

    //---------------------------------------------------------
    // Top Panel: Gimbals + Nav Switches
    // Height: ~120px
    //---------------------------------------------------------
    lv_obj_t *top_panel = lv_obj_create(ui_InputScreen);
    lv_obj_set_size(top_panel, UI_SCREEN_WIDTH, 120);
    lv_obj_align(top_panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(top_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_panel, 0, 0);
    lv_obj_set_style_pad_all(top_panel, 5, 0);
    lv_obj_clear_flag(top_panel, LV_OBJ_FLAG_SCROLLABLE);

    // Left Gimbal
    ui_GimbalLeft = ui_create_gimbal(top_panel, &ui_GimbalLeftDot, "Gimbal L");
    lv_obj_align(ui_GimbalLeft, LV_ALIGN_LEFT_MID, 10, 0);

    // Left Nav Switch
    lv_obj_t *nav1 = ui_create_nav_switch_widget(top_panel, ui_NavSwitch1Btns, "Nav L");
    lv_obj_align(nav1, LV_ALIGN_LEFT_MID, 110, 0);

    // Right Gimbal
    ui_GimbalRight = ui_create_gimbal(top_panel, &ui_GimbalRightDot, "Gimbal R");
    lv_obj_align(ui_GimbalRight, LV_ALIGN_RIGHT_MID, -10, 0);

    // Right Nav Switch
    lv_obj_t *nav2 = ui_create_nav_switch_widget(top_panel, ui_NavSwitch2Btns, "Nav R");
    lv_obj_align(nav2, LV_ALIGN_RIGHT_MID, -110, 0);

    //---------------------------------------------------------
    // Bottom Scrollable Panel
    // Height: Remainder (~120px)
    //---------------------------------------------------------
    lv_obj_t *bottom_panel = lv_obj_create(ui_InputScreen);
    lv_obj_set_size(bottom_panel, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT - 120);
    lv_obj_align(bottom_panel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_panel, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(bottom_panel, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(bottom_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_snap_x(bottom_panel, LV_SCROLL_SNAP_CENTER);
    lv_obj_add_flag(bottom_panel, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_style_pad_all(bottom_panel, 0, 0);
    lv_obj_set_style_pad_gap(bottom_panel, 0, 0);
    lv_obj_set_style_border_width(bottom_panel, 1, 0);
    lv_obj_set_style_border_color(bottom_panel, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_side(bottom_panel, LV_BORDER_SIDE_TOP, 0);

    //---------------------------------------------------------
    // PAGE 1: Switches (1-3), Toggles, Buttons
    //---------------------------------------------------------
    lv_obj_t *page1 = lv_obj_create(bottom_panel);
    lv_obj_set_width(page1, lv_pct(100)); // Full width of parent content
    lv_obj_set_height(page1, lv_pct(100));
    //lv_obj_set_flex_shrink(page1, 0); // Don't shrink in row
    lv_obj_set_style_bg_opa(page1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page1, 0, 0);
    lv_obj_set_flex_flow(page1, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(page1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(page1, 5, 0);
    lv_obj_set_style_pad_gap(page1, 10, 0);

    // Row 1: Buttons
    ui_Button1 = create_button_shim(page1, "B1");
    ui_Button2 = create_button_shim(page1, "B2");
    ui_Button3 = create_button_shim(page1, "B3");
    ui_Button4 = create_button_shim(page1, "B4");

    // Row 2: Toggles & Switches
    // T1, T2
    ui_Toggle1 = create_toggle_shim(page1, "T1", &ui_Toggle1Ind);
    ui_Toggle2 = create_toggle_shim(page1, "T2", &ui_Toggle2Ind);

    // S1, S2, S3
    ui_Switch1 = create_switch_shim(page1, "S1");
    ui_Switch2 = create_switch_shim(page1, "S2");
    ui_Switch3 = create_switch_shim(page1, "S3");

    //---------------------------------------------------------
    // PAGE 2: Switches (4-6), Pots, Encoders
    //---------------------------------------------------------
    lv_obj_t *page2 = lv_obj_create(bottom_panel);
    lv_obj_set_width(page2, lv_pct(100)); // Full width of parent content
    lv_obj_set_height(page2, lv_pct(100));
    //lv_obj_set_flex_shrink(page2, 0);
    lv_obj_set_style_bg_opa(page2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page2, 0, 0);
    lv_obj_set_flex_flow(page2, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(page2, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(page2, 5, 0);
    lv_obj_set_style_pad_gap(page2, 5, 0); // Tighter gap for pots

    // Switches S4, S5, S6
    ui_Switch4 = create_switch_shim(page2, "S4");
    ui_Switch5 = create_switch_shim(page2, "S5");
    ui_Switch6 = create_switch_shim(page2, "S6");

    // Encoders
    create_encoder_display_shim(page2, "E1", &ui_Encoder1Value);
    create_encoder_display_shim(page2, "E2", &ui_Encoder2Value);

    // Pots (Full width almost)
    create_pot_bar_shim(page2, "P1", &ui_Pot1Bar, &ui_Pot1Value);
    create_pot_bar_shim(page2, "P2", &ui_Pot2Bar, &ui_Pot2Value);

    // Initially hide
    lv_obj_add_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
}

void ui_input_screen_show(bool show)
{
    if (show) {
        lv_obj_clear_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_input_set_gimbal(uint8_t index, int16_t x, int16_t y)
{
    // index 0=Left, 1=Right
    // x, y: -100 to 100
    // Visual range: 70x70 gimbal size (defined in ui_common.h UI_GIMBAL_SIZE)
    // Dot size: UI_GIMBAL_DOT_SIZE (14)
    // Max movement: (70 - 14) / 2 = 28 pixel radius.

    lv_obj_t *dot = (index == 0) ? ui_GimbalLeftDot : ui_GimbalRightDot;
    if (!dot) return;

    // Use constants from ui_common.h if possible, or hardcode based on known size
    // UI_GIMBAL_SIZE is 70
    // UI_GIMBAL_DOT_SIZE is 14
    int max_r = (UI_GIMBAL_SIZE - UI_GIMBAL_DOT_SIZE) / 2;

    int16_t px = (x * max_r) / 100;
    int16_t py = (y * max_r) / 100; // Y up is positive usually
    py = -py; // Screen Y is down

    lv_obj_align(dot, LV_ALIGN_CENTER, px, py);
}

void ui_input_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center)
{
    lv_obj_t **btns = (index == 0) ? ui_NavSwitch1Btns : ui_NavSwitch2Btns;
    if (!btns[0]) return; // Check one

    // 0=Up, 1=Down, 2=Left, 3=Right, 4=Center
    bool states[5] = {up, down, left, right, center};

    for (int i = 0; i < 5; i++) {
        if (states[i]) {
            // Apply highlight style
             lv_obj_add_style(btns[i], &ui_styles.btn_highlight, 0);
        } else {
             lv_obj_remove_style(btns[i], &ui_styles.btn_highlight, 0);
        }
    }
}

void ui_input_set_button(uint8_t index, bool pressed)
{
    lv_obj_t *btn = NULL;
    switch(index) {
        case 0: btn = ui_Button1; break;
        case 1: btn = ui_Button2; break;
        case 2: btn = ui_Button3; break;
        case 3: btn = ui_Button4; break;
    }
    if (!btn) return;

    if (pressed) {
        lv_obj_add_style(btn, &ui_styles.btn_pressed, 0);
    } else {
        lv_obj_remove_style(btn, &ui_styles.btn_pressed, 0);
    }
}

void ui_input_set_switch(uint8_t index, bool active)
{
    lv_obj_t *sw = NULL;
    switch(index) {
        case 0: sw = ui_Switch1; break;
        case 1: sw = ui_Switch2; break;
        case 2: sw = ui_Switch3; break;
        case 3: sw = ui_Switch4; break;
        case 4: sw = ui_Switch5; break;
        case 5: sw = ui_Switch6; break;
    }
    if (!sw) return;

    if (active) {
        lv_obj_add_style(sw, &ui_styles.switch_on, 0);
        lv_obj_remove_style(sw, &ui_styles.switch_off, 0);
    } else {
        lv_obj_add_style(sw, &ui_styles.switch_off, 0);
        lv_obj_remove_style(sw, &ui_styles.switch_on, 0);
    }
}

void ui_input_set_toggle(uint8_t index, uint8_t state)
{
    lv_obj_t *ind = (index == 0) ? ui_Toggle1Ind : ui_Toggle2Ind;
    if (!ind) return;

    // state: 0=Top, 1=Middle, 2=Bottom
    
    // Reset styles
    lv_obj_remove_style_all(ind);
    lv_obj_add_style(ind, &ui_styles.toggle3_indicator, 0);

    if (state == 0) {
        lv_obj_align(ind, LV_ALIGN_TOP_MID, 0, 2);
    } else if (state == 1) {
        lv_obj_align(ind, LV_ALIGN_CENTER, 0, 0);
    } else {
        lv_obj_align(ind, LV_ALIGN_BOTTOM_MID, 0, -2);
    }
}

void ui_input_set_pot(uint8_t index, int16_t value)
{
    lv_obj_t *bar = (index == 0) ? ui_Pot1Bar : ui_Pot2Bar;
    lv_obj_t *lbl = (index == 0) ? ui_Pot1Value : ui_Pot2Value;
    
    if (bar) lv_bar_set_value(bar, value, LV_ANIM_OFF);
    if (lbl) lv_label_set_text_fmt(lbl, "%d%%", value);
}

void ui_input_set_encoder(uint8_t index, int32_t value)
{
    lv_obj_t *lbl = (index == 0) ? ui_Encoder1Value : ui_Encoder2Value;
    if (lbl) lv_label_set_text_fmt(lbl, "%ld", (long)value);
}
