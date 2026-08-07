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
    char param[24];
    char option[24];
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

    ElrsTxHardwareProfile tx_profile;
    bool        tx_profile_valid;
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
        size_t want_len = strlen(want);
        if (strncmp(norm, want, want_len) == 0) {
            if (isdigit((unsigned char)want[0]) &&
                isdigit((unsigned char)norm[want_len])) {
                continue;
            }
            return i;
        }
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

static bool norm_contains(const char *text, const char *needle)
{
    char a[256], b[64];
    elrs_normalize_name(text ? text : "", a, sizeof(a));
    elrs_normalize_name(needle ? needle : "", b, sizeof(b));
    return strstr(a, b) != NULL;
}

static bool param_name_contains(const char *needle)
{
    uint8_t count = elrs_client_param_count();
    for (uint8_t id = 1; id <= count; id++) {
        const ElrsParam *p = elrs_client_param(id);
        if (p && norm_contains(p->name, needle)) return true;
    }
    return false;
}

static bool any_info_contains(const char *needle)
{
    const ElrsDeviceInfo *dev =
        elrs_client_device_by_addr(CRSF_ADDR_TX_MODULE);
    if (dev && norm_contains(dev->name, needle)) return true;

    uint8_t count = elrs_client_param_count();
    for (uint8_t id = 1; id <= count; id++) {
        const ElrsParam *p = elrs_client_param(id);
        if (!p) continue;
        if (norm_contains(p->name, needle)) return true;
        if ((p->type == CRSF_PT_INFO || p->type == CRSF_PT_STRING) &&
            norm_contains(p->str_value, needle)) return true;
    }
    return false;
}

uint16_t elrs_service_discovered_max_power_mw(void)
{
    const ElrsParam *p = elrs_service_find("Max Power");
    if (!p || p->type != CRSF_PT_TEXT_SELECTION) return 0;
    uint16_t max_mw = 0;
    int count = elrs_param_option_count(p);
    for (int i = 0; i < count; i++) {
        int mw = option_number(p, i);
        if (mw > (int)max_mw) max_mw = (uint16_t)mw;
    }
    return max_mw;
}

bool elrs_service_tx_feature_discovered(const char *feature_name)
{
    if (!feature_name) return false;
    if (norm_contains(feature_name, "fan")) {
        return elrs_service_find("Fan Thresh") != NULL ||
               elrs_service_find("Fan Threshold") != NULL ||
               param_name_contains("fan");
    }
    if (norm_contains(feature_name, "backpack")) {
        return param_name_contains("backpack") ||
               param_name_contains("vrx") ||
               param_name_contains("wifi");
    }
    if (norm_contains(feature_name, "oled")) {
        return param_name_contains("oled") ||
               param_name_contains("screen");
    }
    if (norm_contains(feature_name, "5d") || norm_contains(feature_name, "button")) {
        return param_name_contains("5d") || param_name_contains("joystick") ||
               param_name_contains("button");
    }
    if (norm_contains(feature_name, "rgb")) {
        return param_name_contains("rgb") || param_name_contains("led");
    }
    if (norm_contains(feature_name, "dcdc")) {
        return param_name_contains("dcdc");
    }
    return param_name_contains(feature_name);
}

static void copy_text(char *dst, size_t len, const char *src)
{
    if (!dst || len == 0) return;
    strncpy(dst, src ? src : "", len - 1);
    dst[len - 1] = '\0';
}

static void set_expected_powers(ElrsTxHardwareProfile *p,
                                const uint16_t *values, uint8_t count)
{
    if (count > 8) count = 8;
    memset(p->expectedPowerMw, 0, sizeof(p->expectedPowerMw));
    for (uint8_t i = 0; i < count; i++) p->expectedPowerMw[i] = values[i];
    p->expectedPowerCount = count;
}

static void fill_profile(ElrsTxHardwareProfile *p, ElrsTxModuleFamily family)
{
    memset(p, 0, sizeof(*p));
    p->family = family;
    switch (family) {
    case ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO: {
        static const uint16_t pow[] = { 10, 25, 50, 100, 250, 500, 1000 };
        copy_text(p->displayName, sizeof(p->displayName),
                  "Happymodel ES24TX Pro");
        copy_text(p->expectedConfiguratorCategory,
                  sizeof(p->expectedConfiguratorCategory),
                  "Happymodel 2.4 GHz");
        copy_text(p->expectedConfiguratorTarget,
                  sizeof(p->expectedConfiguratorTarget),
                  "HappyModel ES24 Pro 2.4GHz TX");
        copy_text(p->expectedFirmwareTarget,
                  sizeof(p->expectedFirmwareTarget),
                  "HappyModel_ES24TX_Pro_Series_2400_TX");
        set_expected_powers(p, pow, (uint8_t)(sizeof(pow) / sizeof(pow[0])));
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        p->maxExpectedPowerMw = 1000;
        p->expectedMinInputVoltage = 5;
        p->expectedMaxInputVoltage = 10;
        p->hasBackpackExpected = true;
        p->hasFanExpected = true;
        p->hasRgbExpected = true;
        p->hasDcdcExpected = true;
        break;
    }
    case ELRS_TX_FAMILY_HAPPYMODEL_ES24_NON_PRO:
        copy_text(p->displayName, sizeof(p->displayName),
                  "Happymodel ES24");
        copy_text(p->expectedConfiguratorCategory,
                  sizeof(p->expectedConfiguratorCategory),
                  "Happymodel 2.4 GHz");
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        p->expectedMinInputVoltage = 5;
        p->expectedMaxInputVoltage = 10;
        break;
    case ELRS_TX_FAMILY_BETAFPV_MICRO_1W: {
        static const uint16_t pow[] = { 25, 50, 250, 500, 1000 };
        copy_text(p->displayName, sizeof(p->displayName),
                  "BETAFPV Micro 1W");
        copy_text(p->expectedConfiguratorCategory,
                  sizeof(p->expectedConfiguratorCategory),
                  "BETAFPV 2.4 GHz");
        copy_text(p->expectedConfiguratorTarget,
                  sizeof(p->expectedConfiguratorTarget),
                  "BETAFPV 2.4GHz 1W Micro TX");
        copy_text(p->expectedFirmwareTarget,
                  sizeof(p->expectedFirmwareTarget),
                  "BETAFPV_2400_TX_MICRO_1000mW");
        set_expected_powers(p, pow, (uint8_t)(sizeof(pow) / sizeof(pow[0])));
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        p->maxExpectedPowerMw = 1000;
        p->expectedMinInputVoltage = 5;
        p->expectedMaxInputVoltage = 12;
        p->hasBackpackExpected = true;
        p->hasFanExpected = true;
        p->hasRgbExpected = true;
        p->hasOledExpected = true;
        p->hasFiveDButtonExpected = true;
        break;
    }
    case ELRS_TX_FAMILY_BETAFPV_MICRO_500MW: {
        static const uint16_t pow[] = { 10, 25, 50, 100, 250, 500 };
        copy_text(p->displayName, sizeof(p->displayName),
                  "BETAFPV Micro 500mW");
        copy_text(p->expectedConfiguratorCategory,
                  sizeof(p->expectedConfiguratorCategory),
                  "BETAFPV 2.4 GHz");
        copy_text(p->expectedConfiguratorTarget,
                  sizeof(p->expectedConfiguratorTarget),
                  "BETAFPV 2.4GHz Micro TX");
        set_expected_powers(p, pow, (uint8_t)(sizeof(pow) / sizeof(pow[0])));
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        p->maxExpectedPowerMw = 500;
        p->expectedMinInputVoltage = 5;
        p->expectedMaxInputVoltage = 12;
        p->hasOledExpected = true;
        p->hasFiveDButtonExpected = true;
        break;
    }
    case ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN:
        copy_text(p->displayName, sizeof(p->displayName),
                  "BETAFPV Micro, unknown power variant");
        copy_text(p->expectedConfiguratorCategory,
                  sizeof(p->expectedConfiguratorCategory),
                  "BETAFPV 2.4 GHz");
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        p->expectedMinInputVoltage = 5;
        p->expectedMaxInputVoltage = 12;
        p->hasOledExpected = true;
        p->hasFiveDButtonExpected = true;
        break;
    case ELRS_TX_FAMILY_OTHER_ELRS_TX:
        copy_text(p->displayName, sizeof(p->displayName), "Other ELRS TX");
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        break;
    case ELRS_TX_FAMILY_UNKNOWN:
    default:
        copy_text(p->displayName, sizeof(p->displayName), "Unknown");
        p->safeBenchPowerMw = 25;
        p->safeFieldPowerMw = 250;
        break;
    }
}

static ElrsTxModuleFamily classify_tx_family(void)
{
    const ElrsDeviceInfo *dev =
        elrs_client_device_by_addr(CRSF_ADDR_TX_MODULE);
    uint16_t max_power = elrs_service_discovered_max_power_mw();

    bool beta = any_info_contains("BETAFPV");
    bool beta_micro = beta && (any_info_contains("Micro") ||
                              any_info_contains("TX_MICRO"));
    bool happy = any_info_contains("HappyModel") ||
                 any_info_contains("Happymodel") ||
                 any_info_contains("ES24");
    bool has_oled_5d = elrs_service_tx_feature_discovered("oled") ||
                       elrs_service_tx_feature_discovered("5d") ||
                       elrs_service_tx_feature_discovered("joystick");
    bool has_fan = elrs_service_tx_feature_discovered("fan");
    bool has_backpack = elrs_service_tx_feature_discovered("backpack");

    if (beta_micro) {
        if (any_info_contains("1W") || any_info_contains("1000mW") ||
            any_info_contains("1000mw") ||
            any_info_contains("BETAFPV_2400_TX_MICRO_1000") ||
            max_power >= 1000) {
            return ELRS_TX_FAMILY_BETAFPV_MICRO_1W;
        }
        if (any_info_contains("500mW") || any_info_contains("500mw") ||
            (max_power >= 500 && max_power < 1000 && has_oled_5d)) {
            return ELRS_TX_FAMILY_BETAFPV_MICRO_500MW;
        }
        return ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN;
    }

    if (happy) {
        if (any_info_contains("ES24TX Pro") || any_info_contains("ES24 Pro") ||
            any_info_contains("HappyModel_ES24TX_Pro_Series_2400_TX") ||
            (max_power >= 1000 && has_fan && has_backpack && !has_oled_5d)) {
            return ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO;
        }
        return max_power >= 1000 ? ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO
                                 : ELRS_TX_FAMILY_HAPPYMODEL_ES24_NON_PRO;
    }

    if (dev && dev->present && dev->is_elrs && dev->address == CRSF_ADDR_TX_MODULE) {
        return ELRS_TX_FAMILY_OTHER_ELRS_TX;
    }
    return ELRS_TX_FAMILY_UNKNOWN;
}

const ElrsTxHardwareProfile *elrs_service_tx_profile(void)
{
    if (!s.tx_profile_valid) {
        fill_profile(&s.tx_profile, classify_tx_family());
        s.tx_profile_valid = true;
    }
    return &s.tx_profile;
}

ElrsTxModuleFamily elrs_service_tx_family(void)
{
    return elrs_service_tx_profile()->family;
}

const char *elrs_service_family_display_name(ElrsTxModuleFamily family)
{
    switch (family) {
    case ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO:
        return "Happymodel ES24TX Pro";
    case ELRS_TX_FAMILY_HAPPYMODEL_ES24_NON_PRO:
        return "Happymodel ES24";
    case ELRS_TX_FAMILY_BETAFPV_MICRO_1W:
        return "BETAFPV Micro 1W";
    case ELRS_TX_FAMILY_BETAFPV_MICRO_500MW:
        return "BETAFPV Micro 500mW";
    case ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN:
        return "BETAFPV Micro, unknown power variant";
    case ELRS_TX_FAMILY_OTHER_ELRS_TX:
        return "Other ELRS TX";
    case ELRS_TX_FAMILY_UNKNOWN:
    default:
        return "Unknown";
    }
}

bool isPowerValueExpectedForFamily(ElrsTxModuleFamily family, uint16_t mw)
{
    ElrsTxHardwareProfile p;
    fill_profile(&p, family);
    for (uint8_t i = 0; i < p.expectedPowerCount; i++) {
        if (p.expectedPowerMw[i] == mw) return true;
    }
    return family == ELRS_TX_FAMILY_UNKNOWN ||
           family == ELRS_TX_FAMILY_OTHER_ELRS_TX ||
           family == ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN;
}

bool isHighPowerForFamily(ElrsTxModuleFamily family, uint16_t mw)
{
    (void)family;
    return mw >= 500;
}

bool isDangerousUnsupportedPower(ElrsTxModuleFamily family, uint16_t mw)
{
    if (mw == 5 || mw == 2000) return true;
    if (family == ELRS_TX_FAMILY_BETAFPV_MICRO_500MW && mw > 500) return true;
    if (family == ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO && mw > 1000) return true;
    if (family == ELRS_TX_FAMILY_BETAFPV_MICRO_1W && mw > 1000) return true;
    return false;
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
        bool fast = (hz >= 500) || (hz >= 333 && full) ||
                    strstr(opt, "F500") || strstr(opt, "F1000") ||
                    strstr(opt, "K1000");
        if (fast && s.baud > 0 && s.baud <= 400000) {
            snprintf(r.warning, sizeof(r.warning),
                     "Selected packet rate may need CRSF baud >400K.");
        } else if (fast &&
               (elrs_service_tx_family() == ELRS_TX_FAMILY_BETAFPV_MICRO_1W ||
                elrs_service_tx_family() == ELRS_TX_FAMILY_BETAFPV_MICRO_500MW ||
                elrs_service_tx_family() == ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN) &&
               any_info_contains("V3.3.0")) {
            snprintf(r.warning, sizeof(r.warning),
                     "BETAFPV V3.x may require 921K baud for Lua/config reliability.");
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
        snprintf(r.warning, sizeof(r.warning),
                 "Power off receiver before changing Switch Mode.");
    } else if (name_is(p, "Max Power")) {
        int mw = option_number(p, opt_index);
        ElrsTxModuleFamily family = elrs_service_tx_family();
        if (mw >= 1000) {
            r.needs_disarm = true;
            r.needs_confirm = true;
            if (family == ELRS_TX_FAMILY_BETAFPV_MICRO_1W) {
                snprintf(r.warning, sizeof(r.warning),
                         "1W mode. Antenna, cooling, and legal compliance required. BETAFPV 1W mode requires antenna and cooling.");
            } else {
                snprintf(r.warning, sizeof(r.warning),
                         "1W mode. Antenna, cooling, and legal compliance required.");
            }
        } else if (mw >= 500) {
            r.needs_disarm = true;
            r.needs_confirm = true;
            if (family == ELRS_TX_FAMILY_BETAFPV_MICRO_500MW) {
                snprintf(r.warning, sizeof(r.warning),
                         "500mW mode requires antenna and airflow.");
            } else {
                snprintf(r.warning, sizeof(r.warning),
                         "High TX power. Ensure antenna attached and airflow available.");
            }
        } else if (mw >= 250) {
            snprintf(r.warning, sizeof(r.warning),
                     "Medium/high TX power. Check antenna.");
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

    s.tx_profile_valid = false;
    const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
    msg(ELRS_MSG_INFO, "ELRS TX found: %s", dev->name);
    msg(ELRS_MSG_INFO, "Detected: %s",
        elrs_service_family_display_name(profile->family));

    switch (profile->family) {
    case ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO:
        msg(ELRS_MSG_WARNING, "Install antenna before RF output. Use 5V-10V supply range.");
        if (!elrs_service_tx_feature_discovered("fan")) {
            msg(ELRS_MSG_WARNING, "Fan Threshold not exposed by this firmware.");
        }
        break;
    case ELRS_TX_FAMILY_BETAFPV_MICRO_1W:
        msg(ELRS_MSG_WARNING, "Install antenna before powering the module; PA damage can occur without antenna.");
        msg(ELRS_MSG_WARNING, "Do not power BETAFPV Micro TX from 3S or higher through XT30.");
        msg(ELRS_MSG_INFO, "BETAFPV local OLED menu may also change ELRS settings.");
        msg(ELRS_MSG_INFO, "If settings disagree, refresh parameters from TX module.");
        if (!elrs_service_tx_feature_discovered("fan")) {
            msg(ELRS_MSG_WARNING, "Fan Threshold not exposed; verify cooling manually.");
        }
        break;
    case ELRS_TX_FAMILY_BETAFPV_MICRO_500MW:
        msg(ELRS_MSG_WARNING, "Install antenna before powering the module; PA damage can occur without antenna.");
        msg(ELRS_MSG_WARNING, "Do not power BETAFPV Micro TX from 3S or higher through XT30.");
        msg(ELRS_MSG_INFO, "BETAFPV local OLED menu may also change ELRS settings.");
        msg(ELRS_MSG_INFO, "If settings disagree, refresh parameters from TX module.");
        break;
    case ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN:
        msg(ELRS_MSG_WARNING, "Warning: module family uncertain; using discovered options");
        msg(ELRS_MSG_WARNING, "Warning: selected defaults may not match detected hardware");
        msg(ELRS_MSG_WARNING, "Do not power BETAFPV Micro TX from 3S or higher through XT30.");
        break;
    case ELRS_TX_FAMILY_HAPPYMODEL_ES24_NON_PRO:
    case ELRS_TX_FAMILY_OTHER_ELRS_TX:
    case ELRS_TX_FAMILY_UNKNOWN:
    default:
        msg(ELRS_MSG_WARNING, "Warning: module family uncertain; using discovered options");
        break;
    }
}

//=============================================================================
// Profiles
//=============================================================================
static bool option_available(const char *param_name, const char *option)
{
    const ElrsParam *p = elrs_service_find(param_name);
    return p && elrs_service_find_option(p, option) >= 0;
}

static bool add_step(ProfileStep *steps, int *n,
                     const char *param, const char *option)
{
    if (*n >= PROFILE_MAX_STEPS || !option_available(param, option)) return false;
    copy_text(steps[*n].param, sizeof(steps[*n].param), param);
    copy_text(steps[*n].option, sizeof(steps[*n].option), option);
    (*n)++;
    return true;
}

static bool add_first_available(ProfileStep *steps, int *n,
                                const char *param,
                                const char *const *options, int count)
{
    for (int i = 0; i < count; i++) {
        if (add_step(steps, n, param, options[i])) return true;
    }
    return false;
}

// Switch Mode options that carry all 16 channels at full resolution, most
// preferred first. The custom controller packs switches/buttons/nav into
// CH9-11, so an 8ch mode (which only transmits CH1-8) silently drops them.
// "16ch Rate/2 Full Res" is preferred over plain "16ch Rate/2" because some
// module families expose both and the plain variant reduces aux resolution.
static const char *const k_switch_mode_pref[] = {
    "16ch Rate/2 Full Res",
    "16ch Rate/2",
};
#define K_SWITCH_MODE_PREF_COUNT \
    ((int)(sizeof(k_switch_mode_pref) / sizeof(k_switch_mode_pref[0])))

// True if a Switch Mode option text is a 16-channel mode (normalized "16ch..").
static bool switch_mode_is_16ch(const char *opt)
{
    char norm[48];
    elrs_normalize_name(opt, norm, sizeof(norm));
    return strncmp(norm, "16ch", 4) == 0;
}

// Queue the best available 16ch full-res Switch Mode.
static bool add_switch_mode_16ch(ProfileStep *steps, int *n)
{
    return add_first_available(steps, n, "Switch Mode", k_switch_mode_pref,
                               K_SWITCH_MODE_PREF_COUNT);
}

static void mw_to_option(uint16_t mw, char *buf, size_t len)
{
    snprintf(buf, len, "%u", (unsigned)mw);
}

static bool add_lowest_power(ProfileStep *steps, int *n)
{
    const ElrsParam *p = elrs_service_find("Max Power");
    if (!p) return false;
    uint16_t best = 0;
    int count = elrs_param_option_count(p);
    for (int i = 0; i < count; i++) {
        int mw = option_number(p, i);
        if (mw <= 0) continue;
        if (best == 0 || mw < (int)best) best = (uint16_t)mw;
    }
    if (best == 0) return false;
    char opt[12];
    mw_to_option(best, opt, sizeof(opt));
    return add_step(steps, n, "Max Power", opt);
}

static bool build_profile_steps(ElrsProfile profile, ProfileStep *steps, int *n)
{
    *n = 0;
    ElrsTxModuleFamily family = elrs_service_tx_family();
    static const char *const telem[] = { "Std", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2" };
    static const char *const bench_es24[] = { "10", "25" };
    static const char *const bench_beta1w[] = { "25", "10" };
    static const char *const bench_beta500[] = { "25", "10" };
    static const char *const field_power[] = { "250", "100", "500" };

    if (profile == ELRS_PROFILE_HIGH_POWER) {
        if (!option_available("Max Power", "1000")) {
            msg(ELRS_MSG_ERROR, "High-power profile requires discovered 1000mW support.");
            return false;
        }
        add_step(steps, n, "Packet Rate", option_available("Packet Rate", "333Hz Full") ? "333Hz Full" : "100Hz Full");
        add_switch_mode_16ch(steps, n);
        add_first_available(steps, n, "Telem Ratio", telem, (int)(sizeof(telem) / sizeof(telem[0])));
        add_step(steps, n, "Dynamic", "Dyn");
        add_step(steps, n, "Max Power", "1000");
        add_step(steps, n, "Fan Thresh", option_available("Fan Thresh", "250") ? "250" : "25");
        return *n > 0;
    }

    if (profile == ELRS_PROFILE_FIELD) {
        add_step(steps, n, "Packet Rate",
                 (s.baud > 400000 && option_available("Packet Rate", "333Hz Full")) ?
                 "333Hz Full" : "100Hz Full");
    } else {
        add_step(steps, n, "Packet Rate", "100Hz Full");
    }
    add_switch_mode_16ch(steps, n);
    add_first_available(steps, n, "Telem Ratio", telem, (int)(sizeof(telem) / sizeof(telem[0])));

    if (profile == ELRS_PROFILE_BENCH) {
        switch (family) {
        case ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO:
        case ELRS_TX_FAMILY_HAPPYMODEL_ES24_NON_PRO:
            add_first_available(steps, n, "Max Power", bench_es24,
                                (int)(sizeof(bench_es24) / sizeof(bench_es24[0])));
            break;
        case ELRS_TX_FAMILY_BETAFPV_MICRO_1W:
            add_first_available(steps, n, "Max Power", bench_beta1w,
                                (int)(sizeof(bench_beta1w) / sizeof(bench_beta1w[0])));
            break;
        case ELRS_TX_FAMILY_BETAFPV_MICRO_500MW:
        case ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN:
            add_first_available(steps, n, "Max Power", bench_beta500,
                                (int)(sizeof(bench_beta500) / sizeof(bench_beta500[0])));
            break;
        default:
            add_lowest_power(steps, n);
            break;
        }
        add_step(steps, n, "Dynamic", "Off");
        add_step(steps, n, "Fan Thresh", "250");
        add_step(steps, n, "Model Match", "Off");
    } else {
        add_first_available(steps, n, "Max Power", field_power,
                            (int)(sizeof(field_power) / sizeof(field_power[0])));
        add_step(steps, n, "Dynamic", "Dyn");
        add_step(steps, n, "Fan Thresh", "250");
    }
    return *n > 0;
}

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

    ProfileStep steps[PROFILE_MAX_STEPS];
    int n = 0;
    switch (profile) {
    case ELRS_PROFILE_BENCH:
        msg(ELRS_MSG_INFO, "Applying Bench profile...");
        break;
    case ELRS_PROFILE_FIELD:
        msg(ELRS_MSG_INFO, "Applying Field profile...");
        break;
    case ELRS_PROFILE_HIGH_POWER:
        msg(ELRS_MSG_WARNING,
            "Enable 1W mode? Antenna, cooling, and legal compliance required.");
        break;
    default:
        return false;
    }

    if (!build_profile_steps(profile, steps, &n)) return false;

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

void elrs_service_enforce_switch_mode(void)
{
    if (s.profile_active) return;              // profile will set it itself
    if (!elrs_client_params_ready()) return;

    const ElrsParam *p = elrs_service_find("Switch Mode");
    if (!p || p->type != CRSF_PT_TEXT_SELECTION) return;

    char cur[48] = "";
    if (p->value >= 0) elrs_param_option(p, p->value, cur, sizeof(cur));
    if (switch_mode_is_16ch(cur)) return;      // already a 16ch mode

    msg(ELRS_MSG_WARNING,
        "Switch Mode '%s' drops CH9-16: robot loses SW_B..H, buttons, and nav. "
        "A 16ch mode is required.", cur[0] ? cur : "?");

    if (is_armed()) {
        msg(ELRS_MSG_ERROR,
            "Disarm to let the controller switch to 16ch Full Res.");
        return;
    }

    for (int i = 0; i < K_SWITCH_MODE_PREF_COUNT; i++) {
        int idx = elrs_service_find_option(p, k_switch_mode_pref[i]);
        if (idx < 0) continue;
        char opt[48] = "";
        elrs_param_option(p, idx, opt, sizeof(opt));
        msg(ELRS_MSG_INFO,
            "Setting Switch Mode to %s (power-cycle the receiver to apply)...",
            opt);
        elrs_client_write_value(p->id, idx);
        return;
    }
    msg(ELRS_MSG_ERROR, "This module offers no 16ch switch mode.");
}

void elrs_service_on_client_event(ElrsClientEvent ev, uint8_t arg)
{
    if (ev == ELRS_EV_PARAMS_LOADED) {
        s.tx_profile_valid = false;
        elrs_service_enforce_switch_mode();
    }

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
