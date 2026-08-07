/**
 * @file elrs_client.h
 * Dynamic CRSF/ExpressLRS parameter client.
 *
 * Discovers devices (ping -> device info), reads the full parameter list
 * with chunk reassembly, writes values with read-back verification and runs
 * command parameters through the full status state machine.
 *
 * NOTHING is hardcoded: parameter numbers, option lists and limits are all
 * discovered from the connected module at runtime.
 *
 * Pure C99, fixed buffers, no heap. Transport-agnostic: the host provides a
 * send callback and feeds received frames into elrs_client_on_frame().
 * All operations are asynchronous and driven from elrs_client_poll().
 */
#ifndef ELRS_CLIENT_H
#define ELRS_CLIENT_H

#include "crsf_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Limits (fixed buffers, no heap)
//=============================================================================
#define ELRS_MAX_DEVICES     4
#define ELRS_MAX_PARAMS      48
#define ELRS_NAME_LEN        28
#define ELRS_OPTS_LEN        176   // semicolon-separated option list
#define ELRS_UNIT_LEN        8
#define ELRS_STR_LEN         48
#define ELRS_CHUNK_BUF_LEN   512

//=============================================================================
// Types
//=============================================================================

typedef struct {
    uint8_t  address;       // CRSF address (0xEE TX, 0xEC RX, ...)
    bool     present;
    bool     is_elrs;       // serial == 'ELRS'
    char     name[32];
    uint8_t  param_count;   // number of parameters reported (excl. param 0)
} ElrsDeviceInfo;

typedef struct {
    bool     valid;         // fully loaded
    uint8_t  id;            // parameter number (1-based, discovered)
    uint8_t  parent;        // parent folder id, 0 = root
    uint8_t  type;          // CrsfParamType
    bool     hidden;
    char     name[ELRS_NAME_LEN];

    // TEXT_SELECTION / integer types
    char     options[ELRS_OPTS_LEN];  // "opt1;opt2;..." (TEXT_SELECTION)
    int32_t  value;
    int32_t  min;
    int32_t  max;
    char     unit[ELRS_UNIT_LEN];

    // FLOAT extras
    uint8_t  precision;
    int32_t  step;

    // STRING / INFO
    char     str_value[ELRS_STR_LEN];

    // COMMAND
    uint8_t  cmd_status;    // CrsfCommandStep
    uint8_t  cmd_timeout;   // in 10 ms units
    char     cmd_info[ELRS_STR_LEN];
} ElrsParam;

typedef enum {
    ELRS_EV_DEVICE_FOUND = 0,   // arg = device index
    ELRS_EV_PARAMS_LOADED,      // all parameters of selected device loaded
    ELRS_EV_PARAM_UPDATED,      // arg = param id (single refresh finished)
    ELRS_EV_LOAD_PROGRESS,      // arg = params loaded so far
    ELRS_EV_WRITE_VERIFIED,     // arg = param id
    ELRS_EV_WRITE_FAILED,       // arg = param id
    ELRS_EV_CMD_STATUS,         // arg = param id, command status changed
    ELRS_EV_TIMEOUT,            // device not responding
} ElrsClientEvent;

/** Transport send: emit an extended frame. payload starts at [dest, orig]. */
typedef void (*ElrsSendFn)(uint8_t frame_type, const uint8_t *payload,
                           uint8_t len, void *user);

typedef void (*ElrsEventFn)(ElrsClientEvent ev, uint8_t arg, void *user);

//=============================================================================
// API
//=============================================================================

void elrs_client_init(ElrsSendFn send_fn, void *send_user);
void elrs_client_set_event_cb(ElrsEventFn cb, void *user);

/** Broadcast a device ping. Discovered devices arrive via DEVICE_FOUND. */
void elrs_client_ping(void);

/** Select a device and (re)load its full parameter list. */
void elrs_client_select_device(uint8_t address);
uint8_t elrs_client_selected_device(void);

/** Re-read every parameter of the selected device. */
void elrs_client_reload_params(void);

/** Re-read a single parameter. */
void elrs_client_refresh_param(uint8_t param_id);

/** Write a value (TEXT_SELECTION index or integer types). Verified by
 *  read-back; result arrives as WRITE_VERIFIED / WRITE_FAILED. */
bool elrs_client_write_value(uint8_t param_id, int32_t value);

/** Command parameter control. */
bool elrs_client_command_start(uint8_t param_id);
bool elrs_client_command_confirm(uint8_t param_id);
bool elrs_client_command_cancel(uint8_t param_id);

/** Feed a received extended frame (payload starts at [dest, orig]). */
void elrs_client_on_frame(uint8_t frame_type, const uint8_t *payload,
                          uint8_t len);

/** Drive timeouts / retries / queued reads. Call frequently (~10 ms). */
void elrs_client_poll(uint32_t now_ms);

//=============================================================================
// Accessors
//=============================================================================

int  elrs_client_device_count(void);
const ElrsDeviceInfo *elrs_client_device(int index);
const ElrsDeviceInfo *elrs_client_device_by_addr(uint8_t address);

/** Param by discovered id (1..param_count). NULL if not loaded. */
const ElrsParam *elrs_client_param(uint8_t param_id);
uint8_t elrs_client_param_count(void);
bool elrs_client_params_ready(void);
bool elrs_client_busy(void);

/** Get option string `index` from a TEXT_SELECTION param into buf.
 *  Returns false if out of range. */
bool elrs_param_option(const ElrsParam *p, int index, char *buf, size_t buflen);
int  elrs_param_option_count(const ElrsParam *p);

#ifdef __cplusplus
}
#endif

#endif // ELRS_CLIENT_H
