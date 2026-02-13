#include "ui_comp_gimbal.h"
#include "../ui_styles.h"

lv_obj_t* ui_create_gimbal_widget(lv_obj_t *parent, lv_obj_t **dot_out)
{
    int gimbal_size = 70;  // Larger gimbals
    
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, gimbal_size + 4, gimbal_size + 4);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *gimbal = lv_obj_create(container);
    lv_obj_center(gimbal);
    lv_obj_set_size(gimbal, gimbal_size, gimbal_size);
    lv_obj_add_style(gimbal, &style_gimbal_bg, 0);
    lv_obj_remove_flag(gimbal, LV_OBJ_FLAG_SCROLLABLE);
    
    // Crosshairs
    lv_obj_t *line_h = lv_obj_create(gimbal);
    lv_obj_set_size(line_h, gimbal_size - 6, 1);
    lv_obj_align(line_h, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(line_h, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_bg_opa(line_h, LV_OPA_50, 0);
    lv_obj_set_style_border_width(line_h, 0, 0);
    lv_obj_remove_flag(line_h, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *line_v = lv_obj_create(gimbal);
    lv_obj_set_size(line_v, 1, gimbal_size - 6);
    lv_obj_align(line_v, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(line_v, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_bg_opa(line_v, LV_OPA_50, 0);
    lv_obj_set_style_border_width(line_v, 0, 0);
    lv_obj_remove_flag(line_v, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *dot = lv_obj_create(gimbal);
    lv_obj_set_size(dot, 14, 14);
    lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(dot, &style_gimbal_dot, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    
    if (dot_out) *dot_out = dot;
    return gimbal;
}

void ui_update_gimbal_dot(lv_obj_t *dot, int16_t x, int16_t y)
{
    if (!dot) return;
    int16_t gimbal_size = 70;
    int16_t dot_size = 14;
    int16_t max_offset = (gimbal_size - dot_size) / 2 - 2;
    int16_t px = (x * max_offset) / 1000;
    int16_t py = (-y * max_offset) / 1000;
    lv_obj_align(dot, LV_ALIGN_CENTER, px, py);
}
