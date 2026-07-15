#include "ui_screen_input.h"
#include "../ui_styles.h"
#include "../components/ui_comp_gimbal.h"
#include "../components/ui_comp_navswitch.h"
#include "../../InputSim.h"
#include <stdio.h>

lv_obj_t *ui_InputScreen = NULL;
lv_obj_t *ui_GimbalPanel = NULL;
lv_obj_t *ui_GimbalLeft = NULL;
lv_obj_t *ui_GimbalLeftDot = NULL;
lv_obj_t *ui_GimbalRight = NULL;
lv_obj_t *ui_GimbalRightDot = NULL;
lv_obj_t *ui_GimbalPage = NULL;
lv_obj_t *ui_NavPage = NULL;
lv_obj_t *ui_NavSwitchIndicators[2][5] = {{NULL}};
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

//=============================================================================
// UI input simulation (compile with -DUI_INPUT_SIM=1)
// Widgets become touch input sources; state lives in InputSim.c
//=============================================================================

#if UI_INPUT_SIM

static bool sim_target_inside(lv_obj_t *target, lv_obj_t *root)
{
    while (target) {
        if (target == root) return true;
        target = lv_obj_get_parent(target);
    }
    return false;
}

static void sim_set_gimbal_from_touch(lv_obj_t *gimbal, uint8_t axis_base)
{
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_area_t coords;
    lv_obj_get_coords(gimbal, &coords);
    int32_t half_w = (coords.x2 - coords.x1) / 2;
    int32_t half_h = (coords.y2 - coords.y1) / 2;
    if (half_w <= 0 || half_h <= 0) return;
    int32_t cx = (coords.x1 + coords.x2) / 2;
    int32_t cy = (coords.y1 + coords.y2) / 2;

    input_sim_set_gimbal(axis_base + 0, (int16_t)(((p.x - cx) * 1000) / half_w));
    input_sim_set_gimbal(axis_base + 1, (int16_t)((-(p.y - cy) * 1000) / half_h));
}

static void sim_input_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    for (uint8_t i = 0; i < 4; i++) {
        if (!sim_target_inside(target, lv_obj_get_parent(ui_Buttons[i]))) continue;
        if (code == LV_EVENT_PRESSED) input_sim_set_button(i, true);
        if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
            input_sim_set_button(i, false);
        }
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        for (uint8_t i = 0; i < 6; i++) {
            if (sim_target_inside(target, lv_obj_get_parent(ui_Switches[i]))) {
                input_sim_toggle_switch(i);
                return;
            }
        }
        for (uint8_t i = 0; i < 2; i++) {
            if (sim_target_inside(target, lv_obj_get_parent(ui_Toggle3Panels[i]))) {
                input_sim_cycle_toggle3(i);
                return;
            }
        }
    }

    uint8_t axis_base;
    if (target == ui_GimbalLeft) axis_base = 0;
    else if (target == ui_GimbalRight) axis_base = 2;
    else return;

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        sim_set_gimbal_from_touch(target, axis_base);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        input_sim_set_gimbal(axis_base + 0, 0);
        input_sim_set_gimbal(axis_base + 1, 0);
    }
}

static void sim_enable_bubbling_to_input_screen(lv_obj_t *obj)
{
    while (obj && obj != ui_InputScreen) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
        obj = lv_obj_get_parent(obj);
    }
}

static void sim_attach_input_handlers(void)
{
    // One descriptor for all simulated controls. Per-widget descriptors caused
    // LVGL heap exhaustion before the navigation bar was created.
    lv_obj_add_event_cb(ui_InputScreen, sim_input_event_cb, LV_EVENT_ALL, NULL);

    for (uint8_t i = 0; i < 4; i++) {
        if (!ui_Buttons[i]) continue;
        lv_obj_t *cont = lv_obj_get_parent(ui_Buttons[i]);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
        sim_enable_bubbling_to_input_screen(cont);
        lv_obj_add_flag(ui_Buttons[i], LV_OBJ_FLAG_CLICKABLE);
        sim_enable_bubbling_to_input_screen(ui_Buttons[i]);
    }
    for (uint8_t i = 0; i < 6; i++) {
        if (!ui_Switches[i]) continue;
        lv_obj_t *cont = lv_obj_get_parent(ui_Switches[i]);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
        sim_enable_bubbling_to_input_screen(cont);
        lv_obj_add_flag(ui_Switches[i], LV_OBJ_FLAG_CLICKABLE);
        sim_enable_bubbling_to_input_screen(ui_Switches[i]);
    }
    for (uint8_t i = 0; i < 2; i++) {
        if (!ui_Toggle3Panels[i]) continue;
        lv_obj_t *cont = lv_obj_get_parent(ui_Toggle3Panels[i]);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
        sim_enable_bubbling_to_input_screen(cont);
        lv_obj_add_flag(ui_Toggle3Panels[i], LV_OBJ_FLAG_CLICKABLE);
        sim_enable_bubbling_to_input_screen(ui_Toggle3Panels[i]);
    }
    lv_obj_t *gimbals[] = {ui_GimbalLeft, ui_GimbalRight};
    for (uint8_t i = 0; i < 2; i++) {
        lv_obj_t *gimbal = gimbals[i];
        if (!gimbal) continue;
        lv_obj_add_flag(gimbal, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(gimbal, LV_OBJ_FLAG_SCROLL_CHAIN);
        uint32_t cnt = lv_obj_get_child_count(gimbal);
        for (uint32_t child = 0; child < cnt; child++) {
            lv_obj_remove_flag(lv_obj_get_child(gimbal, child), LV_OBJ_FLAG_CLICKABLE);
        }
        sim_enable_bubbling_to_input_screen(gimbal);
    }
}

#endif // UI_INPUT_SIM

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
    const char *btn_names[] = {"BTN_1", "BTN_2", "BTN_3", "BTN_4"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn_cont = lv_obj_create(row);
        lv_obj_set_size(btn_cont, 38, INPUT_PANEL_HEIGHT - 12);
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
    const char *toggle_names[] = {"SW_E", "SW_F"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *toggle_cont = lv_obj_create(row);
        lv_obj_set_size(toggle_cont, 30, INPUT_PANEL_HEIGHT - 12);
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
    const char *sw_names[] = {"SW_A", "SW_B", "SW_C"};
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
    const char *sw_names[] = {"SW_D", "SW_G", "SW_H"};
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
    const char *pot_names[] = {"POT_1", "POT_2"};
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
        lv_obj_set_size(ui_PotBars[i], UI_POT_BAR_WIDTH, UI_POT_BAR_HEIGHT);
        lv_obj_align(ui_PotBars[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_style(ui_PotBars[i], &style_bar_bg, 0);
        lv_obj_add_style(ui_PotBars[i], &style_bar_indicator, LV_PART_INDICATOR);
        lv_bar_set_range(ui_PotBars[i], 0, 1000);
        
        ui_PotValues[i] = lv_label_create(pot_cont);
        lv_label_set_text(ui_PotValues[i], "50%");
        lv_obj_add_style(ui_PotValues[i], &style_text_small, 0);
        lv_obj_align(ui_PotValues[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    
    // 2 Encoders
    const char *enc_names[] = {"ENC_1", "ENC_2"};
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
        lv_obj_set_style_text_font(ui_EncValues[i], &lv_font_montserrat_10, 0);
        lv_obj_align(ui_EncValues[i], LV_ALIGN_BOTTOM_MID, 0, -2);
        
        ui_EncPanels[i] = enc_cont;
    }
}

void ui_create_input_screen(lv_obj_t *parent)
{
    ui_InputScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_InputScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_InputScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_InputScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_InputScreen, 0, 0);
    lv_obj_remove_flag(ui_InputScreen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Gimbal/Nav Panel (larger area) ===
    ui_GimbalPanel = lv_obj_create(ui_InputScreen);
    lv_obj_set_size(ui_GimbalPanel, UI_SCREEN_WIDTH, GIMBAL_PANEL_HEIGHT);
    lv_obj_align(ui_GimbalPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(ui_GimbalPanel, &style_panel, 0);
    lv_obj_set_style_border_side(ui_GimbalPanel, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(ui_GimbalPanel, 0, 0);

    // Enable horizontal scrolling with snap
    lv_obj_add_flag(ui_GimbalPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui_GimbalPanel, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(ui_GimbalPanel, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(ui_GimbalPanel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(ui_GimbalPanel, input_panel_scroll_event_cb, LV_EVENT_SCROLL_END, NULL);

    // === Page 1: Gimbals ===
    ui_GimbalPage = lv_obj_create(ui_GimbalPanel);
    lv_obj_set_size(ui_GimbalPage, UI_SCREEN_WIDTH, GIMBAL_PANEL_HEIGHT - 6);
    lv_obj_set_pos(ui_GimbalPage, 0, 0);
    lv_obj_set_style_bg_opa(ui_GimbalPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_GimbalPage, 0, 0);
    lv_obj_remove_flag(ui_GimbalPage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(ui_GimbalPage, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_GimbalPage, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui_GimbalLeft = ui_create_gimbal_widget(ui_GimbalPage, &ui_GimbalLeftDot);
    ui_GimbalRight = ui_create_gimbal_widget(ui_GimbalPage, &ui_GimbalRightDot);

    // === Page 2: Nav Switches ===
    ui_NavPage = lv_obj_create(ui_GimbalPanel);
    lv_obj_set_size(ui_NavPage, UI_SCREEN_WIDTH, GIMBAL_PANEL_HEIGHT - 6);
    lv_obj_set_pos(ui_NavPage, UI_SCREEN_WIDTH, 0);
    lv_obj_set_style_bg_opa(ui_NavPage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_NavPage, 0, 0);
    lv_obj_remove_flag(ui_NavPage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(ui_NavPage, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_NavPage, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui_create_nav_switch_widget(ui_NavPage, ui_NavSwitchIndicators[0]);
    ui_create_nav_switch_widget(ui_NavPage, ui_NavSwitchIndicators[1]);
    
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

void ui_enable_input_simulation(void)
{
#if UI_INPUT_SIM
    sim_attach_input_handlers();
#endif
}

void ui_set_gimbal_left(int16_t x, int16_t y)
{
    ui_update_gimbal_dot(ui_GimbalLeftDot, x, y);
}

void ui_set_gimbal_right(int16_t x, int16_t y)
{
    ui_update_gimbal_dot(ui_GimbalRightDot, x, y);
}

void ui_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center)
{
    if (index >= 2) return;
    ui_update_nav_switch(ui_NavSwitchIndicators[index], up, down, left, right, center);
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
