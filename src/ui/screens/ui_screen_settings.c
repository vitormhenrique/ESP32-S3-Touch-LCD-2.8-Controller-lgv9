#include "ui_screen_settings.h"
#include "../ui_styles.h"

lv_obj_t *ui_SettingsScreen = NULL;

void ui_create_settings_screen(lv_obj_t *parent)
{
    ui_SettingsScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_SettingsScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_SettingsScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_SettingsScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_SettingsScreen, 4, 0);
    lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_SettingsScreen, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(ui_SettingsScreen, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t *title = lv_label_create(ui_SettingsScreen);
    lv_label_set_text(title, "Settings");
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);
    
    const char *settings[] = {"WiFi Config", "Calibration", "Display", "About"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(ui_SettingsScreen);
        lv_obj_set_size(btn, lv_pct(90), 30);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 30 + i * 38);
        lv_obj_add_style(btn, &style_card, 0);
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, settings[i]);
        lv_obj_add_style(lbl, &style_text_primary, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
    }
}
