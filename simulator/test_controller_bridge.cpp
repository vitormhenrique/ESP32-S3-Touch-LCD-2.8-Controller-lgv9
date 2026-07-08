#include "controller_bridge.h"

#include <assert.h>
#include <math.h>
#include <string.h>

static bool closef(float a, float b, float eps = 0.01f)
{
    return fabsf(a - b) <= eps;
}

static uint16_t unitToCrsf(float unit)
{
    if (unit < 0.0f) unit = 0.0f;
    if (unit > 1.0f) unit = 1.0f;
    return uint16_t(float(CPACK_CRSF_MIN) +
                    unit * float(CPACK_CRSF_MAX - CPACK_CRSF_MIN));
}

static uint16_t binToCrsf(uint8_t index, uint8_t bins)
{
    assert(bins > 0);
    if (index >= bins) index = bins - 1;
    return unitToCrsf((float(index) + 0.5f) / float(bins));
}

static void centerChannels(uint16_t ch[CPACK_NUM_CHANNELS])
{
    for (int i = 0; i < CPACK_NUM_CHANNELS; ++i) ch[i] = CPACK_CRSF_MID;
}

static const ControllerCommand& lockCustom(ControllerBridge& bridge,
                                           const uint16_t ch[CPACK_NUM_CHANNELS])
{
    const ControllerCommand* cmd = nullptr;
    for (int i = 0; i < 3; ++i) {
        cmd = &bridge.update(ch, true, uint32_t(i * 20));
    }
    assert(bridge.profileLocked());
    assert(bridge.detectedProfile() == InputProfile::CustomControllerChannelPack);
    return *cmd;
}

static const ControllerCommand& lockTx(ControllerBridge& bridge,
                                       const uint16_t ch[CPACK_NUM_CHANNELS])
{
    const ControllerCommand* cmd = nullptr;
    for (int i = 0; i < 3; ++i) {
        cmd = &bridge.update(ch, true, uint32_t(i * 20));
    }
    assert(bridge.profileLocked());
    assert(bridge.detectedProfile() == InputProfile::Tx16sMk3Direct);
    return *cmd;
}

static void makeTxChannels(uint16_t ch[CPACK_NUM_CHANNELS],
                           uint8_t safety_mask,
                           uint8_t feature_mask,
                           uint8_t action,
                           bool fire)
{
    centerChannels(ch);
    ch[0] = CPACK_CRSF_MIN;
    ch[1] = CPACK_CRSF_MAX;
    ch[2] = CPACK_CRSF_MID;
    ch[3] = unitToCrsf(0.75f);
    ch[4] = unitToCrsf(0.25f);
    ch[5] = unitToCrsf(0.80f);
    ch[6] = unitToCrsf(0.25f);
    ch[7] = unitToCrsf(0.75f);
    ch[8] = CPACK_CRSF_MAX;
    ch[9] = CPACK_CRSF_MID;
    ch[10] = binToCrsf(safety_mask, 4);
    ch[11] = binToCrsf(feature_mask, 16);
    ch[12] = binToCrsf(action, 13);
    ch[13] = fire ? CPACK_CRSF_MAX : CPACK_CRSF_MIN;
    ch[14] = CPACK_CRSF_MID;
    ch[15] = CPACK_CRSF_MID;
}

static void test_custom_detection_and_decode(void)
{
    ChannelPackInputs_t in;
    memset(&in, 0, sizeof(in));
    in.gimbal[0] = -500;
    in.gimbal[1] = 250;
    in.gimbal[2] = 750;
    in.gimbal[3] = -1000;
    in.pot[0] = 123;
    in.pot[1] = 987;
    in.encoder[0] = 2040;
    in.encoder[1] = 12;
    in.switches[0] = true;
    in.switches[2] = true;
    in.switches[5] = true;
    in.buttons[3] = true;
    in.toggles[0] = 2;
    in.toggles[1] = 0;
    in.nav[0][CPACK_NAV_UP] = true;
    in.nav[0][CPACK_NAV_CENTER] = true;
    in.nav[1][CPACK_NAV_LEFT] = true;

    uint16_t ch[CPACK_NUM_CHANNELS];
    ChannelPack::packInputs(&in, ch);

    ControllerBridge bridge;
    const ControllerCommand& cmd0 = bridge.update(ch, true, 0);
    assert(!bridge.profileLocked());
    assert(cmd0.failsafe);
    bridge.update(ch, true, 20);
    const ControllerCommand& cmd = lockCustom(bridge, ch);
    assert(!cmd.failsafe);

    const ChannelPackInputs_t& raw = bridge.rawInputs();
    assert(raw.gimbal[0] == ChannelPack::crsfToGimbal(ch[0]));
    assert(raw.gimbal[1] == ChannelPack::crsfToGimbal(ch[1]));
    assert(raw.pot[0] == ChannelPack::crsfToPot(ch[4]));
    assert(raw.encoder[0] == (int32_t)(ch[6] & 0x7FF));
    assert(raw.switches[0]);
    assert(raw.switches[2]);
    assert(raw.switches[5]);
    assert(!raw.switches[6]);
    assert(!raw.switches[7]);
    assert(raw.buttons[3]);
    assert(raw.toggles[0] == 2);
    assert(raw.toggles[1] == 0);
    assert(raw.nav[0][CPACK_NAV_UP]);
    assert(raw.nav[0][CPACK_NAV_CENTER]);
    assert(raw.nav[1][CPACK_NAV_LEFT]);

    uint16_t invalid[CPACK_NUM_CHANNELS];
    memcpy(invalid, ch, sizeof(invalid));
    invalid[CPACK_CH_SWITCHES] |= 0x00C0;
    ControllerBridge invalid_bridge;
    lockTx(invalid_bridge, invalid);
}

static void assertOnlyAction(const ChannelPackInputs_t& raw, uint8_t action)
{
    bool expected_buttons[4] = { false, false, false, false };
    bool expected_nav[2][5] = {};

    switch (action) {
    case 0: expected_nav[0][CPACK_NAV_UP] = true; break;
    case 1: expected_nav[0][CPACK_NAV_DOWN] = true; break;
    case 2: expected_nav[0][CPACK_NAV_LEFT] = true; break;
    case 3: expected_nav[0][CPACK_NAV_RIGHT] = true; break;
    case 4: expected_nav[0][CPACK_NAV_CENTER] = true; break;
    case 5: expected_buttons[0] = true; break;
    case 6: expected_buttons[1] = true; break;
    case 7: expected_buttons[2] = true; break;
    case 8: expected_buttons[3] = true; break;
    case 9: expected_nav[1][CPACK_NAV_UP] = true; break;
    case 10: expected_nav[1][CPACK_NAV_DOWN] = true; break;
    case 11: expected_nav[1][CPACK_NAV_LEFT] = true; break;
    case 12: expected_nav[1][CPACK_NAV_CENTER] = true; break;
    default: break;
    }

    for (int i = 0; i < 4; ++i) assert(raw.buttons[i] == expected_buttons[i]);
    for (int n = 0; n < 2; ++n) {
        for (int d = 0; d < 5; ++d) {
            assert(raw.nav[n][d] == expected_nav[n][d]);
        }
    }
    assert(!raw.nav[1][CPACK_NAV_RIGHT]);
}

static void test_tx16s_detection_decode_and_actions(void)
{
    uint16_t ch[CPACK_NUM_CHANNELS];
    makeTxChannels(ch, 3, 0x0B, 8, true);

    ControllerBridge bridge;
    const ControllerCommand& cmd = lockTx(bridge, ch);
    assert(!cmd.failsafe);
    assert(cmd.estop);
    assert(!cmd.arm_request);
    assert(cmd.feat_foot_contact);
    assert(cmd.feat_terrain_leveling);
    assert(!cmd.feat_passive_pose);
    assert(cmd.host_authority);
    assert(cmd.mode == ControlMode::RotateBody);
    assert(cmd.gait == 1);

    const ChannelPackInputs_t& raw = bridge.rawInputs();
    assert(raw.gimbal[0] == -1000);
    assert(raw.gimbal[1] == 1000);
    assert(raw.pot[0] >= 249 && raw.pot[0] <= 251);
    assert(raw.pot[1] >= 799 && raw.pot[1] <= 801);
    assert(raw.encoder[0] >= 511 && raw.encoder[0] <= 513);
    assert(raw.encoder[1] >= 1534 && raw.encoder[1] <= 1536);
    assert(closef(bridge.encoderAccum(0), 0.25f));
    assert(closef(bridge.encoderAccum(1), 0.75f));
    assert(raw.toggles[0] == 2);
    assert(raw.toggles[1] == 1);
    assert(raw.switches[0]);
    assert(raw.switches[1]);
    assert(raw.switches[2]);
    assert(raw.switches[3]);
    assert(!raw.switches[4]);
    assert(raw.switches[5]);
    assert(!raw.switches[6]);
    assert(!raw.switches[7]);
    assertOnlyAction(raw, 8);

    makeTxChannels(ch, 1, 0x00, 4, false);
    bridge.update(ch, true, 100);
    const ChannelPackInputs_t& inactive = bridge.rawInputs();
    for (int i = 0; i < 4; ++i) assert(!inactive.buttons[i]);
    for (int n = 0; n < 2; ++n) {
        for (int d = 0; d < 5; ++d) assert(!inactive.nav[n][d]);
    }

    for (uint8_t action = 0; action <= 12; ++action) {
        makeTxChannels(ch, 1, 0x00, action, true);
        bridge.update(ch, true, 200 + action);
        assertOnlyAction(bridge.rawInputs(), action);
    }
}

static void test_lock_once_behavior(void)
{
    uint16_t tx[CPACK_NUM_CHANNELS];
    makeTxChannels(tx, 1, 0x00, 0, false);

    ControllerBridge bridge;
    lockTx(bridge, tx);

    ChannelPackInputs_t in;
    memset(&in, 0, sizeof(in));
    in.switches[0] = true;
    uint16_t custom[CPACK_NUM_CHANNELS];
    ChannelPack::packInputs(&in, custom);
    bridge.update(custom, true, 100);
    assert(bridge.detectedProfile() == InputProfile::Tx16sMk3Direct);

    ControllerBridge inverse;
    lockCustom(inverse, custom);
    inverse.update(tx, true, 100);
    assert(inverse.detectedProfile() == InputProfile::CustomControllerChannelPack);
}

static void test_link_loss_behavior(void)
{
    uint16_t tx[CPACK_NUM_CHANNELS];
    makeTxChannels(tx, 1, 0x00, 0, false);

    ControllerBridge bridge;
    lockTx(bridge, tx);
    const ControllerCommand& lost = bridge.update(tx, false, 100);
    assert(lost.failsafe);
    assert(bridge.profileLocked());
    assert(bridge.detectedProfile() == InputProfile::Tx16sMk3Direct);

    const ControllerCommand& restored = bridge.update(tx, true, 120);
    assert(!restored.failsafe);
    assert(bridge.detectedProfile() == InputProfile::Tx16sMk3Direct);

    ControllerBridge pending;
    pending.update(tx, true, 0);
    pending.update(tx, true, 20);
    assert(!pending.profileLocked());
    pending.update(tx, false, 40);
    assert(!pending.profileLocked());
    assert(pending.detectedProfile() == InputProfile::Unknown);

    ChannelPackInputs_t in;
    memset(&in, 0, sizeof(in));
    uint16_t custom[CPACK_NUM_CHANNELS];
    ChannelPack::packInputs(&in, custom);
    lockCustom(pending, custom);
}

static void test_safety_behavior(void)
{
    uint16_t tx[CPACK_NUM_CHANNELS];
    centerChannels(tx);
    ControllerBridge bridge;
    const ControllerCommand& no_link = bridge.update(tx, false, 0);
    assert(no_link.failsafe);
    assert(!bridge.profileLocked());

    makeTxChannels(tx, 2, 0x00, 0, false);
    ControllerBridge estop2;
    const ControllerCommand& cmd2 = lockTx(estop2, tx);
    assert(cmd2.estop);
    assert(!cmd2.arm_request);

    makeTxChannels(tx, 3, 0x00, 0, false);
    ControllerBridge estop3;
    const ControllerCommand& cmd3 = lockTx(estop3, tx);
    assert(cmd3.estop);
    assert(!cmd3.arm_request);
}

static void test_clamp_and_normalization(void)
{
    uint16_t ch[CPACK_NUM_CHANNELS];
    makeTxChannels(ch, 0, 0, 12, false);
    ch[0] = 0;
    ch[1] = 3000;
    ch[4] = 0;
    ch[5] = 3000;
    ch[6] = 0;
    ch[7] = 3000;
    ch[10] = CPACK_CRSF_MAX;
    ch[11] = CPACK_CRSF_MAX;
    ch[12] = CPACK_CRSF_MAX;
    ch[13] = CPACK_CRSF_MIN;
    ch[14] = CPACK_CRSF_MID;
    ch[15] = CPACK_CRSF_MID;

    ControllerBridge bridge;
    const ControllerCommand& cmd = lockTx(bridge, ch);
    const ChannelPackInputs_t& raw = bridge.rawInputs();
    assert(raw.gimbal[0] == -1000);
    assert(raw.gimbal[1] == 1000);
    assert(raw.pot[0] == 0);
    assert(raw.pot[1] == 1000);
    assert(raw.encoder[0] == 0);
    assert(raw.encoder[1] == 2047);
    assert(closef(bridge.encoderAccum(0), 0.0f));
    assert(closef(bridge.encoderAccum(1), 1.0f));
    assert(raw.switches[0]);
    assert(raw.switches[1]);
    assert(raw.switches[2]);
    assert(raw.switches[3]);
    assert(raw.switches[4]);
    assert(raw.switches[5]);
    assert(cmd.estop);
    assert(!cmd.arm_request);
    for (int i = 0; i < 4; ++i) assert(!raw.buttons[i]);
    for (int n = 0; n < 2; ++n) {
        for (int d = 0; d < 5; ++d) assert(!raw.nav[n][d]);
    }
}

int main()
{
    test_custom_detection_and_decode();
    test_tx16s_detection_decode_and_actions();
    test_lock_once_behavior();
    test_link_loss_behavior();
    test_safety_behavior();
    test_clamp_and_normalization();
    return 0;
}
