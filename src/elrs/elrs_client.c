/**
 * @file elrs_client.c
 * Dynamic CRSF/ExpressLRS parameter client implementation.
 * See elrs_client.h. Pure C99, fixed buffers, fully asynchronous.
 */
#include "elrs_client.h"
#include <string.h>
#include <stdio.h>

//=============================================================================
// Tunables
//=============================================================================
#define PING_TIMEOUT_MS    500
#define PING_RETRIES       3
#define READ_TIMEOUT_MS    300
#define READ_RETRIES       3
#define WRITE_ATTEMPTS     2      // initial + 1 retry

//=============================================================================
// State
//=============================================================================
typedef enum {
    ST_IDLE = 0,
    ST_PINGING,
    ST_LOADING,
} ClientState;

static struct {
    ElrsSendFn  send;
    void       *send_user;
    ElrsEventFn event_cb;
    void       *event_user;

    ClientState state;
    uint32_t    now_ms;

    // devices
    ElrsDeviceInfo devices[ELRS_MAX_DEVICES];
    int         device_count;
    uint8_t     sel_addr;          // selected device address, 0 = none
    bool        want_load;         // load params once device info arrives

    // ping
    uint32_t    ping_sent_ms;
    uint8_t     ping_retries;

    // parameter storage for the selected device
    ElrsParam   params[ELRS_MAX_PARAMS];
    uint8_t     param_count;       // from device info
    bool        params_ready;

    // read queue (stack, like the lua script)
    uint8_t     queue[ELRS_MAX_PARAMS];
    int         q_len;
    bool        awaiting;          // a chunk request is outstanding
    uint8_t     cur_chunk;
    uint8_t     retries;
    uint8_t     expect_chunks_remain;
    bool        has_expect;
    uint32_t    req_sent_ms;
    uint8_t     buf[ELRS_CHUNK_BUF_LEN];
    size_t      buf_len;
    bool        single_refresh;    // queue holds a single-field refresh

    // write verification
    bool        wr_pending;        // waiting for read-back
    uint8_t     wr_field;
    int32_t     wr_value;
    uint8_t     wr_attempts;

    // command state
    bool        cmd_active;
    uint8_t     cmd_field;
    uint32_t    cmd_next_poll_ms;
    uint32_t    cmd_poll_interval;
} c;

//=============================================================================
// Helpers
//=============================================================================
static void emit(ElrsClientEvent ev, uint8_t arg)
{
    if (c.event_cb) c.event_cb(ev, arg, c.event_user);
}

static void send_ext(uint8_t frame_type, uint8_t dest,
                     const uint8_t *data, uint8_t data_len)
{
    if (!c.send) return;
    uint8_t payload[CRSF_MAX_PAYLOAD_LEN];
    if ((size_t)data_len + 2 > sizeof(payload)) return;
    payload[0] = dest;
    payload[1] = CRSF_ADDR_ELRS_LUA;
    if (data_len) memcpy(&payload[2], data, data_len);
    c.send(frame_type, payload, (uint8_t)(data_len + 2), c.send_user);
}

static void request_chunk(void)
{
    if (c.q_len <= 0) return;
    uint8_t field = c.queue[c.q_len - 1];
    uint8_t data[2] = { field, c.cur_chunk };
    send_ext(CRSF_FT_PARAM_READ, c.sel_addr, data, 2);
    c.awaiting = true;
    c.req_sent_ms = c.now_ms;
}

static void queue_reset(void)
{
    c.q_len = 0;
    c.awaiting = false;
    c.cur_chunk = 0;
    c.retries = 0;
    c.has_expect = false;
    c.buf_len = 0;
    c.single_refresh = false;
}

static void queue_push_field(uint8_t field)
{
    if (c.q_len < (int)sizeof(c.queue)) c.queue[c.q_len++] = field;
}

/** Copy a null-terminated string out of a bounded buffer.
 *  Returns index just past the terminator, or -1 if unterminated. */
static int copy_str(const uint8_t *data, size_t len, size_t off,
                    char *out, size_t out_len)
{
    size_t i = off;
    size_t o = 0;
    while (i < len) {
        uint8_t ch = data[i++];
        if (ch == 0) {
            if (out && out_len) out[o < out_len ? o : out_len - 1] = '\0';
            return (int)i;
        }
        if (out && o < out_len - 1) out[o++] = (char)ch;
    }
    return -1; // unterminated — malformed
}

static int32_t get_be(const uint8_t *data, size_t off, int size, bool sign)
{
    int32_t v = 0;
    for (int i = 0; i < size; i++) v = (v << 8) | data[off + i];
    if (sign) {
        if (size == 1) v = (int8_t)v;
        else if (size == 2) v = (int16_t)v;
    }
    return v;
}

//=============================================================================
// Field data parsing (complete reassembled entry)
// layout: [parent][type|hidden][name\0][type-specific...]
//=============================================================================
static bool parse_field_data(uint8_t field_id, const uint8_t *d, size_t len)
{
    if (field_id == 0 || field_id > ELRS_MAX_PARAMS) return false;
    if (len < 3) return false;

    ElrsParam *p = &c.params[field_id - 1];
    memset(p, 0, sizeof(*p));
    p->id = field_id;
    p->parent = d[0];
    p->type = d[1] & 0x7F;
    p->hidden = (d[1] & CRSF_PARAM_HIDDEN_BIT) != 0;

    int off = copy_str(d, len, 2, p->name, sizeof(p->name));
    if (off < 0) return false;
    size_t o = (size_t)off;

    switch (p->type) {
    case CRSF_PT_UINT8: case CRSF_PT_INT8:
    case CRSF_PT_UINT16: case CRSF_PT_INT16: {
        bool sign = (p->type == CRSF_PT_INT8 || p->type == CRSF_PT_INT16);
        int size = (p->type <= CRSF_PT_INT8) ? 1 : 2;
        if (o + (size_t)size * 4 > len) return false;
        p->value = get_be(d, o, size, sign);
        p->min   = get_be(d, o + size, size, sign);
        p->max   = get_be(d, o + 2 * (size_t)size, size, sign);
        // default at o+3*size (unused)
        copy_str(d, len, o + 4 * (size_t)size, p->unit, sizeof(p->unit));
        break;
    }
    case CRSF_PT_FLOAT: {
        if (o + 21 > len) return false;
        p->value = get_be(d, o, 4, true);
        p->min   = get_be(d, o + 4, 4, true);
        p->max   = get_be(d, o + 8, 4, true);
        // default at o+12
        p->precision = d[o + 16];
        p->step  = get_be(d, o + 17, 4, true);
        copy_str(d, len, o + 21, p->unit, sizeof(p->unit));
        break;
    }
    case CRSF_PT_TEXT_SELECTION: {
        int no = copy_str(d, len, o, p->options, sizeof(p->options));
        if (no < 0) return false;
        o = (size_t)no;
        if (o + 3 > len) return false;
        p->value = d[o];
        p->min   = d[o + 1];
        p->max   = d[o + 2];
        copy_str(d, len, o + 4, p->unit, sizeof(p->unit)); // skip default byte
        break;
    }
    case CRSF_PT_STRING:
    case CRSF_PT_INFO:
        copy_str(d, len, o, p->str_value, sizeof(p->str_value));
        break;
    case CRSF_PT_FOLDER:
        break;
    case CRSF_PT_COMMAND: {
        if (o + 2 > len) return false;
        p->cmd_status  = d[o];
        p->cmd_timeout = d[o + 1];
        copy_str(d, len, o + 2, p->cmd_info, sizeof(p->cmd_info));
        break;
    }
    default:
        // unknown type — keep header info only, don't reject the tree
        break;
    }

    p->valid = true;
    return true;
}

//=============================================================================
// Frame handlers
//=============================================================================
static void handle_device_info(const uint8_t *pl, uint8_t len)
{
    // [dest][src][name\0][serial u32][hw u32][sw u32][field count][param ver]
    if (len < 4) return;
    uint8_t addr = pl[1];

    char name[32] = {0};
    int off = copy_str(pl, len, 2, name, sizeof(name));
    if (off < 0) return;
    if ((size_t)off + 13 > len) return;

    ElrsDeviceInfo *dev = NULL;
    for (int i = 0; i < c.device_count; i++) {
        if (c.devices[i].address == addr) { dev = &c.devices[i]; break; }
    }
    if (!dev) {
        if (c.device_count >= ELRS_MAX_DEVICES) return;
        dev = &c.devices[c.device_count++];
        memset(dev, 0, sizeof(*dev));
        dev->address = addr;
    }
    dev->present = true;
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    uint32_t serial = (uint32_t)get_be(pl, (size_t)off, 4, false);
    dev->is_elrs = (serial == 0x454C5253); // 'ELRS'
    dev->param_count = pl[off + 12];

    emit(ELRS_EV_DEVICE_FOUND, (uint8_t)(c.device_count - 1));

    // If we were waiting for this device to start loading params
    if (c.want_load && addr == c.sel_addr) {
        c.want_load = false;
        c.state = ST_IDLE;
        c.param_count = dev->param_count;
        if (c.param_count > ELRS_MAX_PARAMS) c.param_count = ELRS_MAX_PARAMS;
        elrs_client_reload_params();
    }
}

static void finish_field(uint8_t field_id, const uint8_t *d, size_t len)
{
    bool ok = parse_field_data(field_id, d, len);

    // pop from queue
    if (c.q_len > 0) c.q_len--;
    c.cur_chunk = 0;
    c.retries = 0;
    c.has_expect = false;
    c.buf_len = 0;
    c.awaiting = false;

    if (ok) {
        const ElrsParam *p = &c.params[field_id - 1];

        // write verification read-back
        if (c.wr_pending && field_id == c.wr_field) {
            c.wr_pending = false;
            if (p->value == c.wr_value) {
                emit(ELRS_EV_WRITE_VERIFIED, field_id);
            } else if (c.wr_attempts < WRITE_ATTEMPTS) {
                elrs_client_write_value(field_id, c.wr_value);
                return;
            } else {
                emit(ELRS_EV_WRITE_FAILED, field_id);
            }
        }

        // command status update
        if (c.cmd_active && field_id == c.cmd_field &&
            p->type == CRSF_PT_COMMAND) {
            uint32_t t = (uint32_t)p->cmd_timeout * 10;
            c.cmd_poll_interval = t < 100 ? 100 : t;
            c.cmd_next_poll_ms = c.now_ms + c.cmd_poll_interval;
            if (p->cmd_status == CRSF_CMD_READY) c.cmd_active = false;
            emit(ELRS_EV_CMD_STATUS, field_id);
        }
    }

    if (c.q_len > 0) {
        request_chunk();
    } else {
        bool was_single = c.single_refresh;
        c.single_refresh = false;
        if (c.state == ST_LOADING) {
            c.state = ST_IDLE;
            c.params_ready = true;
            emit(ELRS_EV_PARAMS_LOADED, 0);
        } else if (was_single && ok) {
            emit(ELRS_EV_PARAM_UPDATED, field_id);
        }
        if (c.state == ST_LOADING || !was_single) {
            // progress event during bulk loads handled below
        }
    }

    if (c.state == ST_LOADING) {
        emit(ELRS_EV_LOAD_PROGRESS,
             (uint8_t)(c.param_count - (c.q_len > 255 ? 255 : c.q_len)));
    }
}

static void handle_param_entry(const uint8_t *pl, uint8_t len)
{
    // [dest][src][field id][chunks remain][data...]
    if (len < 4) return;
    if (pl[1] != c.sel_addr) return;
    if (c.q_len <= 0) return;

    uint8_t field_id = pl[2];
    uint8_t chunks_remain = pl[3];
    if (field_id != c.queue[c.q_len - 1]) return; // not what we asked for

    if (c.has_expect && chunks_remain != c.expect_chunks_remain) {
        // stream desync — restart this field
        c.cur_chunk = 0;
        c.buf_len = 0;
        c.has_expect = false;
        c.awaiting = false;
        return;
    }

    const uint8_t *chunk = &pl[4];
    size_t chunk_len = (size_t)len - 4;

    if (chunks_remain > 0 || c.cur_chunk > 0) {
        // accumulate
        if (c.buf_len + chunk_len > sizeof(c.buf)) { // overflow — abort field
            c.cur_chunk = 0; c.buf_len = 0; c.has_expect = false;
            c.awaiting = false;
            return;
        }
        memcpy(&c.buf[c.buf_len], chunk, chunk_len);
        c.buf_len += chunk_len;
    }

    if (chunks_remain > 0) {
        c.cur_chunk++;
        c.expect_chunks_remain = (uint8_t)(chunks_remain - 1);
        c.has_expect = true;
        c.retries = 0;
        request_chunk();
    } else {
        if (c.cur_chunk > 0) {
            finish_field(field_id, c.buf, c.buf_len);
        } else {
            finish_field(field_id, chunk, chunk_len);
        }
    }
}

//=============================================================================
// Public API
//=============================================================================
void elrs_client_init(ElrsSendFn send_fn, void *send_user)
{
    memset(&c, 0, sizeof(c));
    c.send = send_fn;
    c.send_user = send_user;
}

void elrs_client_set_event_cb(ElrsEventFn cb, void *user)
{
    c.event_cb = cb;
    c.event_user = user;
}

void elrs_client_ping(void)
{
    if (!c.send) return;
    uint8_t payload[2] = { CRSF_ADDR_BROADCAST, CRSF_ADDR_HANDSET };
    c.send(CRSF_FT_DEVICE_PING, payload, 2, c.send_user);
    c.ping_sent_ms = c.now_ms;
    if (c.state == ST_IDLE) c.state = ST_PINGING;
}

void elrs_client_select_device(uint8_t address)
{
    c.sel_addr = address;
    c.params_ready = false;
    c.param_count = 0;
    c.wr_pending = false;
    c.cmd_active = false;
    queue_reset();
    memset(c.params, 0, sizeof(c.params));

    const ElrsDeviceInfo *dev = elrs_client_device_by_addr(address);
    if (dev && dev->param_count > 0) {
        c.param_count = dev->param_count > ELRS_MAX_PARAMS
                            ? ELRS_MAX_PARAMS : dev->param_count;
        elrs_client_reload_params();
    } else {
        // need device info first
        c.want_load = true;
        c.ping_retries = 0;
        c.state = ST_PINGING;
        elrs_client_ping();
    }
}

uint8_t elrs_client_selected_device(void) { return c.sel_addr; }

void elrs_client_reload_params(void)
{
    if (c.sel_addr == 0 || c.param_count == 0) return;
    queue_reset();
    c.params_ready = false;
    // stack: push highest first so field 1 is read first
    for (int i = c.param_count; i >= 1; i--) queue_push_field((uint8_t)i);
    c.state = ST_LOADING;
    c.retries = 0;
    request_chunk();
}

void elrs_client_refresh_param(uint8_t param_id)
{
    if (c.sel_addr == 0 || param_id == 0) return;
    if (c.q_len > 0) {
        queue_push_field(param_id); // will be served next (stack)
    } else {
        queue_push_field(param_id);
        c.single_refresh = true;
        c.cur_chunk = 0;
        c.retries = 0;
        request_chunk();
    }
}

bool elrs_client_write_value(uint8_t param_id, int32_t value)
{
    if (!c.send || c.sel_addr == 0 || param_id == 0) return false;
    const ElrsParam *p = elrs_client_param(param_id);
    if (!p) return false;

    uint8_t data[6];
    uint8_t n = 0;
    data[n++] = param_id;
    switch (p->type) {
    case CRSF_PT_UINT8: case CRSF_PT_INT8: case CRSF_PT_TEXT_SELECTION:
        data[n++] = (uint8_t)value;
        break;
    case CRSF_PT_UINT16: case CRSF_PT_INT16:
        data[n++] = (uint8_t)(value >> 8);
        data[n++] = (uint8_t)value;
        break;
    case CRSF_PT_FLOAT:
        data[n++] = (uint8_t)(value >> 24);
        data[n++] = (uint8_t)(value >> 16);
        data[n++] = (uint8_t)(value >> 8);
        data[n++] = (uint8_t)value;
        break;
    default:
        return false;
    }
    send_ext(CRSF_FT_PARAM_WRITE, c.sel_addr, data, n);

    // track for verification
    if (c.wr_field == param_id && c.wr_value == value && c.wr_attempts > 0) {
        c.wr_attempts++;
    } else {
        c.wr_field = param_id;
        c.wr_value = value;
        c.wr_attempts = 1;
    }
    c.wr_pending = true;
    elrs_client_refresh_param(param_id);
    return true;
}

static bool send_cmd_step(uint8_t param_id, uint8_t step)
{
    if (!c.send || c.sel_addr == 0) return false;
    uint8_t data[2] = { param_id, step };
    send_ext(CRSF_FT_PARAM_WRITE, c.sel_addr, data, 2);
    return true;
}

bool elrs_client_command_start(uint8_t param_id)
{
    const ElrsParam *p = elrs_client_param(param_id);
    if (!p || p->type != CRSF_PT_COMMAND) return false;
    if (!send_cmd_step(param_id, CRSF_CMD_START)) return false;
    c.cmd_active = true;
    c.cmd_field = param_id;
    c.cmd_poll_interval = 200;
    c.cmd_next_poll_ms = c.now_ms + c.cmd_poll_interval;
    elrs_client_refresh_param(param_id);
    return true;
}

bool elrs_client_command_confirm(uint8_t param_id)
{
    if (!send_cmd_step(param_id, CRSF_CMD_CONFIRM)) return false;
    elrs_client_refresh_param(param_id);
    return true;
}

bool elrs_client_command_cancel(uint8_t param_id)
{
    bool ok = send_cmd_step(param_id, CRSF_CMD_CANCEL);
    c.cmd_active = false;
    if (ok) elrs_client_refresh_param(param_id);
    return ok;
}

void elrs_client_on_frame(uint8_t frame_type, const uint8_t *payload,
                          uint8_t len)
{
    if (!payload || len < 2) return;
    switch (frame_type) {
    case CRSF_FT_DEVICE_INFO:
        handle_device_info(payload, len);
        break;
    case CRSF_FT_PARAM_SETTINGS_ENTRY:
        handle_param_entry(payload, len);
        break;
    default:
        break;
    }
}

void elrs_client_poll(uint32_t now_ms)
{
    c.now_ms = now_ms;

    // ping retries while waiting for device info
    if (c.state == ST_PINGING && c.want_load) {
        if (now_ms - c.ping_sent_ms >= PING_TIMEOUT_MS) {
            if (c.ping_retries + 1 >= PING_RETRIES) {
                c.state = ST_IDLE;
                c.want_load = false;
                emit(ELRS_EV_TIMEOUT, 0);
            } else {
                c.ping_retries++;
                elrs_client_ping();
            }
        }
    }

    // read timeout / retry
    if (c.awaiting && c.q_len > 0 &&
        now_ms - c.req_sent_ms >= READ_TIMEOUT_MS) {
        if (c.retries + 1 >= READ_RETRIES) {
            // give up on this field
            uint8_t field = c.queue[c.q_len - 1];
            c.q_len--;
            c.cur_chunk = 0; c.buf_len = 0; c.has_expect = false;
            c.retries = 0;
            c.awaiting = false;
            if (c.wr_pending && field == c.wr_field) {
                c.wr_pending = false;
                emit(ELRS_EV_WRITE_FAILED, field);
            }
            if (c.q_len > 0) {
                request_chunk();
            } else if (c.state == ST_LOADING) {
                c.state = ST_IDLE;
                emit(ELRS_EV_TIMEOUT, field);
            }
        } else {
            c.retries++;
            request_chunk(); // re-request same chunk
        }
    }

    // stalled queue (e.g. refresh queued while awaiting was false)
    if (!c.awaiting && c.q_len > 0) {
        request_chunk();
    }

    // command polling
    if (c.cmd_active && now_ms >= c.cmd_next_poll_ms) {
        const ElrsParam *p = elrs_client_param(c.cmd_field);
        if (p && (p->cmd_status == CRSF_CMD_PROGRESS ||
                  p->cmd_status == CRSF_CMD_CONFIRMATION_NEEDED ||
                  p->cmd_status == CRSF_CMD_START)) {
            send_cmd_step(c.cmd_field, CRSF_CMD_POLL);
            elrs_client_refresh_param(c.cmd_field);
        }
        c.cmd_next_poll_ms = now_ms + c.cmd_poll_interval;
    }
}

//=============================================================================
// Accessors
//=============================================================================
int elrs_client_device_count(void) { return c.device_count; }

const ElrsDeviceInfo *elrs_client_device(int index)
{
    if (index < 0 || index >= c.device_count) return NULL;
    return &c.devices[index];
}

const ElrsDeviceInfo *elrs_client_device_by_addr(uint8_t address)
{
    for (int i = 0; i < c.device_count; i++) {
        if (c.devices[i].address == address) return &c.devices[i];
    }
    return NULL;
}

const ElrsParam *elrs_client_param(uint8_t param_id)
{
    if (param_id == 0 || param_id > ELRS_MAX_PARAMS) return NULL;
    const ElrsParam *p = &c.params[param_id - 1];
    return p->valid ? p : NULL;
}

uint8_t elrs_client_param_count(void) { return c.param_count; }
bool elrs_client_params_ready(void) { return c.params_ready; }

bool elrs_client_busy(void)
{
    return c.q_len > 0 || c.state == ST_LOADING || c.state == ST_PINGING ||
           c.wr_pending || c.cmd_active;
}

bool elrs_param_option(const ElrsParam *p, int index, char *buf, size_t buflen)
{
    if (!p || p->type != CRSF_PT_TEXT_SELECTION || index < 0 || !buf || !buflen)
        return false;
    const char *s = p->options;
    int i = 0;
    while (i < index) {
        s = strchr(s, ';');
        if (!s) return false;
        s++;
        i++;
    }
    const char *end = strchr(s, ';');
    size_t n = end ? (size_t)(end - s) : strlen(s);
    if (n >= buflen) n = buflen - 1;
    memcpy(buf, s, n);
    buf[n] = '\0';
    return true;
}

int elrs_param_option_count(const ElrsParam *p)
{
    if (!p || p->type != CRSF_PT_TEXT_SELECTION) return 0;
    if (p->options[0] == '\0') return 0;
    int n = 1;
    for (const char *s = p->options; *s; s++) {
        if (*s == ';') n++;
    }
    return n;
}
