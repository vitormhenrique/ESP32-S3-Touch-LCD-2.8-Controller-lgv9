/**
 * @file elrs_service.c
 * High-level ExpressLRS settings service. See elrs_service.h.
 */
#include "elrs_service.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>

//=============================================================================
// State
//=============================================================================
#define PROFILE_MAX_STEPS 8

typedef struct {
    const char *param;
    const char *option;
} ProfileStep;

static struct {
    ElrsMsgFn   msg_cb;
    void       *msg_user;
    ElrsArmedFn armed_cb;
    void       *armed_user;
    uint32_t    baud;

    // profile sequencing
    bool        profile_active;
    ProfileStep steps[PROFILE_MAX_STEPS];
    int         step_count;
    int         step_index;
    uint8_t     pending_write_id;   // param id we're waiting on
    uint32_t    step_deadline_ms;
    uint32_t    now_ms;
    bool        step_in_flight;
} s;

static void msg(ElrsMsgLevel lvl, const char *fmt, ...)
{
    if (!s.msg_cb) return;
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    s.msg_cb(lvl, buf, s.msg_user);
}

static bool is_armed(void)
{
    return s.armed_cb ? s.armed_cb(s.armed_user) : false;
}

bool elrs_service_is_armed(void) { return is_armed(); }

//=============================================================================
// Name normalization + aliases
//=============================================================================
void elrs_normalize_name(const char *in, char *out, size_t out_len)
{
    size_t o = 0;
    for (const char *p = in; *p && o < out_len - 1; p++) {
        char ch = *p;
        if (ch == ' ' || ch == '_' || ch == '-' || ch == '/' || ch == '>')
            continue;
        out[o++] = (char)tolower((unsigned char)ch);
    }
    out[o] = '\0';
}

// alias pairs: user-facing name -> ELRS parameter name (both normalized)
static const struct { const char *alias; const char *canon; } k_aliases[] = {
    { "telemetryratio", "telemratio" },
    { "packetrate",     "packetrate" },
    { "maxtxpower",     "maxpower" },
    { "fanthreshold",   "fanthresh" },
    { "wifi",           "enablewifi" },
    { "txwifi",         "enablewifi" },
    { "rxwifi",         "enablerxwifi" },
    { "bindmode",       "enterbindmode" },
    { "modelid",        "modelmatch" },
};

const ElrsParam *elrs_service_find(const char *name)
{
    char want[48];
    elrs_normalize_name(name, want, sizeof(want));

    // apply alias mapping
    for (size_t i = 0; i < sizeof(k_aliases) / sizeof(k_aliases[0]); i++) {
        if (strcmp(want, k_aliases[i].alias) == 0) {
            strncpy(want, k_aliases[i].canon, sizeof(want) - 1);
            want[sizeof(want) - 1] = '\0';
            break;
        }
    }

    uint8_t count = elrs_client_param_count();
    for (uint8_t id = 1; id <= count; id++) {
        const ElrsParam *p = elrs_client_param(id);
        if (!p) continue;
        char have[48];
        elrs_normalize_name(p->name, have, sizeof(have));
        if (strcmp(want, have) == 0) return p;
    }
    return NULL;
}

int elrs_service_find_option(const ElrsParam *p, const char *option_text)
{
    if (!p || p->type != CRSF_PT_TEXT_SELECTION) return -1;

    char want[48];
    elrs_normalize_name(option_text, want, sizeof(want));

    int count = elrs_param_option_count(p);
    char opt[48], norm[48];

    // pass 1: exact normalized match
    for (int i = 0; i < count; i++) {
        if (!elrs_param_option(p, i, opt, sizeof(opt))) continue;
        elrs_normalize_name(opt, norm, sizeof(norm));
        if (strcmp(norm, want) == 0) return i;
    }
    // pass 2: prefix match (e.g. "500" matches "500mw", "100hzfull" etc.)
    for (int i = 0; i < count; i++) {
        if (!elrs_param_option(p, i, opt, sizeof(opt))) continue;
        elrs_normalize_name(opt, norm, sizeof(norm));
        if (strncmp(norm, want, strlen(want)) == 0) return i;
    }
    return -1;
}

//=============================================================================
// Safety classification
//=============================================================================
static bool name_is(const ElrsParam *p, const char *n)
{
    char a[48], b[48];
    elrs_normalize_name(p->name, a, sizeof(a));
    elrs_normalize_name(n, b, sizeof(b));
    return strcmp(a, b) == 0;
}

/** Parse the numeric prefix of an option ("333Hz Full" -> 333). */
static int option_number(const ElrsParam *p, int opt_index)
{
    char opt[48];
    if (!elrs_param_option(p, opt_index, opt, sizeof(opt))) return -1;
    if (!isdigit((unsigned char)opt[0])) return -1;
    return atoi(opt);
}

ElrsWriteSafety elrs_service_classify(const ElrsParam *p, int opt_index)
{
    ElrsWriteSafety r;
    memset(&r, 0, sizeof(r));
    if (!p) return r;

    if (name_is(p, "Packet Rate")) {
        r.needs_disarm = true;
        r.needs_confirm = true;
        int hz = option_number(p, opt_index);
        char opt[48] = "";
        elrs_param_option(p, opt_index, opt, sizeof(opt));
        bool full = strstr(opt, "Full") != NULL;
        if (hz >= 333 && full && s.baud > 0 && s.baud <= 400000) {
            snprintf(r.warning, sizeof(r.warning),
                     "Warning: %s requires CRSF baud > 400K. Current baud "
                     "%lu may cause channel glitches.",
                     opt, (unsigned long)s.baud);
        }
    } else if (name_is(p, "Telem Ratio")) {
        char opt[48] = "";
        elrs_param_option(p, opt_index, opt, sizeof(opt));
        if (strcmp(opt, "Off") == 0) {
            snprintf(r.warning, sizeof(r.warning),
                     "Warning: telemetry Off disables link stats and "
                     "dynamic power.");
        } else if (strcmp(opt, "Race") == 0) {
            snprintf(r.warning, sizeof(r.warning),
                     "Warning: Race ratio minimizes telemetry.");
        }
    } else if (name_is(p, "Switch Mode")) {
        r.needs_disarm = true;
        r.needs_confirm = true;
    } else if (name_is(p, "Max Power")) {
        int mw = option_number(p, opt_index);
        if (mw >= 500) {
            r.needs_disarm = true;
            r.needs_confirm = true;
            snprintf(r.warning, sizeof(r.warning),
                     "Warning: %dmW is high power. Ensure antenna is "
                     "connected and expect significant heat.", mw);
        } else if (mw >= 250) {
            snprintf(r.warning, sizeof(r.warning),
                     "Warning: %dmW+ increases heat. Check module fan.", mw);
        }
    } else if (name_is(p, "Model Match") || name_is(p, "Link Mode")) {
        r.needs_disarm = true;
        r.needs_confirm = true;
    } else if (name_is(p, "Protocol") || name_is(p, "Protocol 2")) {
        r.needs_disarm = true;
        r.needs_confirm = true;
        char opt[48] = "";
        elrs_param_option(p, opt_index, opt, sizeof(opt));
        if (strstr(opt, "CRSF") == NULL) {
            snprintf(r.warning, sizeof(r.warning),
                     "Warning: non-CRSF protocol (%s) will break telemetry "
                     "to this controller.", opt);
        }
    } else if (p->type == CRSF_PT_COMMAND) {
        // bind / wifi / ble commands: block while armed
        r.needs_disarm = true;
        r.needs_confirm = true;
    }

    return r;
}

//=============================================================================
// High-level named write
//=============================================================================
bool elrs_service_set(const char *param_name, const char *option_text)
{
    const ElrsParam *p = elrs_service_find(param_name);
    if (!p) {
        msg(ELRS_MSG_ERROR, "'%s' not offered by this module.", param_name);
        return false;
    }
    int idx = elrs_service_find_option(p, option_text);
    if (idx < 0) {
        msg(ELRS_MSG_ERROR, "'%s' is not a valid option for %s.",
            option_text, p->name);
        return false;
    }

    ElrsWriteSafety safety = elrs_service_classify(p, idx);
    if (safety.needs_disarm && is_armed()) {
        msg(ELRS_MSG_ERROR,
            "Blocked: cannot change RF settings while armed.");
        return false;
    }

    char opt[48];
    elrs_param_option(p, idx, opt, sizeof(opt));
    msg(ELRS_MSG_INFO, "Setting %s to %s...", p->name, opt);
    return elrs_client_write_value(p->id, idx);
}

//=============================================================================
// Module identity check
//=============================================================================
void elrs_service_check_module(void)
{
    const ElrsDeviceInfo *dev =
        elrs_client_device_by_addr(CRSF_ADDR_TX_MODULE);
    if (!dev || !dev->present) return;

    char norm[64];
    elrs_normalize_name(dev->name, norm, sizeof(norm));
    if (strstr(norm, "es24tx") == NULL) {
        msg(ELRS_MSG_WARNING,
            "Module is not ES24TX Pro; using discovered options.");
    }
}

//=============================================================================
// Profiles
//=============================================================================
static const ProfileStep k_bench[] = {
    { "Packet Rate",  "100Hz Full" },
    { "Switch Mode",  "16ch Rate/2" },
    { "Telem Ratio",  "Std" },
    { "Max Power",    "25" },
    { "Dynamic",      "Off" },
    { "Fan Thresh",   "250" },
    { "Model Match",  "Off" },
};

static const ProfileStep k_field_fast[] = {
    { "Packet Rate",  "333Hz Full" },
    { "Switch Mode",  "16ch Rate/2" },
    { "Telem Ratio",  "Std" },
    { "Max Power",    "250" },
    { "Dynamic",      "Dyn" },
    { "Fan Thresh",   "250" },
};

static const ProfileStep k_field_slow[] = {
    { "Packet Rate",  "100Hz Full" },
    { "Switch Mode",  "16ch Rate/2" },
    { "Telem Ratio",  "Std" },
    { "Max Power",    "250" },
    { "Dynamic",      "Dyn" },
    { "Fan Thresh",   "250" },
};

static const ProfileStep k_high_power[] = {
    { "Max Power",    "1000" },
    { "Dynamic",      "Dyn" },
    { "Fan Thresh",   "250" },
};

static void profile_advance(void);

static void profile_try_step(void)
{
    while (s.step_index < s.step_count) {
        const ProfileStep *st = &s.steps[s.step_index];
        const ElrsParam *p = elrs_service_find(st->param);
        if (!p) {
            msg(ELRS_MSG_WARNING, "Skipping %s: not offered by module.",
                st->param);
            s.step_index++;
            continue;
        }
        int idx = elrs_service_find_option(p, st->option);
        if (idx < 0) {
            msg(ELRS_MSG_WARNING, "Skipping %s: option '%s' not offered.",
                p->name, st->option);
            s.step_index++;
            continue;
        }
        if ((int32_t)idx == p->value) {
            s.step_index++;
            continue; // already set
        }
        ElrsWriteSafety safety = elrs_service_classify(p, idx);
        if (safety.needs_disarm && is_armed()) {
            msg(ELRS_MSG_ERROR,
                "Blocked: cannot change RF settings while armed.");
            s.profile_active = false;
            return;
        }
        char opt[48];
        elrs_param_option(p, idx, opt, sizeof(opt));
        msg(ELRS_MSG_INFO, "Setting %s to %s...", p->name, opt);
        s.pending_write_id = p->id;
        s.step_in_flight = true;
        s.step_deadline_ms = s.now_ms + 3000;
        elrs_client_write_value(p->id, idx);
        return;
    }
    // done
    s.profile_active = false;
    msg(ELRS_MSG_SUCCESS, "Robot ELRS defaults applied.");
}

static void profile_advance(void)
{
    s.step_in_flight = false;
    s.step_index++;
    profile_try_step();
}

bool elrs_service_apply_profile(ElrsProfile profile)
{
    if (!elrs_client_params_ready()) {
        msg(ELRS_MSG_ERROR, "Settings not loaded yet.");
        return false;
    }
    if (is_armed()) {
        msg(ELRS_MSG_ERROR,
            "Blocked: cannot change RF settings while armed.");
        return false;
    }
    if (s.profile_active) return false;

    const ProfileStep *steps;
    int n;
    switch (profile) {
    case ELRS_PROFILE_BENCH:
        steps = k_bench; n = (int)(sizeof(k_bench) / sizeof(k_bench[0]));
        msg(ELRS_MSG_INFO, "Applying Bench profile...");
        break;
    case ELRS_PROFILE_FIELD:
        if (s.baud > 400000) {
            steps = k_field_fast;
            n = (int)(sizeof(k_field_fast) / sizeof(k_field_fast[0]));
        } else {
            steps = k_field_slow;
            n = (int)(sizeof(k_field_slow) / sizeof(k_field_slow[0]));
        }
        msg(ELRS_MSG_INFO, "Applying Field profile...");
        break;
    case ELRS_PROFILE_HIGH_POWER:
        steps = k_high_power;
        n = (int)(sizeof(k_high_power) / sizeof(k_high_power[0]));
        msg(ELRS_MSG_WARNING, "Applying High-Power profile (1000mW)...");
        break;
    default:
        return false;
    }

    if (n > PROFILE_MAX_STEPS) n = PROFILE_MAX_STEPS;
    memcpy(s.steps, steps, (size_t)n * sizeof(ProfileStep));
    s.step_count = n;
    s.step_index = 0;
    s.profile_active = true;
    s.step_in_flight = false;
    profile_try_step();
    return true;
}

bool elrs_service_profile_active(void) { return s.profile_active; }

void elrs_service_on_client_event(ElrsClientEvent ev, uint8_t arg)
{
    // feedback for individual writes
    const ElrsParam *p = elrs_client_param(arg);
    if (ev == ELRS_EV_WRITE_VERIFIED && p) {
        msg(ELRS_MSG_SUCCESS, "%s updated and verified.", p->name);
    } else if (ev == ELRS_EV_WRITE_FAILED && p) {
        msg(ELRS_MSG_ERROR,
            "Write failed: module did not accept %s.", p->name);
    }

    // profile sequencing
    if (s.profile_active && s.step_in_flight && arg == s.pending_write_id) {
        if (ev == ELRS_EV_WRITE_VERIFIED) {
            profile_advance();
        } else if (ev == ELRS_EV_WRITE_FAILED) {
            msg(ELRS_MSG_ERROR, "Profile aborted.");
            s.profile_active = false;
            s.step_in_flight = false;
        }
    }
}

//=============================================================================
// Init / poll
//=============================================================================
void elrs_service_init(void)
{
    memset(&s, 0, sizeof(s));
    s.baud = 420000;
}

void elrs_service_set_msg_cb(ElrsMsgFn cb, void *user)
{
    s.msg_cb = cb;
    s.msg_user = user;
}

void elrs_service_set_armed_cb(ElrsArmedFn cb, void *user)
{
    s.armed_cb = cb;
    s.armed_user = user;
}

void elrs_service_set_baud(uint32_t baud) { s.baud = baud; }

void elrs_service_poll(uint32_t now_ms)
{
    s.now_ms = now_ms;
    if (s.profile_active && s.step_in_flight &&
        now_ms >= s.step_deadline_ms) {
        msg(ELRS_MSG_ERROR, "Profile step timed out; aborted.");
        s.profile_active = false;
        s.step_in_flight = false;
    }
}
