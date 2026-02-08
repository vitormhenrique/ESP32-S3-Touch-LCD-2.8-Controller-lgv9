/**
 * @file ui_screen_robot.c
 * @brief Robot Selection Screen Implementation
 */

#include "ui_screen_robot.h"

//=============================================================================
// Screen Objects
//=============================================================================

lv_obj_t *ui_RobotSelectScreen = NULL;

static lv_obj_t *ui_RobotTitle = NULL;
static lv_obj_t *ui_RobotCards[ROBOT_TYPE_COUNT] = {NULL};
static lv_obj_t *ui_RobotCheckmarks[ROBOT_TYPE_COUNT] = {NULL};
static lv_obj_t *ui_BtnSave = NULL;

static RobotType_t selected_robot = ROBOT_TYPE_GENERIC;
static robot_select_cb_t robot_callback = NULL;

//=============================================================================
// Robot Info
//=============================================================================

typedef struct {
    const char *name;
    const char *description;
    const char *icon;
} RobotInfo_t;

static const RobotInfo_t robot_info[ROBOT_TYPE_COUNT] = {
    {
        .name = "Generic Robot",
        .description = "Standard RC control\nBasic telemetry",
        .icon = LV_SYMBOL_HOME
    },
    {
        .name = "Hexapod",
        .description = "18x DYNAMIXEL MX-28AR\nIMU + Servo telemetry",
        .icon = LV_SYMBOL_SETTINGS  // Would use custom icon
    }
};

//=============================================================================
// Event Handlers
//=============================================================================

static void robot_card_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *card = lv_event_get_target(e);
        
        // Find which card was clicked
        for (int i = 0; i < ROBOT_TYPE_COUNT; i++) {
            if (card == ui_RobotCards[i] || lv_obj_get_parent(card) == ui_RobotCards[i]) {
                ui_robot_set_selected((RobotType_t)i);
                break;
            }
        }
    }
}

static void save_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (robot_callback) {
            robot_callback(selected_robot);
        }
        ui_robot_screen_show(false);
    }
}

//=============================================================================
// Create Robot Card
//=============================================================================

static lv_obj_t *create_robot_card(lv_obj_t *parent, RobotType_t type)
{
    const RobotInfo_t *info = &robot_info[type];
    
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, UI_SCREEN_WIDTH - 32, 80);
    lv_obj_set_style_bg_color(card, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x2A3A4A), LV_STATE_CHECKED);
    lv_obj_set_style_border_color(card, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(UI_COLOR_ACCENT_BLUE), LV_STATE_CHECKED);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_width(card, 2, LV_STATE_CHECKED);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
    
    lv_obj_add_event_cb(card, robot_card_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Icon
    lv_obj_t *icon = lv_label_create(card);
    lv_label_set_text(icon, info->icon);
    lv_obj_set_style_text_color(icon, lv_color_hex(UI_COLOR_ACCENT_BLUE), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
    
    // Name
    lv_obj_t *name = lv_label_create(card);
    lv_label_set_text(name, info->name);
    lv_obj_set_style_text_color(name, lv_color_hex(UI_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 40, 0);
    
    // Description
    lv_obj_t *desc = lv_label_create(card);
    lv_label_set_text(desc, info->description);
    lv_obj_set_style_text_color(desc, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_10, 0);
    lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 40, 20);
    
    // Checkmark (hidden by default)
    lv_obj_t *check = lv_label_create(card);
    lv_label_set_text(check, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(check, lv_color_hex(UI_COLOR_ACCENT_GREEN), 0);
    lv_obj_set_style_text_font(check, &lv_font_montserrat_16, 0);
    lv_obj_align(check, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_add_flag(check, LV_OBJ_FLAG_HIDDEN);
    
    ui_RobotCheckmarks[type] = check;
    
    return card;
}

//=============================================================================
// Public Functions
//=============================================================================

void ui_robot_screen_create(lv_obj_t *parent)
{
    ui_RobotSelectScreen = lv_obj_create(parent);
    lv_obj_set_size(ui_RobotSelectScreen, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(ui_RobotSelectScreen, 0, 0);
    lv_obj_set_style_bg_color(ui_RobotSelectScreen, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_border_width(ui_RobotSelectScreen, 0, 0);
    lv_obj_remove_flag(ui_RobotSelectScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_RobotSelectScreen, LV_OBJ_FLAG_HIDDEN);
    
    // Title
    ui_RobotTitle = ui_create_label(ui_RobotSelectScreen, "Select Robot");
    lv_obj_set_style_text_font(ui_RobotTitle, &lv_font_montserrat_16, 0);
    lv_obj_align(ui_RobotTitle, LV_ALIGN_TOP_MID, 0, 12);
    
    // Subtitle
    lv_obj_t *subtitle = ui_create_label_secondary(ui_RobotSelectScreen, "Choose your robot profile");
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 36);
    
    // Robot cards container
    lv_obj_t *container = lv_obj_create(ui_RobotSelectScreen);
    lv_obj_set_size(container, UI_SCREEN_WIDTH, 180);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 8, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(container, 8, 0);
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create robot cards
    for (int i = 0; i < ROBOT_TYPE_COUNT; i++) {
        ui_RobotCards[i] = create_robot_card(container, (RobotType_t)i);
    }
    
    // Save button
    ui_BtnSave = ui_create_button(ui_RobotSelectScreen, "Save", 120, 40);
    lv_obj_align(ui_BtnSave, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_add_event_cb(ui_BtnSave, save_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Set default selection
    ui_robot_set_selected(ROBOT_TYPE_GENERIC);
}

void ui_robot_screen_show(bool show)
{
    if (ui_RobotSelectScreen) {
        if (show) {
            lv_obj_remove_flag(ui_RobotSelectScreen, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ui_RobotSelectScreen);
        } else {
            lv_obj_add_flag(ui_RobotSelectScreen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_robot_set_selected(RobotType_t robot_type)
{
    if (robot_type >= ROBOT_TYPE_COUNT) return;
    
    selected_robot = robot_type;
    
    // Update visual state of all cards
    for (int i = 0; i < ROBOT_TYPE_COUNT; i++) {
        if (ui_RobotCards[i]) {
            if (i == robot_type) {
                lv_obj_add_state(ui_RobotCards[i], LV_STATE_CHECKED);
                if (ui_RobotCheckmarks[i]) {
                    lv_obj_remove_flag(ui_RobotCheckmarks[i], LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                lv_obj_remove_state(ui_RobotCards[i], LV_STATE_CHECKED);
                if (ui_RobotCheckmarks[i]) {
                    lv_obj_add_flag(ui_RobotCheckmarks[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    }
}

RobotType_t ui_robot_get_selected(void)
{
    return selected_robot;
}

void ui_robot_set_callback(robot_select_cb_t cb)
{
    robot_callback = cb;
}
