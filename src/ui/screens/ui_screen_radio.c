/**
 * @file ui_screen_radio.c
 * Dynamic ExpressLRS configuration UI (Settings -> Radio).
 *
 * Everything shown here is discovered live from the connected module via
 * the CRSF parameter protocol — no hardcoded parameter numbers or option
 * lists. Works against the real TX module on hardware and against the
 * simulated ES24TX Pro in the desktop simulator.
 */
#include "ui_screen_radio.h"
#include "../ui_helpers.h"
#include "../ui_styles.h"
#include "../../elrs/elrs_client.h"
#include "../../elrs/elrs_service.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//=============================================================================
// State
//=============================================================================
static lv_obj_t *radio_root;
static lv_obj_t *status_label;
static lv_obj_t *btn_tx, *btn_rx;
static lv_obj_t *param_list;
static lv_obj_t *toast;
static lv_timer_t *toast_timer;
static lv_timer_t *poll_timer;

// popups (created on demand on the top layer)
static lv_obj_t *modal_bg;
static lv_obj_t *editor_roller;
static lv_obj_t *num_value_label;
static lv_obj_t *cmd_info_label;
static lv_obj_t *cmd_confirm_btn;
static lv_obj_t *cmd_popup;

static uint8_t current_folder;      // 0 = root
static bool    show_hidden;
static bool    discovery_started;
static uint8_t edit_param_id;
static int32_t num_edit_value;

// pending confirmed action
typedef enum { ACT_NONE, ACT_WRITE, ACT_COMMAND, ACT_PROFILE } PendingAction;
static PendingAction pending_action;
static uint8_t pending_id;
static int32_t pending_value;

static void rebuild_list(void);
static void close_modal(void);

#define SYNTH_TX_PROFILE   (-100)
#define SYNTH_FW_NOTES     (-101)

//=============================================================================
// Toast messages
//=============================================================================
static void toast_hide_cb(lv_timer_t *t)
{
    (void)t;
    if (toast) lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);
    toast_timer = NULL;
}

static void show_toast(ElrsMsgLevel level, const char *text)
{
    if (!toast) return;
    uint32_t color;
    switch (level) {
    case ELRS_MSG_SUCCESS: color = UI_COLOR_ACCENT_GREEN;  break;
    case ELRS_MSG_WARNING: color = UI_COLOR_ACCENT_ORANGE; break;
    case ELRS_MSG_ERROR:   color = UI_COLOR_ACCENT_RED;    break;
    default:               color = UI_COLOR_ACCENT_BLUE;   break;
    }
    lv_obj_set_style_border_color(toast, lv_color_hex(color), 0);
    lv_obj_t *lbl = lv_obj_get_child(toast, 0);
    lv_label_set_text(lbl, text);
    lv_obj_remove_flag(toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast);

    if (toast_timer) lv_timer_delete(toast_timer);
    toast_timer = lv_timer_create(toast_hide_cb, 2600, NULL);
    lv_timer_set_repeat_count(toast_timer, 1);
}

static void service_msg_cb(ElrsMsgLevel level, const char *text, void *user)
{
    (void)user;
    show_toast(level, text);
}

//=============================================================================
// Status bar
//=============================================================================
static void update_status_label(const char *text)
{
    if (status_label) lv_label_set_text(status_label, text);
}

static void update_device_tabs(void)
{
    uint8_t sel = elrs_client_selected_device();
    if (btn_tx) {
        lv_obj_set_style_bg_color(btn_tx,
            lv_color_hex(sel == CRSF_ADDR_TX_MODULE ?
                         UI_COLOR_ACCENT_BLUE : UI_COLOR_BG_CARD), 0);
    }
    if (btn_rx) {
        lv_obj_set_style_bg_color(btn_rx,
            lv_color_hex(sel == CRSF_ADDR_RECEIVER ?
                         UI_COLOR_ACCENT_BLUE : UI_COLOR_BG_CARD), 0);
    }
}

//=============================================================================
// Modal helpers
//=============================================================================
static void modal_bg_click_cb(lv_event_t *e)
{
    (void)e;
    // tap outside closes non-command popups
    if (!cmd_popup) close_modal();
}

static lv_obj_t *open_modal(int w, int h)
{
    modal_bg = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modal_bg, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(modal_bg, 0, 0);
    lv_obj_set_style_bg_color(modal_bg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(modal_bg, 0, 0);
    lv_obj_set_style_radius(modal_bg, 0, 0);
    lv_obj_set_style_pad_all(modal_bg, 0, 0);
    lv_obj_remove_flag(modal_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modal_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(modal_bg, modal_bg_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *panel = lv_obj_create(modal_bg);
    lv_obj_set_size(panel, w, h);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 10, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

static void close_modal(void)
{
    if (modal_bg) {
        lv_obj_delete(modal_bg);
        modal_bg = NULL;
    }
    editor_roller = NULL;
    num_value_label = NULL;
    cmd_info_label = NULL;
    cmd_confirm_btn = NULL;
    cmd_popup = NULL;
}

static lv_obj_t *modal_button(lv_obj_t *parent, const char *text,
                              uint32_t color, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 92, 28);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_radius(btn, 7, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(lbl);
    return btn;
}

//=============================================================================
// Write execution (post-confirmation)
//=============================================================================
static void do_write(uint8_t id, int32_t value)
{
    const ElrsParam *p = elrs_client_param(id);
    if (!p) return;
    if (p->type == CRSF_PT_TEXT_SELECTION) {
        char opt[48] = "?";
        elrs_param_option(p, (int)value, opt, sizeof(opt));
        char buf[80];
        snprintf(buf, sizeof(buf), "Setting %s to %s...", p->name, opt);
        show_toast(ELRS_MSG_INFO, buf);
    } else {
        char buf[80];
        snprintf(buf, sizeof(buf), "Writing %s...", p->name);
        show_toast(ELRS_MSG_INFO, buf);
    }
    elrs_client_write_value(id, value);
}

//=============================================================================
// Command popup
//=============================================================================
static void cmd_cancel_cb(lv_event_t *e)
{
    (void)e;
    elrs_client_command_cancel(pending_id);
    show_toast(ELRS_MSG_INFO, "Command cancelled.");
    close_modal();
}

static void cmd_confirm_cb(lv_event_t *e)
{
    (void)e;
    elrs_client_command_confirm(pending_id);
    if (cmd_confirm_btn) lv_obj_add_flag(cmd_confirm_btn, LV_OBJ_FLAG_HIDDEN);
}

static void open_command_popup(uint8_t id)
{
    const ElrsParam *p = elrs_client_param(id);
    if (!p) return;
    close_modal();
    pending_id = id;

    lv_obj_t *panel = open_modal(270, 140);
    cmd_popup = panel;

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, p->name);
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    cmd_info_label = lv_label_create(panel);
    lv_label_set_text(cmd_info_label, "Command in progress...");
    lv_obj_add_style(cmd_info_label, &style_text_secondary, 0);
    lv_obj_set_width(cmd_info_label, 240);
    lv_label_set_long_mode(cmd_info_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(cmd_info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(cmd_info_label, LV_ALIGN_TOP_MID, 0, 24);

    cmd_confirm_btn = modal_button(panel, "Confirm",
                                   UI_COLOR_ACCENT_GREEN, cmd_confirm_cb);
    lv_obj_align(cmd_confirm_btn, LV_ALIGN_BOTTOM_LEFT, 4, 0);
    lv_obj_add_flag(cmd_confirm_btn, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *cancel = modal_button(panel, "Cancel",
                                    UI_COLOR_BG_CARD, cmd_cancel_cb);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_RIGHT, -4, 0);
}

static void update_command_popup(uint8_t id)
{
    if (!cmd_popup || id != pending_id) return;
    const ElrsParam *p = elrs_client_param(id);
    if (!p) return;

    const char *status_txt;
    switch (p->cmd_status) {
    case CRSF_CMD_PROGRESS:            status_txt = "In progress..."; break;
    case CRSF_CMD_CONFIRMATION_NEEDED: status_txt = "Confirmation required"; break;
    case CRSF_CMD_READY:               status_txt = "Done"; break;
    default:                           status_txt = "Working..."; break;
    }

    char buf[96];
    if (p->cmd_info[0]) {
        snprintf(buf, sizeof(buf), "%s\n%s", p->cmd_info, status_txt);
    } else {
        snprintf(buf, sizeof(buf), "%s", status_txt);
    }
    if (cmd_info_label) lv_label_set_text(cmd_info_label, buf);

    if (cmd_confirm_btn) {
        if (p->cmd_status == CRSF_CMD_CONFIRMATION_NEEDED) {
            lv_obj_remove_flag(cmd_confirm_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(cmd_confirm_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (p->cmd_status == CRSF_CMD_READY) {
        show_toast(ELRS_MSG_SUCCESS, "Command completed.");
        close_modal();
        rebuild_list();
    }
}

//=============================================================================
// Confirmation dialog
//=============================================================================
static void confirm_yes_cb(lv_event_t *e)
{
    (void)e;
    PendingAction act = pending_action;
    pending_action = ACT_NONE;
    close_modal();
    switch (act) {
    case ACT_WRITE:
        do_write(pending_id, pending_value);
        break;
    case ACT_COMMAND:
        if (elrs_client_command_start(pending_id)) {
            open_command_popup(pending_id);
        }
        break;
    case ACT_PROFILE:
        elrs_service_apply_profile((ElrsProfile)pending_value);
        break;
    default:
        break;
    }
}

static void confirm_no_cb(lv_event_t *e)
{
    (void)e;
    pending_action = ACT_NONE;
    close_modal();
}

static void open_confirm_dialog(const char *title_txt, const char *body)
{
    close_modal();
    lv_obj_t *panel = open_modal(276, 150);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, title_txt);
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *msg = lv_label_create(panel);
    lv_label_set_text(msg, body);
    lv_obj_add_style(msg, &style_text_secondary, 0);
    lv_obj_set_width(msg, 248);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 22);

    lv_obj_t *no = modal_button(panel, "Cancel", UI_COLOR_BG_CARD,
                                confirm_no_cb);
    lv_obj_align(no, LV_ALIGN_BOTTOM_LEFT, 4, 0);

    lv_obj_t *yes = modal_button(panel, "Confirm", UI_COLOR_ACCENT_BLUE,
                                 confirm_yes_cb);
    lv_obj_align(yes, LV_ALIGN_BOTTOM_RIGHT, -4, 0);
}

//=============================================================================
// Read-only help/profile pages
//=============================================================================
static void text_page_close_cb(lv_event_t *e)
{
    (void)e;
    close_modal();
}

static void open_text_page(const char *title_txt, const char *body)
{
    close_modal();
    lv_obj_t *panel = open_modal(292, 196);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, title_txt);
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *msg = lv_label_create(panel);
    lv_label_set_text(msg, body);
    lv_obj_add_style(msg, &style_text_secondary, 0);
    lv_obj_set_width(msg, 260);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_align(msg, LV_ALIGN_TOP_LEFT, 4, 24);

    lv_obj_t *close = modal_button(panel, "Close", UI_COLOR_BG_CARD,
                                   text_page_close_cb);
    lv_obj_align(close, LV_ALIGN_BOTTOM_MID, 0, 0);
}

static void append_line(char *buf, size_t len, const char *fmt, ...)
{
    size_t used = strlen(buf);
    if (used >= len - 1) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(&buf[used], len - used, fmt, ap);
    va_end(ap);
}

static void open_tx_profile_page(void)
{
    const ElrsDeviceInfo *dev =
        elrs_client_device_by_addr(CRSF_ADDR_TX_MODULE);
    const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
    uint16_t discovered_max = elrs_service_discovered_max_power_mw();
    bool beta = profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_1W ||
                profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_500MW ||
                profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN;
    char body[900] = "";
    append_line(body, sizeof(body), "Detected family: %s\n",
                elrs_service_family_display_name(profile->family));
    append_line(body, sizeof(body), "Device name: %s\n",
                dev ? dev->name : "unknown");
    append_line(body, sizeof(body), "Configurator category: %s\n",
                profile->expectedConfiguratorCategory[0] ?
                profile->expectedConfiguratorCategory : "unknown");
    append_line(body, sizeof(body), "Configurator target: %s\n",
                profile->expectedConfiguratorTarget[0] ?
                profile->expectedConfiguratorTarget : "unknown");
    append_line(body, sizeof(body), "Target: %s\n",
                profile->expectedFirmwareTarget[0] ?
                profile->expectedFirmwareTarget : "discover from firmware");
    append_line(body, sizeof(body), "Expected max power: %umW\n",
                (unsigned)profile->maxExpectedPowerMw);
    append_line(body, sizeof(body), "Power: discovered up to %umW\n",
                (unsigned)discovered_max);
    append_line(body, sizeof(body), "Input voltage note: %uV-%uV\n",
                (unsigned)profile->expectedMinInputVoltage,
                (unsigned)profile->expectedMaxInputVoltage);
    append_line(body, sizeof(body), "Fan expected/discovered: %s/%s\n",
                profile->hasFanExpected ? "yes" : "maybe/no",
                elrs_service_tx_feature_discovered("fan") ? "yes" : "no");
    append_line(body, sizeof(body), "Backpack expected/discovered: %s/%s\n",
                profile->hasBackpackExpected ? "yes" : "maybe/no",
                elrs_service_tx_feature_discovered("backpack") ? "yes" : "no");
    append_line(body, sizeof(body), "OLED/5D expected/discovered: %s/%s\n",
                (profile->hasOledExpected || profile->hasFiveDButtonExpected) ?
                "yes" : "not expected",
                (elrs_service_tx_feature_discovered("oled") ||
                 elrs_service_tx_feature_discovered("5d")) ? "yes" : "no");
    append_line(body, sizeof(body), "Module local UI: %s\n",
                (profile->hasOledExpected || profile->hasFiveDButtonExpected) ?
                "OLED/5D present" : "not expected");
    append_line(body, sizeof(body), "CRSF baud: configured in controller\n");
    if (beta) {
        append_line(body, sizeof(body), "BETAFPV: do not use 3S+ on XT30\n");
        append_line(body, sizeof(body), "BETAFPV local OLED menu may also change ELRS settings.\n");
        append_line(body, sizeof(body), "If settings disagree, refresh parameters from TX module.\n");
    } else if (profile->family == ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO) {
        append_line(body, sizeof(body), "Use 5V-10V supply range. Install antenna before RF output.\n");
    }
    if (profile->family == ELRS_TX_FAMILY_UNKNOWN ||
        profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN ||
        profile->family == ELRS_TX_FAMILY_OTHER_ELRS_TX) {
        append_line(body, sizeof(body), "Family uncertain; using discovered options\n");
    }
    open_text_page("TX Module Profile", body);
}

static void open_fw_notes_page(void)
{
    const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
    bool beta = profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_1W ||
                profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_500MW ||
                profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN;
    char body[900] = "";
    append_line(body, sizeof(body), "Firmware flashing/build options are handled by ExpressLRS Configurator/WebUI.\n");
    append_line(body, sizeof(body), "Binding Phrase, Regulatory Domain, Home WiFi credentials are build/WebUI settings unless exposed at runtime.\n\n");
    if (beta) {
        append_line(body, sizeof(body), "Configurator category: BETAFPV 2.4 GHz\n");
        append_line(body, sizeof(body), "Device: BETAFPV 2.4GHz Micro TX or BETAFPV 2.4GHz 1W Micro TX\n");
        append_line(body, sizeof(body), "Older modules may ship with BETAFPV-custom ELRS V2.0.0-style OLED/5D firmware.\n");
        append_line(body, sizeof(body), "BETAFPV provides 2.5.1 and V3.3.0 module bins for Micro 500mW/1W.\n");
        append_line(body, sizeof(body), "BETAFPV V3.3.0 notes external TX protocol baud may need 921K or higher for Lua/script access.\n");
    } else {
        append_line(body, sizeof(body), "Configurator category: Happymodel 2.4 GHz\n");
        append_line(body, sizeof(body), "Device: HappyModel ES24 Pro 2.4GHz TX\n");
        append_line(body, sizeof(body), "Target: HappyModel_ES24TX_Pro_Series_2400_TX\n");
    }
    append_line(body, sizeof(body), "If updating older/factory firmware to 3.x over WiFi, use 2.5.2 then Repartitioner before 3.x WiFi flash.\n");
    append_line(body, sizeof(body), "UART/ETX passthrough update may not require that WiFi path.\n");
    open_text_page("Firmware Target / Update Notes", body);
}

//=============================================================================
// Write request (safety checks + optional confirmation)
//=============================================================================
static void request_write(uint8_t id, int32_t value)
{
    const ElrsParam *p = elrs_client_param(id);
    if (!p) return;

    int opt_index = (p->type == CRSF_PT_TEXT_SELECTION) ? (int)value : -1;
    ElrsWriteSafety safety = elrs_service_classify(p, opt_index);

    if (safety.needs_disarm && elrs_service_is_armed()) {
        show_toast(ELRS_MSG_ERROR,
                   "Blocked: cannot change RF settings while armed.");
        return;
    }

    if (safety.needs_confirm || safety.warning[0]) {
        char body[160];
        char opt[48] = "";
        if (p->type == CRSF_PT_TEXT_SELECTION) {
            elrs_param_option(p, (int)value, opt, sizeof(opt));
        }
        if (safety.warning[0]) {
            snprintf(body, sizeof(body), "%s\n\nChange %s to %s?",
                     safety.warning, p->name, opt);
        } else {
            snprintf(body, sizeof(body), "Change %s to %s?", p->name, opt);
        }
        pending_action = ACT_WRITE;
        pending_id = id;
        pending_value = value;
        open_confirm_dialog("Confirm change", body);
    } else {
        do_write(id, value);
    }
}

static void request_command(uint8_t id)
{
    const ElrsParam *p = elrs_client_param(id);
    if (!p) return;

    ElrsWriteSafety safety = elrs_service_classify(p, -1);
    if (safety.needs_disarm && elrs_service_is_armed()) {
        show_toast(ELRS_MSG_ERROR,
                   "Blocked: cannot run commands while armed.");
        return;
    }
    char body[96];
    snprintf(body, sizeof(body), "Run '%s' now?", p->name);
    pending_action = ACT_COMMAND;
    pending_id = id;
    open_confirm_dialog("Run command", body);
}

//=============================================================================
// Option / numeric editors
//=============================================================================
static void editor_ok_cb(lv_event_t *e)
{
    (void)e;
    if (!editor_roller) return;
    int idx = (int)lv_roller_get_selected(editor_roller);
    uint8_t id = edit_param_id;
    close_modal();
    const ElrsParam *p = elrs_client_param(id);
    if (p && (int32_t)idx != p->value) request_write(id, idx);
}

static void editor_cancel_cb(lv_event_t *e)
{
    (void)e;
    close_modal();
}

static void open_option_editor(const ElrsParam *p)
{
    close_modal();
    edit_param_id = p->id;

    lv_obj_t *panel = open_modal(250, 190);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, p->name);
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // convert ";" separated options to "\n" for the roller
    static char opts[ELRS_OPTS_LEN];
    strncpy(opts, p->options, sizeof(opts) - 1);
    opts[sizeof(opts) - 1] = '\0';
    for (char *ch = opts; *ch; ch++) {
        if (*ch == ';') *ch = '\n';
    }

    editor_roller = lv_roller_create(panel);
    lv_roller_set_options(editor_roller, opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(editor_roller, 3);
    lv_obj_set_width(editor_roller, 200);
    lv_obj_align(editor_roller, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_bg_color(editor_roller, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_set_style_text_color(editor_roller,
                                lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_color(editor_roller,
                              lv_color_hex(UI_COLOR_ACCENT_BLUE),
                              LV_PART_SELECTED);
    if (p->value >= 0 && p->value <= p->max) {
        lv_roller_set_selected(editor_roller, (uint16_t)p->value, LV_ANIM_OFF);
    }

    lv_obj_t *no = modal_button(panel, "Cancel", UI_COLOR_BG_CARD,
                                editor_cancel_cb);
    lv_obj_align(no, LV_ALIGN_BOTTOM_LEFT, 4, 0);

    lv_obj_t *ok = modal_button(panel, "Set", UI_COLOR_ACCENT_BLUE,
                                editor_ok_cb);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_RIGHT, -4, 0);
}

// numeric editor -------------------------------------------------------------
static void num_update_label(void)
{
    const ElrsParam *p = elrs_client_param(edit_param_id);
    if (!p || !num_value_label) return;
    char buf[32];
    if (p->type == CRSF_PT_FLOAT && p->precision > 0) {
        int32_t div = 1;
        for (int i = 0; i < p->precision; i++) div *= 10;
        snprintf(buf, sizeof(buf), "%ld.%0*ld %s",
                 (long)(num_edit_value / div), p->precision,
                 (long)(num_edit_value % div < 0 ?
                        -(num_edit_value % div) : num_edit_value % div),
                 p->unit);
    } else {
        snprintf(buf, sizeof(buf), "%ld %s", (long)num_edit_value, p->unit);
    }
    lv_label_set_text(num_value_label, buf);
}

static void num_step(int dir)
{
    const ElrsParam *p = elrs_client_param(edit_param_id);
    if (!p) return;
    int32_t step = (p->type == CRSF_PT_FLOAT && p->step > 0) ? p->step : 1;
    int32_t v = num_edit_value + dir * step;
    if (v < p->min) v = p->min;
    if (v > p->max) v = p->max;
    num_edit_value = v;
    num_update_label();
}

static void num_minus_cb(lv_event_t *e) { (void)e; num_step(-1); }
static void num_plus_cb(lv_event_t *e)  { (void)e; num_step(1); }

static void num_ok_cb(lv_event_t *e)
{
    (void)e;
    uint8_t id = edit_param_id;
    int32_t v = num_edit_value;
    close_modal();
    const ElrsParam *p = elrs_client_param(id);
    if (p && v != p->value) request_write(id, v);
}

static void open_numeric_editor(const ElrsParam *p)
{
    close_modal();
    edit_param_id = p->id;
    num_edit_value = p->value;

    lv_obj_t *panel = open_modal(250, 160);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, p->name);
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *minus = lv_button_create(panel);
    lv_obj_set_size(minus, 40, 34);
    lv_obj_align(minus, LV_ALIGN_TOP_LEFT, 14, 30);
    lv_obj_set_style_bg_color(minus, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_add_event_cb(minus, num_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ml = lv_label_create(minus);
    lv_label_set_text(ml, LV_SYMBOL_MINUS);
    lv_obj_center(ml);

    num_value_label = lv_label_create(panel);
    lv_obj_add_style(num_value_label, &style_text_primary, 0);
    lv_obj_set_style_text_font(num_value_label, &lv_font_montserrat_14, 0);
    lv_obj_align(num_value_label, LV_ALIGN_TOP_MID, 0, 38);

    lv_obj_t *plus = lv_button_create(panel);
    lv_obj_set_size(plus, 40, 34);
    lv_obj_align(plus, LV_ALIGN_TOP_RIGHT, -14, 30);
    lv_obj_set_style_bg_color(plus, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_add_event_cb(plus, num_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(plus);
    lv_label_set_text(pl, LV_SYMBOL_PLUS);
    lv_obj_center(pl);

    num_update_label();

    lv_obj_t *no = modal_button(panel, "Cancel", UI_COLOR_BG_CARD,
                                editor_cancel_cb);
    lv_obj_align(no, LV_ALIGN_BOTTOM_LEFT, 4, 0);

    lv_obj_t *ok = modal_button(panel, "Set", UI_COLOR_ACCENT_BLUE,
                                num_ok_cb);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_RIGHT, -4, 0);
}

//=============================================================================
// Presets popup
//=============================================================================
static void preset_cb(lv_event_t *e)
{
    int profile = (int)(intptr_t)lv_event_get_user_data(e);
    close_modal();
    if (profile == ELRS_PROFILE_HIGH_POWER) {
        pending_action = ACT_PROFILE;
        pending_value = profile;
        open_confirm_dialog("High power",
            "This sets Max Power to 1000mW. Significant heat — ensure the "
            "antenna is connected and the fan works. Continue?");
    } else {
        elrs_service_apply_profile((ElrsProfile)profile);
    }
}

static void open_presets_popup(void)
{
    close_modal();
    lv_obj_t *panel = open_modal(240, 196);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "Robot ELRS Defaults");
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    static const struct { const char *name; uint32_t color; int profile; }
    presets[] = {
        { "Bench (low power)",  UI_COLOR_ACCENT_GREEN,  ELRS_PROFILE_BENCH },
        { "Field (mid power)",  UI_COLOR_ACCENT_BLUE,   ELRS_PROFILE_FIELD },
        { "High power (1W)",    UI_COLOR_ACCENT_ORANGE, ELRS_PROFILE_HIGH_POWER },
    };

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, 200, 30);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 24 + i * 38);
        lv_obj_set_style_bg_color(btn, lv_color_hex(presets[i].color), 0);
        lv_obj_set_style_radius(btn, 7, 0);
        lv_obj_add_event_cb(btn, preset_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)presets[i].profile);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, presets[i].name);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
    }

    lv_obj_t *cancel = modal_button(panel, "Cancel", UI_COLOR_BG_CARD,
                                    editor_cancel_cb);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_MID, 0, 0);
}

//=============================================================================
// Parameter list
//=============================================================================
static void format_param_value(const ElrsParam *p, char *buf, size_t len)
{
    switch (p->type) {
    case CRSF_PT_TEXT_SELECTION: {
        char opt[40] = "?";
        elrs_param_option(p, (int)p->value, opt, sizeof(opt));
        snprintf(buf, len, "%s%s%s", opt,
                 p->unit[0] ? " " : "", p->unit);
        break;
    }
    case CRSF_PT_UINT8: case CRSF_PT_INT8:
    case CRSF_PT_UINT16: case CRSF_PT_INT16:
        snprintf(buf, len, "%ld%s%s", (long)p->value,
                 p->unit[0] ? " " : "", p->unit);
        break;
    case CRSF_PT_FLOAT: {
        int32_t div = 1;
        for (int i = 0; i < p->precision; i++) div *= 10;
        if (div > 1) {
            snprintf(buf, len, "%ld.%0*ld%s%s",
                     (long)(p->value / div), p->precision,
                     (long)(p->value % div < 0 ?
                            -(p->value % div) : p->value % div),
                     p->unit[0] ? " " : "", p->unit);
        } else {
            snprintf(buf, len, "%ld%s%s", (long)p->value,
                     p->unit[0] ? " " : "", p->unit);
        }
        break;
    }
    case CRSF_PT_STRING:
    case CRSF_PT_INFO:
        snprintf(buf, len, "%s", p->str_value);
        break;
    case CRSF_PT_FOLDER:
        snprintf(buf, len, LV_SYMBOL_RIGHT);
        break;
    case CRSF_PT_COMMAND:
        snprintf(buf, len, LV_SYMBOL_PLAY);
        break;
    default:
        snprintf(buf, len, "-");
        break;
    }
}

static void row_click_cb(lv_event_t *e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);

    if (id == SYNTH_TX_PROFILE) {
        open_tx_profile_page();
        return;
    }
    if (id == SYNTH_FW_NOTES) {
        open_fw_notes_page();
        return;
    }

    if (id == 0) {
        // back row: go up one folder level
        const ElrsParam *folder = elrs_client_param(current_folder);
        current_folder = folder ? folder->parent : 0;
        rebuild_list();
        return;
    }

    const ElrsParam *p = elrs_client_param((uint8_t)id);
    if (!p) return;

    switch (p->type) {
    case CRSF_PT_FOLDER:
        current_folder = p->id;
        rebuild_list();
        break;
    case CRSF_PT_TEXT_SELECTION:
        open_option_editor(p);
        break;
    case CRSF_PT_UINT8: case CRSF_PT_INT8:
    case CRSF_PT_UINT16: case CRSF_PT_INT16:
    case CRSF_PT_FLOAT:
        open_numeric_editor(p);
        break;
    case CRSF_PT_COMMAND:
        request_command(p->id);
        break;
    default:
        break; // STRING / INFO are read-only
    }
}

static lv_obj_t *add_row(const char *name, const char *value,
                         uint32_t value_color, bool clickable, int id)
{
    lv_obj_t *row = lv_obj_create(param_list);
    lv_obj_set_size(row, lv_pct(100), 30);
    lv_obj_set_style_bg_color(row, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(UI_COLOR_BORDER), 0);
    lv_obj_set_style_radius(row, 7, 0);
    lv_obj_set_style_pad_hor(row, 8, 0);
    lv_obj_set_style_pad_ver(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    if (clickable) {
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_COLOR_BORDER),
                                  LV_STATE_PRESSED);
        lv_obj_add_event_cb(row, row_click_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)id);
    }

    lv_obj_t *nl = lv_label_create(row);
    lv_label_set_text(nl, name);
    lv_obj_add_style(nl, &style_text_primary, 0);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_12, 0);
    lv_obj_align(nl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(nl, 160);

    lv_obj_t *vl = lv_label_create(row);
    lv_label_set_text(vl, value);
    lv_obj_set_style_text_color(vl, lv_color_hex(value_color), 0);
    lv_obj_set_style_text_font(vl, &lv_font_montserrat_12, 0);
    lv_obj_align(vl, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_label_set_long_mode(vl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(vl, 110);
    lv_obj_set_style_text_align(vl, LV_TEXT_ALIGN_RIGHT, 0);

    return row;
}

static void rebuild_list(void)
{
    if (!param_list) return;
    lv_obj_clean(param_list);

    if (!elrs_client_params_ready()) {
        add_row("Waiting for module...", "", UI_COLOR_TEXT_SECONDARY,
                false, -1);
        return;
    }

    // "up" row while inside a folder
    if (current_folder != 0) {
        const ElrsParam *folder = elrs_client_param(current_folder);
        char name[40];
        snprintf(name, sizeof(name), LV_SYMBOL_LEFT "  %s",
                 folder ? folder->name : "Back");
        add_row(name, "", UI_COLOR_ACCENT_BLUE, true, 0);
    }

    if (current_folder == 0 &&
        elrs_client_selected_device() == CRSF_ADDR_TX_MODULE) {
        add_row("TX Module Profile", LV_SYMBOL_RIGHT,
                UI_COLOR_ACCENT_BLUE, true, SYNTH_TX_PROFILE);
        add_row("Firmware Target / Update Notes", LV_SYMBOL_RIGHT,
                UI_COLOR_ACCENT_BLUE, true, SYNTH_FW_NOTES);
    }

    uint8_t count = elrs_client_param_count();
    for (uint8_t id = 1; id <= count; id++) {
        const ElrsParam *p = elrs_client_param(id);
        if (!p || p->parent != current_folder) continue;
        if (p->hidden && !show_hidden) continue;
        if (p->type == CRSF_PT_OUT_OF_RANGE) continue;

        char value[64];
        format_param_value(p, value, sizeof(value));

        uint32_t color = UI_COLOR_ACCENT_BLUE;
        bool clickable = true;
        if (p->type == CRSF_PT_INFO || p->type == CRSF_PT_STRING) {
            color = UI_COLOR_TEXT_SECONDARY;
            clickable = false;
        } else if (p->type == CRSF_PT_COMMAND) {
            color = UI_COLOR_ACCENT_GREEN;
        } else if (p->type == CRSF_PT_FOLDER) {
            color = UI_COLOR_TEXT_SECONDARY;
        }
        if (p->hidden) color = UI_COLOR_ACCENT_PURPLE;

        add_row(p->name, value, color, clickable, p->id);
    }
}

//=============================================================================
// Client / service event plumbing
//=============================================================================
static void client_event_cb(ElrsClientEvent ev, uint8_t arg, void *user)
{
    (void)user;
    elrs_service_on_client_event(ev, arg);

    switch (ev) {
    case ELRS_EV_DEVICE_FOUND: {
        const ElrsDeviceInfo *dev = elrs_client_device(arg);
        if (dev && dev->address == elrs_client_selected_device()) {
            char buf[48];
            snprintf(buf, sizeof(buf), "%s", dev->name);
            update_status_label(buf);
        }
        break;
    }
    case ELRS_EV_LOAD_PROGRESS: {
        char buf[48];
        snprintf(buf, sizeof(buf), "Loading %u/%u...",
                 (unsigned)arg, (unsigned)elrs_client_param_count());
        update_status_label(buf);
        break;
    }
    case ELRS_EV_PARAMS_LOADED: {
        const ElrsDeviceInfo *dev =
            elrs_client_device_by_addr(elrs_client_selected_device());
        char buf[64];
        if (elrs_client_selected_device() == CRSF_ADDR_TX_MODULE) {
            const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
            snprintf(buf, sizeof(buf), "Detected: %s", profile->displayName);
        } else {
            snprintf(buf, sizeof(buf), "%s",
                     dev ? dev->name : "Connected");
        }
        update_status_label(buf);
        char toast_buf[64];
        snprintf(toast_buf, sizeof(toast_buf),
                 "Settings loaded (%u parameters).",
                 (unsigned)elrs_client_param_count());
        show_toast(ELRS_MSG_SUCCESS, toast_buf);
        elrs_service_check_module();
        current_folder = 0;
        rebuild_list();
        break;
    }
    case ELRS_EV_PARAM_UPDATED:
        rebuild_list();
        update_command_popup(arg);
        break;
    case ELRS_EV_WRITE_VERIFIED:
    case ELRS_EV_WRITE_FAILED:
        rebuild_list();
        break;
    case ELRS_EV_CMD_STATUS:
        update_command_popup(arg);
        break;
    case ELRS_EV_TIMEOUT:
        update_status_label("Module not responding");
        show_toast(ELRS_MSG_ERROR,
                   "TX module not responding. Check wiring/baud.");
        rebuild_list();
        break;
    default:
        break;
    }
}

static void poll_timer_cb(lv_timer_t *t)
{
    (void)t;
    uint32_t now = lv_tick_get();
    elrs_client_poll(now);
    elrs_service_poll(now);
}

//=============================================================================
// Top bar buttons
//=============================================================================
static void device_tab_cb(lv_event_t *e)
{
    uint8_t addr = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    if (elrs_client_selected_device() == addr &&
        elrs_client_params_ready()) return;
    current_folder = 0;
    update_status_label(addr == CRSF_ADDR_TX_MODULE ?
                        "Reading TX module..." : "Reading receiver...");
    elrs_client_select_device(addr);
    update_device_tabs();
    rebuild_list();
}

static void refresh_cb(lv_event_t *e)
{
    (void)e;
    current_folder = 0;
    update_status_label("Reloading...");
    show_toast(ELRS_MSG_INFO, "Reading settings from module...");
    elrs_client_select_device(elrs_client_selected_device() ?
                              elrs_client_selected_device() :
                              CRSF_ADDR_TX_MODULE);
    rebuild_list();
}

static void hidden_toggle_cb(lv_event_t *e)
{
    (void)e;
    show_hidden = !show_hidden;
    show_toast(ELRS_MSG_INFO, show_hidden ?
               "Advanced: showing hidden parameters." :
               "Hidden parameters concealed.");
    rebuild_list();
}

static void presets_cb(lv_event_t *e)
{
    (void)e;
    if (!elrs_client_params_ready()) {
        show_toast(ELRS_MSG_ERROR, "Settings not loaded yet.");
        return;
    }
    open_presets_popup();
}

static lv_obj_t *icon_button(lv_obj_t *parent, const char *icon,
                             lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 26, 22);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, icon);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(lbl);
    return btn;
}

//=============================================================================
// Public API
//=============================================================================
void ui_radio_menu_create(lv_obj_t *parent)
{
    radio_root = parent;

    // ---- top bar --------------------------------------------------------
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, lv_pct(100), 26);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 26);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bar, 4, 0);

    // device tabs
    btn_tx = lv_button_create(bar);
    lv_obj_set_size(btn_tx, 32, 22);
    lv_obj_set_style_radius(btn_tx, 6, 0);
    lv_obj_set_style_pad_all(btn_tx, 0, 0);
    lv_obj_add_event_cb(btn_tx, device_tab_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)CRSF_ADDR_TX_MODULE);
    lv_obj_t *txl = lv_label_create(btn_tx);
    lv_label_set_text(txl, "TX");
    lv_obj_set_style_text_font(txl, &lv_font_montserrat_12, 0);
    lv_obj_center(txl);

    btn_rx = lv_button_create(bar);
    lv_obj_set_size(btn_rx, 32, 22);
    lv_obj_set_style_radius(btn_rx, 6, 0);
    lv_obj_set_style_pad_all(btn_rx, 0, 0);
    lv_obj_add_event_cb(btn_rx, device_tab_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)CRSF_ADDR_RECEIVER);
    lv_obj_t *rxl = lv_label_create(btn_rx);
    lv_label_set_text(rxl, "RX");
    lv_obj_set_style_text_font(rxl, &lv_font_montserrat_12, 0);
    lv_obj_center(rxl);

    // status label (grows)
    status_label = lv_label_create(bar);
    lv_label_set_text(status_label, "Not connected");
    lv_obj_add_style(status_label, &style_text_secondary, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(status_label, 1);

    // action buttons
    icon_button(bar, LV_SYMBOL_EYE_OPEN, hidden_toggle_cb, NULL);
    icon_button(bar, LV_SYMBOL_CHARGE, presets_cb, NULL);
    icon_button(bar, LV_SYMBOL_REFRESH, refresh_cb, NULL);

    // ---- parameter list --------------------------------------------------
    param_list = lv_obj_create(parent);
    lv_obj_set_size(param_list, lv_pct(100),
                    UI_CONTENT_HEIGHT - 26 - 26 - 6);
    lv_obj_align(param_list, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(param_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(param_list, 0, 0);
    lv_obj_set_style_pad_all(param_list, 0, 0);
    lv_obj_set_style_pad_row(param_list, 4, 0);
    lv_obj_set_flex_flow(param_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(param_list, LV_DIR_VER);

    // ---- toast -----------------------------------------------------------
    toast = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toast, 300, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -42);
    lv_obj_set_style_bg_color(toast, lv_color_hex(UI_COLOR_BG_PANEL), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_90, 0);
    lv_obj_set_style_border_width(toast, 1, 0);
    lv_obj_set_style_radius(toast, 8, 0);
    lv_obj_set_style_pad_all(toast, 6, 0);
    lv_obj_remove_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *toast_lbl = lv_label_create(toast);
    lv_label_set_text(toast_lbl, "");
    lv_obj_add_style(toast_lbl, &style_text_primary, 0);
    lv_obj_set_style_text_font(toast_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_width(toast_lbl, 286);
    lv_label_set_long_mode(toast_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(toast_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // ---- wiring -----------------------------------------------------------
    elrs_service_init();
    elrs_service_set_msg_cb(service_msg_cb, NULL);
    elrs_client_set_event_cb(client_event_cb, NULL);
    poll_timer = lv_timer_create(poll_timer_cb, 20, NULL);

    update_device_tabs();
    rebuild_list();
}

void ui_radio_on_show(void)
{
    if (!discovery_started) {
        discovery_started = true;
        update_status_label("Reading TX module...");
        show_toast(ELRS_MSG_INFO, "Reading settings from TX module...");
        elrs_client_select_device(CRSF_ADDR_TX_MODULE);
        update_device_tabs();
    }
}
