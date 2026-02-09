#include "ui_comp_navswitch.h"
#include "../ui_styles.h"

lv_obj_t* ui_create_nav_switch_widget(lv_obj_t *parent, lv_obj_t **indicators)
{
    int size = 90; // Increased overall size
    
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, size, size);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *bg = lv_obj_create(container);
    lv_obj_set_size(bg, size, size);
    lv_obj_center(bg);
    // Removed style_gimbal_bg for transparent look without blue border
    lv_obj_set_style_bg_opa(bg, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    
    // Modern D-Pad Layout - Continuous Cross
    // 0: Up, 1: Down, 2: Left, 3: Right, 4: Center
    // Overlapping buttons to create a solid shape
    
    int b_size = 28; // Increased button size (~15% bigger)
    int offset = 22; // Increased offset for larger layout
    
    struct { int w; int h; int x; int y; int r; const char *sym; } props[] = {
        {b_size, b_size, 0, -offset, 8, LV_SYMBOL_UP},    // Up
        {b_size, b_size, 0, offset, 8, LV_SYMBOL_DOWN},   // Down
        {b_size, b_size, -offset, 0, 8, LV_SYMBOL_LEFT},  // Left
        {b_size, b_size, offset, 0, 8, LV_SYMBOL_RIGHT},  // Right
        {34, 34, 0, 0, LV_RADIUS_CIRCLE, LV_SYMBOL_BULLET}  // Center: Bigger ball (34px)
    };
    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *ind = lv_obj_create(bg);
        lv_obj_set_size(ind, props[i].w, props[i].h);
        lv_obj_align(ind, LV_ALIGN_CENTER, props[i].x, props[i].y);
        lv_obj_set_style_radius(ind, props[i].r, 0);
        lv_obj_set_style_bg_color(ind, lv_color_hex(UI_COLOR_SWITCH_OFF), 0);
        lv_obj_set_style_border_width(ind, 0, 0);
        lv_obj_remove_flag(ind, LV_OBJ_FLAG_SCROLLABLE);
        
        // Add symbol label
        lv_obj_t *lbl = lv_label_create(ind);
        lv_label_set_text(lbl, props[i].sym);
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        
        if (indicators) indicators[i] = ind;
    }
    
    return container;
}

void ui_update_nav_switch(lv_obj_t **indicators, bool up, bool down, bool left, bool right, bool center)
{
    if (!indicators) return;
    
    // 0=Up, 1=Down, 2=Left, 3=Right, 4=Center
    bool states[] = {up, down, left, right, center};
    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *ind = indicators[i];
        if (ind) {
            lv_color_t color = states[i] ? lv_color_hex(UI_COLOR_ACCENT_BLUE) : lv_color_hex(UI_COLOR_SWITCH_OFF);
            lv_obj_set_style_bg_color(ind, color, 0);
            
            // Add glow effect if active
            if (states[i]) {
                lv_obj_set_style_shadow_width(ind, 8, 0);
                lv_obj_set_style_shadow_color(ind, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
                lv_obj_set_style_shadow_opa(ind, LV_OPA_50, 0);
            } else {
                lv_obj_set_style_shadow_width(ind, 0, 0);
            }
        }
    }
}
