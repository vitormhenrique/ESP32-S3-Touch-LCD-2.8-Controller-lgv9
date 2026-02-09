#include "ui.h"

lv_obj_t *ui_MainScreen = NULL;
lv_obj_t *ui_ContentArea = NULL;
static int current_screen = SCREEN_INPUT;

void ui_init(void)
{
    ui_styles_init();
    
    ui_MainScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_MainScreen, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_bg_opa(ui_MainScreen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(ui_MainScreen, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_create_header(ui_MainScreen);
    
    ui_ContentArea = lv_obj_create(ui_MainScreen);
    lv_obj_set_size(ui_ContentArea, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(ui_ContentArea, 0, UI_HEADER_HEIGHT);
    lv_obj_set_style_bg_opa(ui_ContentArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_ContentArea, 0, 0);
    lv_obj_set_style_pad_all(ui_ContentArea, 0, 0);
    lv_obj_remove_flag(ui_ContentArea, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_create_input_screen(ui_ContentArea);
    ui_create_telemetry_screen(ui_ContentArea);
    ui_create_settings_screen(ui_ContentArea);
    
    ui_create_nav_bar(ui_MainScreen);
    
    lv_screen_load(ui_MainScreen);
    
    // Set initial screen
    ui_show_screen(SCREEN_INPUT);
}

void ui_destroy(void)
{
    if (ui_MainScreen) {
        lv_obj_del(ui_MainScreen);
        ui_MainScreen = NULL;
    }
}

void ui_show_screen(int screen_index)
{
    if (screen_index >= SCREEN_COUNT) return;
    
    if (ui_InputScreen) lv_obj_add_flag(ui_InputScreen, LV_OBJ_FLAG_HIDDEN);
    if (ui_TelemetryScreen) lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
    if (ui_SettingsScreen) lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
    
    switch (screen_index) {
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
    
    current_screen = screen_index;
    ui_update_nav_buttons(screen_index);
}

int ui_get_current_screen(void)
{
    return current_screen;
}

void ui_update(void)
{
    // periodic update if needed
}
