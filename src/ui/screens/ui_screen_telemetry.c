#include "ui_screen_telemetry.h"
#include "../ui_styles.h"

lv_obj_t *ui_TelemetryScreen = NULL;
lv_obj_t *ui_TelemetryPanel1 = NULL;
lv_obj_t *ui_TelemetryPanel2 = NULL;

static lv_obj_t *ui_TelemetryLabels[8] = {NULL};
static lv_obj_t *ui_TelemetryValues[8] = {NULL};

void ui_create_telemetry_screen(lv_obj_t *parent)
{
    ui_TelemetryScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_TelemetryScreen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_TelemetryScreen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_TelemetryScreen, 0, 0);
    lv_obj_set_style_pad_all(ui_TelemetryScreen, 2, 0);
    lv_obj_remove_flag(ui_TelemetryScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_TelemetryScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Panel 1: Robot Data
    ui_TelemetryPanel1 = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(ui_TelemetryPanel1, 155, UI_CONTENT_HEIGHT - 8);
    lv_obj_align(ui_TelemetryPanel1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(ui_TelemetryPanel1, &style_card, 0);
    lv_obj_set_style_pad_all(ui_TelemetryPanel1, 4, 0);
    lv_obj_add_flag(ui_TelemetryPanel1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_TelemetryPanel1, LV_SCROLLBAR_MODE_AUTO);
    
    lv_obj_t *title1 = lv_label_create(ui_TelemetryPanel1);
    lv_label_set_text(title1, "Robot Data");
    lv_obj_add_style(title1, &style_text_primary, 0);
    lv_obj_set_style_text_font(title1, &lv_font_montserrat_10, 0);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 0);
    
    const char *labels1[] = {"Temp 1:", "Temp 2:", "Bat V:", "Current:"};
    const char *values1[] = {"--°C", "--°C", "--V", "--A"};
    for (int i = 0; i < 4; i++) {
        ui_TelemetryLabels[i] = lv_label_create(ui_TelemetryPanel1);
        lv_label_set_text(ui_TelemetryLabels[i], labels1[i]);
        lv_obj_add_style(ui_TelemetryLabels[i], &style_text_secondary, 0);
        lv_obj_set_style_text_font(ui_TelemetryLabels[i], &lv_font_montserrat_10, 0);
        lv_obj_align(ui_TelemetryLabels[i], LV_ALIGN_TOP_LEFT, 0, 18 + i * 18);
        
        ui_TelemetryValues[i] = lv_label_create(ui_TelemetryPanel1);
        lv_label_set_text(ui_TelemetryValues[i], values1[i]);
        lv_obj_add_style(ui_TelemetryValues[i], &style_text_primary, 0);
        lv_obj_set_style_text_font(ui_TelemetryValues[i], &lv_font_montserrat_10, 0);
        lv_obj_align(ui_TelemetryValues[i], LV_ALIGN_TOP_RIGHT, 0, 18 + i * 18);
    }
    
    // Panel 2: Status
    ui_TelemetryPanel2 = lv_obj_create(ui_TelemetryScreen);
    lv_obj_set_size(ui_TelemetryPanel2, 155, UI_CONTENT_HEIGHT - 8);
    lv_obj_align(ui_TelemetryPanel2, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_style(ui_TelemetryPanel2, &style_card, 0);
    lv_obj_set_style_pad_all(ui_TelemetryPanel2, 4, 0);
    lv_obj_add_flag(ui_TelemetryPanel2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_TelemetryPanel2, LV_SCROLLBAR_MODE_AUTO);
    
    lv_obj_t *title2 = lv_label_create(ui_TelemetryPanel2);
    lv_label_set_text(title2, "Status");
    lv_obj_add_style(title2, &style_text_primary, 0);
    lv_obj_set_style_text_font(title2, &lv_font_montserrat_10, 0);
    lv_obj_align(title2, LV_ALIGN_TOP_MID, 0, 0);
    
    const char *labels2[] = {"RSSI:", "Latency:", "Errors:", "Uptime:"};
    const char *values2[] = {"--dBm", "--ms", "0", "--"};
    for (int i = 0; i < 4; i++) {
        ui_TelemetryLabels[i + 4] = lv_label_create(ui_TelemetryPanel2);
        lv_label_set_text(ui_TelemetryLabels[i + 4], labels2[i]);
        lv_obj_add_style(ui_TelemetryLabels[i + 4], &style_text_secondary, 0);
        lv_obj_set_style_text_font(ui_TelemetryLabels[i + 4], &lv_font_montserrat_10, 0);
        lv_obj_align(ui_TelemetryLabels[i + 4], LV_ALIGN_TOP_LEFT, 0, 18 + i * 18);
        
        ui_TelemetryValues[i + 4] = lv_label_create(ui_TelemetryPanel2);
        lv_label_set_text(ui_TelemetryValues[i + 4], values2[i]);
        lv_obj_add_style(ui_TelemetryValues[i + 4], &style_text_primary, 0);
        lv_obj_set_style_text_font(ui_TelemetryValues[i + 4], &lv_font_montserrat_10, 0);
        lv_obj_align(ui_TelemetryValues[i + 4], LV_ALIGN_TOP_RIGHT, 0, 18 + i * 18);
    }
}

void ui_update_telemetry_value(uint8_t index, const char *label, const char *value)
{
    if (index >= 8) return;
    if (ui_TelemetryLabels[index] && label) {
        lv_label_set_text(ui_TelemetryLabels[index], label);
    }
    if (ui_TelemetryValues[index] && value) {
        lv_label_set_text(ui_TelemetryValues[index], value);
    }
}
