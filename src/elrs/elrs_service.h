/**
 * @file elrs_service.h
 * High-level ExpressLRS settings service.
 *
 * Sits on top of elrs_client and provides:
 *  - lookup of parameters by normalized NAME (never by hardcoded number)
 *  - safety rules (arming lockouts, high-power confirmation, baud warnings)
 *  - user feedback messages for the UI
 *  - one-button "Robot ELRS Defaults" profiles
 *
 * Pure C99, no heap.
 */
#ifndef ELRS_SERVICE_H
#define ELRS_SERVICE_H

#include "elrs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Messages / feedback
//=============================================================================
typedef enum {
    ELRS_MSG_INFO = 0,
    ELRS_MSG_SUCCESS,
    ELRS_MSG_WARNING,
    ELRS_MSG_ERROR,
} ElrsMsgLevel;

typedef void (*ElrsMsgFn)(ElrsMsgLevel level, const char *text, void *user);

/** Returns true when the robot is armed (RF changes must be blocked). */
typedef bool (*ElrsArmedFn)(void *user);

//=============================================================================
// TX module hardware profile
//=============================================================================
typedef enum {
    ELRS_TX_FAMILY_UNKNOWN = 0,
    ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO,
    ELRS_TX_FAMILY_HAPPYMODEL_ES24_NON_PRO,
    ELRS_TX_FAMILY_BETAFPV_MICRO_1W,
    ELRS_TX_FAMILY_BETAFPV_MICRO_500MW,
    ELRS_TX_FAMILY_BETAFPV_MICRO_UNKNOWN,
    ELRS_TX_FAMILY_OTHER_ELRS_TX,
} ElrsTxModuleFamily;

typedef struct {
    ElrsTxModuleFamily family;
    char displayName[48];
    char expectedConfiguratorCategory[32];
    char expectedConfiguratorTarget[48];
    char expectedFirmwareTarget[64];

    uint16_t expectedPowerMw[8];
    uint8_t expectedPowerCount;
    uint16_t safeBenchPowerMw;
    uint16_t safeFieldPowerMw;
    uint16_t maxExpectedPowerMw;

    uint8_t expectedMinInputVoltage;
    uint8_t expectedMaxInputVoltage;

    bool hasBackpackExpected;
    bool hasFanExpected;
    bool hasRgbExpected;
    bool hasOledExpected;
    bool hasFiveDButtonExpected;
    bool hasDcdcExpected;
} ElrsTxHardwareProfile;

//=============================================================================
// Safety classification for a parameter (derived from its NAME)
//=============================================================================
typedef struct {
    bool needs_disarm;      // blocked while armed
    bool needs_confirm;     // show confirmation dialog before writing
    char warning[160];      // non-empty => show warning text with confirm
} ElrsWriteSafety;

//=============================================================================
// Setup profiles
//=============================================================================
typedef enum {
    ELRS_PROFILE_BENCH = 0,   // 100Hz Full, low power, telem Std
    ELRS_PROFILE_FIELD,       // higher rate, mid power, dynamic
    ELRS_PROFILE_HIGH_POWER,  // field + 1000mW (explicit confirm)
} ElrsProfile;

//=============================================================================
// API
//=============================================================================

void elrs_service_init(void);
void elrs_service_set_msg_cb(ElrsMsgFn cb, void *user);
void elrs_service_set_armed_cb(ElrsArmedFn cb, void *user);

/** Set the handset CRSF baud rate (used for packet-rate warnings). */
void elrs_service_set_baud(uint32_t baud);

/** Drive queued profile steps. Call frequently alongside elrs_client_poll. */
void elrs_service_poll(uint32_t now_ms);

/** Normalize a name: lowercase, strip spaces/underscores/hyphens/slashes. */
void elrs_normalize_name(const char *in, char *out, size_t out_len);

/**
 * Find a loaded parameter by human name (case/spacing-insensitive,
 * common aliases such as "Telem Ratio" == "Telemetry Ratio" supported).
 * Returns NULL if not present in the discovered tree.
 */
const ElrsParam *elrs_service_find(const char *name);

/**
 * Find the option index of a TEXT_SELECTION whose text matches
 * `option_text` (normalized compare; also matches by leading number,
 * e.g. "500" matches "500 mW"). Returns -1 if the module doesn't offer it.
 */
int elrs_service_find_option(const ElrsParam *p, const char *option_text);

/** Safety classification for writing option `opt_index` to `p`. */
ElrsWriteSafety elrs_service_classify(const ElrsParam *p, int opt_index);

/** Current armed state (via the registered callback; false if none). */
bool elrs_service_is_armed(void);

/**
 * High-level named write: finds the parameter and option, applies safety
 * checks (armed lockout), emits feedback messages and performs a verified
 * write. Returns false if rejected immediately.
 * Note: confirmation dialogs are the UI's job — this call assumes the user
 * already confirmed if the classification requires it.
 */
bool elrs_service_set(const char *param_name, const char *option_text);

/** Classify the selected TX module from device info and discovered params. */
const ElrsTxHardwareProfile *elrs_service_tx_profile(void);
ElrsTxModuleFamily elrs_service_tx_family(void);
const char *elrs_service_family_display_name(ElrsTxModuleFamily family);
uint16_t elrs_service_discovered_max_power_mw(void);
bool elrs_service_tx_feature_discovered(const char *feature_name);
bool isPowerValueExpectedForFamily(ElrsTxModuleFamily family, uint16_t mw);
bool isHighPowerForFamily(ElrsTxModuleFamily family, uint16_t mw);
bool isDangerousUnsupportedPower(ElrsTxModuleFamily family, uint16_t mw);

/** Check/classify the selected TX module. Emits warnings as needed. */
void elrs_service_check_module(void);

/** Apply a one-button profile (sequential verified writes with feedback). */
bool elrs_service_apply_profile(ElrsProfile profile);
bool elrs_service_profile_active(void);

/**
 * Ensure the TX module is in a 16-channel switch mode. The custom controller
 * packs switches/buttons/nav into CH9-11, which an "8ch" mode never transmits.
 * Warns if the current mode is not 16ch and, when disarmed and no profile is
 * running, writes the best available 16ch Full Res mode. Safe to call whenever
 * parameters (re)load. */
void elrs_service_enforce_switch_mode(void);

/** Hook: must be called from the client event callback so the service can
 *  advance profile steps on write results. */
void elrs_service_on_client_event(ElrsClientEvent ev, uint8_t arg);

#ifdef __cplusplus
}
#endif

#endif // ELRS_SERVICE_H
