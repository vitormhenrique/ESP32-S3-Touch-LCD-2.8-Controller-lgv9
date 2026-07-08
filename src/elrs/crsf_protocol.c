/**
 * @file crsf_protocol.c
 * CRSF framing + CRC implementations. Pure C99.
 */
#include "crsf_protocol.h"
#include <string.h>

static uint8_t crc8_calc(uint8_t poly, const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (uint8_t)((crc & 0x80) ? (crc << 1) ^ poly : (crc << 1));
        }
    }
    return crc;
}

uint8_t crsf_crc8_d5(const uint8_t *data, size_t len)
{
    return crc8_calc(0xD5, data, len);
}

uint8_t crsf_crc8_ba(const uint8_t *data, size_t len)
{
    return crc8_calc(0xBA, data, len);
}

size_t crsf_frame_build(uint8_t *out, uint8_t sync, uint8_t frame_type,
                        const uint8_t *payload, size_t payload_len)
{
    if (payload_len > CRSF_MAX_PAYLOAD_LEN) return 0;
    if (payload_len > 0 && payload == NULL) return 0;

    // [sync][len][type][payload...][crc]  len = type + payload + crc
    out[0] = sync;
    out[1] = (uint8_t)(payload_len + 2);
    out[2] = frame_type;
    if (payload_len) memcpy(&out[3], payload, payload_len);
    out[3 + payload_len] = crsf_crc8_d5(&out[2], payload_len + 1);
    return payload_len + 4;
}

bool crsf_frame_validate(const uint8_t *buf, size_t buf_len,
                         uint8_t *frame_type,
                         const uint8_t **payload, size_t *payload_len)
{
    if (buf_len < 4) return false;
    uint8_t len = buf[1];                       // type + payload + crc
    if (len < 2 || (size_t)(len + 2) > buf_len) return false;
    if (len + 2u > CRSF_MAX_FRAME_LEN) return false;

    uint8_t crc = crsf_crc8_d5(&buf[2], (size_t)len - 1);
    if (crc != buf[2 + len - 1]) return false;

    if (frame_type)  *frame_type = buf[2];
    if (payload)     *payload = &buf[3];
    if (payload_len) *payload_len = (size_t)len - 2;
    return true;
}
