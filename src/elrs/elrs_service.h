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
// Safety classification for a parameter (derived from its NAME)
//=============================================================================
typedef struct {
    bool needs_disarm;      // blocked while armed
    bool needs_confirm;     // show confirmation dialog before writing
    char warning[96];       // non-empty => show warning text with confirm
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

/** Check whether the selected TX module is the expected ES24TX Pro. Emits a
 *  warning message if it is not (options are still used as discovered). */
void elrs_service_check_module(void);

/** Apply a one-button profile (sequential verified writes with feedback). */
bool elrs_service_apply_profile(ElrsProfile profile);
bool elrs_service_profile_active(void);

/** Hook: must be called from the client event callback so the service can
 *  advance profile steps on write results. */
void elrs_service_on_client_event(ElrsClientEvent ev, uint8_t arg);

#ifdef __cplusplus
}
#endif

#endif // ELRS_SERVICE_H
