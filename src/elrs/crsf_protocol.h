/**
 * @file crsf_protocol.h
 * CRSF protocol constants and framing helpers for the ELRS configuration
 * client. Pure C99, no platform dependencies — compiles on firmware,
 * simulator and host tests.
 */
#ifndef CRSF_PROTOCOL_H
#define CRSF_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Addresses
//=============================================================================
#define CRSF_ADDR_BROADCAST      0x00
#define CRSF_ADDR_HANDSET        0xEA  // radio handset (us)
#define CRSF_ADDR_RECEIVER       0xEC  // ELRS receiver
#define CRSF_ADDR_TX_MODULE      0xEE  // ELRS TX module
#define CRSF_ADDR_ELRS_LUA       0xEF  // lua/handset origin used by ELRS

// AlfredoCRSF's crsf_protocol.h also defines this (as 0XC8)
#ifndef CRSF_SYNC_BYTE
#define CRSF_SYNC_BYTE           0xC8
#endif

//=============================================================================
// Frame types (extended header frames used by the config protocol)
//=============================================================================
#define CRSF_FT_DEVICE_PING          0x28
#define CRSF_FT_DEVICE_INFO          0x29
#define CRSF_FT_PARAM_SETTINGS_ENTRY 0x2B
#define CRSF_FT_PARAM_READ           0x2C
#define CRSF_FT_PARAM_WRITE          0x2D
#define CRSF_FT_ELRS_STATUS          0x2E
#define CRSF_FT_COMMAND              0x32

//=============================================================================
// Parameter (field) types
//=============================================================================
typedef enum {
    CRSF_PT_UINT8          = 0x00,
    CRSF_PT_INT8           = 0x01,
    CRSF_PT_UINT16         = 0x02,
    CRSF_PT_INT16          = 0x03,
    CRSF_PT_FLOAT          = 0x08,
    CRSF_PT_TEXT_SELECTION = 0x09,
    CRSF_PT_STRING         = 0x0A,
    CRSF_PT_FOLDER         = 0x0B,
    CRSF_PT_INFO           = 0x0C,
    CRSF_PT_COMMAND        = 0x0D,
    CRSF_PT_OUT_OF_RANGE   = 0x7F,
} CrsfParamType;

#define CRSF_PARAM_HIDDEN_BIT    0x80

//=============================================================================
// Command parameter step/status values
//=============================================================================
typedef enum {
    CRSF_CMD_READY               = 0,
    CRSF_CMD_START               = 1,
    CRSF_CMD_PROGRESS            = 2,
    CRSF_CMD_CONFIRMATION_NEEDED = 3,
    CRSF_CMD_CONFIRM             = 4,
    CRSF_CMD_CANCEL              = 5,
    CRSF_CMD_POLL                = 6,
} CrsfCommandStep;

//=============================================================================
// Limits
//=============================================================================
#define CRSF_MAX_FRAME_LEN       64   // full frame incl. sync/len/type/crc
#define CRSF_MAX_PAYLOAD_LEN     60   // type-specific payload

//=============================================================================
// CRC helpers
//=============================================================================

/** CRC8 poly 0xD5 — frame CRC over [type .. payload]. */
uint8_t crsf_crc8_d5(const uint8_t *data, size_t len);

/** CRC8 poly 0xBA — inner CRC for CRSF_FT_COMMAND (0x32) direct commands. */
uint8_t crsf_crc8_ba(const uint8_t *data, size_t len);

/**
 * Build a complete CRSF frame into `out` (capacity >= CRSF_MAX_FRAME_LEN).
 * `payload` is the frame payload immediately after the type byte (for
 * extended frames the first two bytes must be [dest, origin]).
 * Returns total frame length, or 0 on error.
 */
size_t crsf_frame_build(uint8_t *out, uint8_t sync, uint8_t frame_type,
                        const uint8_t *payload, size_t payload_len);

/**
 * Validate a complete frame buffer. On success returns true and fills
 * *frame_type, *payload (pointer into buf) and *payload_len.
 */
bool crsf_frame_validate(const uint8_t *buf, size_t buf_len,
                         uint8_t *frame_type,
                         const uint8_t **payload, size_t *payload_len);

#ifdef __cplusplus
}
#endif

#endif // CRSF_PROTOCOL_H
