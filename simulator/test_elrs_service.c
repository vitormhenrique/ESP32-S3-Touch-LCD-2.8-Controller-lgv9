#include "elrs_client.h"
#include "elrs_service.h"
#include "elrs_sim.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int g_write_verified;
static int g_write_failed;
static int g_cmd_ready;
static int g_msg_count;
static int g_warn_count;
static bool g_armed;
static uint32_t g_now;
static char g_last_msg[256];

static void msg_cb(ElrsMsgLevel level, const char *text, void *user)
{
    (void)user;
    g_msg_count++;
    if (level == ELRS_MSG_WARNING) g_warn_count++;
    strncpy(g_last_msg, text ? text : "", sizeof(g_last_msg) - 1);
    g_last_msg[sizeof(g_last_msg) - 1] = '\0';
}

static bool armed_cb(void *user)
{
    (void)user;
    return g_armed;
}

static void event_cb(ElrsClientEvent ev, uint8_t arg, void *user)
{
    (void)user;
    elrs_service_on_client_event(ev, arg);
    if (ev == ELRS_EV_WRITE_VERIFIED) g_write_verified++;
    if (ev == ELRS_EV_WRITE_FAILED) g_write_failed++;
    if (ev == ELRS_EV_CMD_STATUS) {
        const ElrsParam *p = elrs_client_param(arg);
        if (p && p->cmd_status == CRSF_CMD_READY) g_cmd_ready++;
    }
}

static void reset_counters(void)
{
    g_write_verified = 0;
    g_write_failed = 0;
    g_cmd_ready = 0;
    g_msg_count = 0;
    g_warn_count = 0;
    g_armed = false;
    g_now = 0;
    g_last_msg[0] = '\0';
}

static void drive_ms(uint32_t ms)
{
    uint32_t end = g_now + ms;
    while (g_now <= end) {
        elrs_client_poll(g_now);
        elrs_service_poll(g_now);
        elrs_sim_poll(g_now);
        g_now += 10;
    }
}

static void reset_messages_only(void)
{
    g_msg_count = 0;
    g_warn_count = 0;
    g_last_msg[0] = '\0';
}

static void load_variant(ElrsSimTxVariant variant, uint32_t baud)
{
    reset_counters();
    elrs_sim_select_tx_variant(variant);
    elrs_client_init(elrs_sim_send_frame, NULL);
    elrs_service_init();
    elrs_service_set_msg_cb(msg_cb, NULL);
    elrs_service_set_armed_cb(armed_cb, NULL);
    elrs_service_set_baud(baud);
    elrs_client_set_event_cb(event_cb, NULL);
    elrs_client_select_device(CRSF_ADDR_TX_MODULE);
    drive_ms(6000);
    assert(elrs_client_params_ready());
    reset_messages_only();
    elrs_service_check_module();
}

static void assert_write_option(const char *param_name, const char *option)
{
    const ElrsParam *p = elrs_service_find(param_name);
    assert(p != NULL);
    int idx = elrs_service_find_option(p, option);
    assert(idx >= 0);
    int before = g_write_verified;
    assert(elrs_service_set(param_name, option));
    drive_ms(1200);
    p = elrs_service_find(param_name);
    assert(p != NULL);
    assert(p->value == idx);
    assert(g_write_verified > before);
    assert(g_write_failed == 0);
}

static void test_happymodel_detection(void)
{
    load_variant(ELRS_SIM_TX_HAPPYMODEL_ES24_PRO, 921000);
    const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
    assert(profile->family == ELRS_TX_FAMILY_HAPPYMODEL_ES24_PRO);
    assert(profile->hasOledExpected == false);
    assert(profile->hasFiveDButtonExpected == false);
    assert(profile->hasFanExpected == true);
    assert(profile->hasBackpackExpected == true);
    assert(profile->hasRgbExpected == true);
    assert(profile->maxExpectedPowerMw == 1000);
    assert(elrs_service_discovered_max_power_mw() == 1000);
}

static void test_betafpv_1w_detection(void)
{
    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_1W, 921000);
    const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
    assert(profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_1W);
    assert(profile->hasOledExpected == true);
    assert(profile->hasFiveDButtonExpected == true);
    assert(profile->hasFanExpected == true);
    assert(profile->hasBackpackExpected == true);
    assert(profile->hasRgbExpected == true);
    assert(profile->maxExpectedPowerMw == 1000);
    assert(elrs_service_discovered_max_power_mw() == 1000);
}

static void test_betafpv_500_detection_and_limits(void)
{
    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_500MW, 921000);
    const ElrsTxHardwareProfile *profile = elrs_service_tx_profile();
    assert(profile->family == ELRS_TX_FAMILY_BETAFPV_MICRO_500MW);
    assert(profile->hasOledExpected == true);
    assert(profile->hasFiveDButtonExpected == true);
    assert(profile->maxExpectedPowerMw == 500);
    assert(elrs_service_discovered_max_power_mw() == 500);
    assert(!elrs_service_set("Max Power", "1000"));
}

static void test_shared_protocol_writes_and_commands(void)
{
    load_variant(ELRS_SIM_TX_HAPPYMODEL_ES24_PRO, 921000);
    assert_write_option("Packet Rate", "100Hz Full");
    assert_write_option("Telem Ratio", "Std");
    assert_write_option("Switch Mode", "16ch Rate/2");
    assert_write_option("Max Power", "25");
    assert_write_option("Dynamic", "Off");

    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_1W, 921000);
    assert_write_option("Packet Rate", "100Hz Full");
    assert_write_option("Telem Ratio", "Std");
    assert_write_option("Switch Mode", "16ch Rate/2 Full Res");
    assert_write_option("Max Power", "25");
    assert_write_option("Dynamic", "Off");

    const ElrsParam *bind = elrs_service_find("Bind");
    assert(bind != NULL);
    assert(elrs_client_command_start(bind->id));
    drive_ms(5000);
    assert(g_cmd_ready > 0);

    const ElrsParam *wifi = elrs_service_find("Enable WiFi");
    assert(wifi != NULL);
    assert(elrs_client_command_start(wifi->id));
    drive_ms(500);
    assert(elrs_client_command_confirm(wifi->id));
    drive_ms(7000);
    assert(g_cmd_ready > 1);

    const ElrsParam *ble = elrs_service_find("BLE Joystick");
    assert(ble != NULL);
    assert(elrs_client_command_start(ble->id));
    drive_ms(5000);
    assert(g_cmd_ready > 2);
}

static void test_power_safety(void)
{
    load_variant(ELRS_SIM_TX_HAPPYMODEL_ES24_PRO, 921000);
    const ElrsParam *max_power = elrs_service_find("Max Power");
    int idx_1000 = elrs_service_find_option(max_power, "1000");
    ElrsWriteSafety safety = elrs_service_classify(max_power, idx_1000);
    assert(safety.needs_confirm);
    assert(safety.needs_disarm);
    g_armed = true;
    assert(!elrs_service_set("Max Power", "1000"));
    g_armed = false;
    assert(!elrs_service_set("Max Power", "2000"));
    assert(!elrs_service_set("Max Power", "5"));

    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_1W, 921000);
    max_power = elrs_service_find("Max Power");
    idx_1000 = elrs_service_find_option(max_power, "1000");
    safety = elrs_service_classify(max_power, idx_1000);
    assert(safety.needs_confirm);
    assert(safety.needs_disarm);
    assert(strstr(safety.warning, "1W mode") != NULL);

    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_500MW, 921000);
    max_power = elrs_service_find("Max Power");
    int idx_500 = elrs_service_find_option(max_power, "500");
    safety = elrs_service_classify(max_power, idx_500);
    assert(safety.needs_confirm);
    assert(strstr(safety.warning, "500mW") != NULL);
}

static void test_vendor_warnings(void)
{
    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_1W, 921000);
    assert(g_warn_count >= 2);
    assert(elrs_service_tx_profile()->hasOledExpected);

    load_variant(ELRS_SIM_TX_HAPPYMODEL_ES24_PRO, 921000);
    assert(!elrs_service_tx_profile()->hasOledExpected);
}

static void test_baud_rate_warnings(void)
{
    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_1W, 400000);
    const ElrsParam *rate = elrs_service_find("Packet Rate");
    int idx = elrs_service_find_option(rate, "333Hz Full");
    ElrsWriteSafety safety = elrs_service_classify(rate, idx);
    assert(strstr(safety.warning, "CRSF baud >400K") != NULL);

    load_variant(ELRS_SIM_TX_BETAFPV_MICRO_1W, 921000);
    rate = elrs_service_find("Packet Rate");
    idx = elrs_service_find_option(rate, "333Hz Full");
    safety = elrs_service_classify(rate, idx);
    assert(safety.warning[0] == '\0');
    assert(elrs_service_find_option(rate, "F1000") >= 0);

    const ElrsParam *sw = elrs_service_find("Switch Mode");
    int sw_idx = elrs_service_find_option(sw, "16ch Rate/2 Full Res");
    safety = elrs_service_classify(sw, sw_idx);
    assert(strstr(safety.warning, "Power off receiver") != NULL);
}

int main(void)
{
    test_happymodel_detection();
    test_betafpv_1w_detection();
    test_betafpv_500_detection_and_limits();
    test_shared_protocol_writes_and_commands();
    test_power_safety();
    test_vendor_warnings();
    test_baud_rate_warnings();
    puts("ELRS service tests passed");
    return 0;
}
