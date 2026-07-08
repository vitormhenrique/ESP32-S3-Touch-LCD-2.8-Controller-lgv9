/**
 * @file elrs_sim.c
 * Simulated ExpressLRS devices for the desktop simulator:
 *   - 0xEE "HappyModel ES24TX Pro" TX module (full parameter tree)
 *   - 0xEC "ELRS 2400 RX" receiver
 *
 * Implements the CRSF config protocol: device ping/info, chunked parameter
 * read, parameter write and the command state machine — with artificial
 * response latency so the client's async path is exercised.
 */
#include "elrs_sim.h"
#include "crsf_protocol.h"
#include "elrs_client.h"
#include <string.h>
#include <stdio.h>

#define SIM_LATENCY_MS   25
#define CHUNK_DATA_MAX   56   // payload budget per settings-entry chunk

//=============================================================================
// Parameter tables
//=============================================================================
typedef struct {
    uint8_t     parent;
    uint8_t     type;         // CrsfParamType | CRSF_PARAM_HIDDEN_BIT
    const char *name;
    const char *options;      // TEXT_SELECTION option list
    uint8_t     value;        // current selection / value
    const char *unit;
    const char *text;         // INFO/STRING value
    // command runtime state
    uint8_t     cmd_status;
    char        cmd_info[40];
    uint32_t    cmd_auto_ms;  // when to auto-transition (0 = none)
} SimParam;

static SimParam tx_params[] = {
    // parent, type, name, options, value, unit, text
    { 0, CRSF_PT_TEXT_SELECTION, "Packet Rate",
      "50Hz;100Hz Full;150Hz;250Hz;333Hz Full;500Hz", 2, "Hz", NULL,
      0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Telem Ratio",
      "Std;Off;1:128;1:64;1:32;1:16;1:8;1:4;1:2;Race", 0, "", NULL,
      0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Switch Mode",
      "8ch;16ch Rate/2;12ch Mixed", 1, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Model Match",
      "Off;On", 0, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_FOLDER, "TX Power", NULL, 0, "", NULL, 0, "", 0 },   // id 5
    { 5, CRSF_PT_TEXT_SELECTION, "Max Power",
      "10;25;50;100;250;500;1000", 1, "mW", NULL, 0, "", 0 },
    { 5, CRSF_PT_TEXT_SELECTION, "Dynamic",
      "Off;Dyn;AUX9;AUX10;AUX11;AUX12", 0, "", NULL, 0, "", 0 },
    { 5, CRSF_PT_TEXT_SELECTION, "Fan Thresh",
      "10mW;25mW;50mW;100mW;250mW;500mW;1000mW", 4, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_FOLDER, "VTX Administrator", NULL, 0, "", NULL, 0, "", 0 }, // 9
    { 9, CRSF_PT_TEXT_SELECTION, "Band",
      "Off;A;B;E;F;R;L", 0, "", NULL, 0, "", 0 },
    { 9, CRSF_PT_TEXT_SELECTION, "Channel",
      "1;2;3;4;5;6;7;8", 0, "", NULL, 0, "", 0 },
    { 9, CRSF_PT_TEXT_SELECTION, "Pwr Lvl",
      "-;1;2;3;4;5;6;7;8", 0, "", NULL, 0, "", 0 },
    { 9, CRSF_PT_COMMAND, "Send VTx", NULL, 0, "", NULL,
      CRSF_CMD_READY, "", 0 },
    { 0, CRSF_PT_FOLDER, "WiFi Connectivity", NULL, 0, "", NULL, 0, "", 0 }, // 14
    { 14, CRSF_PT_COMMAND, "Enable WiFi", NULL, 0, "", NULL,
      CRSF_CMD_READY, "", 0 },
    { 14, CRSF_PT_COMMAND, "Enable Rx WiFi", NULL, 0, "", NULL,
      CRSF_CMD_READY, "", 0 },
    { 0, CRSF_PT_COMMAND, "BLE Joystick", NULL, 0, "", NULL,
      CRSF_CMD_READY, "", 0 },
    { 0, CRSF_PT_COMMAND, "Bind", NULL, 0, "", NULL,
      CRSF_CMD_READY, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION | CRSF_PARAM_HIDDEN_BIT, "Telem Bandwidth",
      "Auto;Low;Med;High", 0, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_INFO, "Bad/Good", NULL, 0, "", "0/250", 0, "", 0 },
    { 0, CRSF_PT_INFO, "master f00dbabe", NULL, 0, "", "", 0, "", 0 },
};

static SimParam rx_params[] = {
    { 0, CRSF_PT_TEXT_SELECTION, "Protocol",
      "CRSF;Inverted CRSF;SBUS;Inverted SBUS;SUMD;MAVLink", 0, "", NULL,
      0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Antenna Mode",
      "Antenna B;Antenna A;Diversity", 2, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Tlm Power",
      "10mW;25mW;50mW;100mW", 1, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Team Race",
      "Off;On", 0, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_TEXT_SELECTION, "Bind Storage",
      "Persistent;Volatile;Returnable", 0, "", NULL, 0, "", 0 },
    { 0, CRSF_PT_COMMAND, "Enter Bind Mode", NULL, 0, "", NULL,
      CRSF_CMD_READY, "", 0 },
    { 0, CRSF_PT_INFO, "Model ID", NULL, 0, "", "Off", 0, "", 0 },
};

typedef struct {
    uint8_t     address;
    const char *name;
    SimParam   *params;
    uint8_t     count;
} SimDevice;

static SimDevice devices[] = {
    { CRSF_ADDR_TX_MODULE, "HappyModel ES24TX Pro",
      tx_params, (uint8_t)(sizeof(tx_params) / sizeof(tx_params[0])) },
    { CRSF_ADDR_RECEIVER, "ELRS 2400 RX",
      rx_params, (uint8_t)(sizeof(rx_params) / sizeof(rx_params[0])) },
};
#define SIM_DEVICE_COUNT (sizeof(devices) / sizeof(devices[0]))

//=============================================================================
// Response queue (adds latency)
//=============================================================================
typedef struct {
    bool     used;
    uint32_t due_ms;
    uint8_t  frame_type;
    uint8_t  payload[CRSF_MAX_PAYLOAD_LEN];
    uint8_t  len;
} SimResponse;

#define SIM_QUEUE_LEN 8
static SimResponse queue[SIM_QUEUE_LEN];
static uint32_t sim_now;

static void queue_response(uint8_t frame_type, const uint8_t *payload,
                           uint8_t len, uint32_t delay_ms)
{
    for (int i = 0; i < SIM_QUEUE_LEN; i++) {
        if (!queue[i].used) {
            queue[i].used = true;
            queue[i].due_ms = sim_now + delay_ms;
            queue[i].frame_type = frame_type;
            memcpy(queue[i].payload, payload, len);
            queue[i].len = len;
            return;
        }
    }
}

static SimDevice *find_device(uint8_t addr)
{
    for (size_t i = 0; i < SIM_DEVICE_COUNT; i++) {
        if (devices[i].address == addr) return &devices[i];
    }
    return NULL;
}

//=============================================================================
// Serialization
//=============================================================================
static size_t put_str(uint8_t *buf, size_t off, const char *s)
{
    size_t n = strlen(s);
    memcpy(&buf[off], s, n);
    buf[off + n] = 0;
    return off + n + 1;
}

static int option_count(const char *options)
{
    if (!options || !options[0]) return 0;
    int n = 1;
    for (const char *s = options; *s; s++) {
        if (*s == ';') n++;
    }
    return n;
}

/** Serialize the full settings-entry data for one field. */
static size_t serialize_field(const SimParam *p, uint8_t *buf)
{
    size_t o = 0;
    buf[o++] = p->parent;
    buf[o++] = p->type;
    o = put_str(buf, o, p->name);

    switch (p->type & 0x7F) {
    case CRSF_PT_TEXT_SELECTION:
        o = put_str(buf, o, p->options ? p->options : "");
        buf[o++] = p->value;                              // value
        buf[o++] = 0;                                     // min
        buf[o++] = (uint8_t)(option_count(p->options) - 1); // max
        buf[o++] = 0;                                     // default
        o = put_str(buf, o, p->unit ? p->unit : "");
        break;
    case CRSF_PT_INFO:
    case CRSF_PT_STRING:
        o = put_str(buf, o, p->text ? p->text : "");
        break;
    case CRSF_PT_COMMAND:
        buf[o++] = p->cmd_status;
        buf[o++] = 20;                                    // poll: 200 ms
        o = put_str(buf, o, p->cmd_info);
        break;
    case CRSF_PT_FOLDER:
    default:
        break;
    }
    return o;
}

//=============================================================================
// Request handlers
//=============================================================================
static void send_device_info(const SimDevice *dev)
{
    uint8_t pl[CRSF_MAX_PAYLOAD_LEN];
    size_t o = 0;
    pl[o++] = CRSF_ADDR_HANDSET;     // dest
    pl[o++] = dev->address;          // origin
    o = put_str(pl, o, dev->name);
    // serial number 'ELRS'
    pl[o++] = 'E'; pl[o++] = 'L'; pl[o++] = 'R'; pl[o++] = 'S';
    // hardware / software version
    memset(&pl[o], 0, 8); o += 8;
    pl[o++] = dev->count;            // field count
    pl[o++] = 0;                     // parameter protocol version
    queue_response(CRSF_FT_DEVICE_INFO, pl, (uint8_t)o, SIM_LATENCY_MS);
}

static void handle_read(uint8_t dest, uint8_t field_id, uint8_t chunk)
{
    SimDevice *dev = find_device(dest);
    if (!dev || field_id == 0 || field_id > dev->count) return;

    static uint8_t data[512];
    size_t total = serialize_field(&dev->params[field_id - 1], data);

    size_t chunks = (total + CHUNK_DATA_MAX - 1) / CHUNK_DATA_MAX;
    if (chunks == 0) chunks = 1;
    if (chunk >= chunks) return;

    size_t start = (size_t)chunk * CHUNK_DATA_MAX;
    size_t n = total - start;
    if (n > CHUNK_DATA_MAX) n = CHUNK_DATA_MAX;

    uint8_t pl[CRSF_MAX_PAYLOAD_LEN];
    pl[0] = CRSF_ADDR_HANDSET;
    pl[1] = dev->address;
    pl[2] = field_id;
    pl[3] = (uint8_t)(chunks - chunk - 1);   // chunks remaining
    memcpy(&pl[4], &data[start], n);
    queue_response(CRSF_FT_PARAM_SETTINGS_ENTRY, pl, (uint8_t)(n + 4),
                   SIM_LATENCY_MS);
}

static void command_step(SimDevice *dev, SimParam *p, uint8_t step)
{
    switch (step) {
    case CRSF_CMD_START:
        if (strcmp(p->name, "Enable WiFi") == 0) {
            p->cmd_status = CRSF_CMD_CONFIRMATION_NEEDED;
            snprintf(p->cmd_info, sizeof(p->cmd_info),
                     "Enter WiFi Update Mode?");
            p->cmd_auto_ms = 0;
        } else if (strcmp(p->name, "Bind") == 0 ||
                   strcmp(p->name, "Enter Bind Mode") == 0) {
            p->cmd_status = CRSF_CMD_PROGRESS;
            snprintf(p->cmd_info, sizeof(p->cmd_info), "Binding...");
            p->cmd_auto_ms = sim_now + 3000;
        } else {
            p->cmd_status = CRSF_CMD_PROGRESS;
            snprintf(p->cmd_info, sizeof(p->cmd_info), "Running...");
            p->cmd_auto_ms = sim_now + 4000;
        }
        break;
    case CRSF_CMD_CONFIRM:
        if (p->cmd_status == CRSF_CMD_CONFIRMATION_NEEDED) {
            p->cmd_status = CRSF_CMD_PROGRESS;
            snprintf(p->cmd_info, sizeof(p->cmd_info), "WiFi Running...");
            p->cmd_auto_ms = sim_now + 6000;
        }
        break;
    case CRSF_CMD_CANCEL:
        p->cmd_status = CRSF_CMD_READY;
        p->cmd_info[0] = '\0';
        p->cmd_auto_ms = 0;
        break;
    case CRSF_CMD_POLL:
    default:
        break;
    }
    (void)dev;
}

static void handle_write(uint8_t dest, const uint8_t *data, uint8_t len)
{
    SimDevice *dev = find_device(dest);
    if (!dev || len < 2) return;
    uint8_t field_id = data[0];
    if (field_id == 0 || field_id > dev->count) return;

    SimParam *p = &dev->params[field_id - 1];
    switch (p->type & 0x7F) {
    case CRSF_PT_COMMAND:
        command_step(dev, p, data[1]);
        break;
    case CRSF_PT_TEXT_SELECTION: {
        uint8_t v = data[1];
        int max = option_count(p->options) - 1;
        if (v <= max) p->value = v;
        printf("ELRS sim: %s <- %u\n", p->name, v);
        break;
    }
    default:
        break;
    }
}

//=============================================================================
// Public API
//=============================================================================
void elrs_sim_send_frame(uint8_t frame_type, const uint8_t *payload,
                         uint8_t len, void *user)
{
    (void)user;
    if (!payload || len < 2) return;
    uint8_t dest = payload[0];

    switch (frame_type) {
    case CRSF_FT_DEVICE_PING:
        for (size_t i = 0; i < SIM_DEVICE_COUNT; i++) {
            if (dest == CRSF_ADDR_BROADCAST || dest == devices[i].address) {
                send_device_info(&devices[i]);
            }
        }
        break;
    case CRSF_FT_PARAM_READ:
        if (len >= 4) handle_read(dest, payload[2], payload[3]);
        break;
    case CRSF_FT_PARAM_WRITE:
        handle_write(dest, &payload[2], (uint8_t)(len - 2));
        break;
    default:
        break;
    }
}

void elrs_sim_poll(uint32_t now_ms)
{
    sim_now = now_ms;

    // deliver due responses
    for (int i = 0; i < SIM_QUEUE_LEN; i++) {
        if (queue[i].used && now_ms >= queue[i].due_ms) {
            queue[i].used = false;
            elrs_client_on_frame(queue[i].frame_type, queue[i].payload,
                                 queue[i].len);
        }
    }

    // auto-transition running commands
    for (size_t d = 0; d < SIM_DEVICE_COUNT; d++) {
        for (uint8_t f = 0; f < devices[d].count; f++) {
            SimParam *p = &devices[d].params[f];
            if ((p->type & 0x7F) == CRSF_PT_COMMAND &&
                p->cmd_auto_ms != 0 && now_ms >= p->cmd_auto_ms) {
                p->cmd_auto_ms = 0;
                p->cmd_status = CRSF_CMD_READY;
                p->cmd_info[0] = '\0';
            }
        }
    }
}
