#include "controller_bridge.h"

#include <string.h>

namespace {

static bool nearCrsfMid(uint16_t value)
{
    int diff = (int)value - (int)CPACK_CRSF_MID;
    if (diff < 0) diff = -diff;
    return diff <= 16;
}

static bool looksLikeCustomChannelPack(const uint16_t ch[CPACK_NUM_CHANNELS])
{
    const bool switches_ok = (ch[CPACK_CH_SWITCHES] & ~uint16_t(0x003F)) == 0;

    const uint16_t btn_toggle = ch[CPACK_CH_BTN_TOGGLE];
    const bool btn_toggle_ok =
        (btn_toggle & ~uint16_t(0x00FF)) == 0 &&
        (((btn_toggle >> 4) & 0x03) <= 2) &&
        (((btn_toggle >> 6) & 0x03) <= 2);

    const bool nav_ok = (ch[CPACK_CH_NAV] & ~uint16_t(0x03FF)) == 0;

    bool reserved_ok = true;
    for (int i = 11; i < CPACK_NUM_CHANNELS; ++i) {
        reserved_ok = reserved_ok && nearCrsfMid(ch[i]);
    }

    return switches_ok && btn_toggle_ok && nav_ok && reserved_ok;
}

static uint16_t clampCrsf(uint16_t value)
{
    if (value < CPACK_CRSF_MIN) return CPACK_CRSF_MIN;
    if (value > CPACK_CRSF_MAX) return CPACK_CRSF_MAX;
    return value;
}

static float clampfLocal(float value, float lo, float hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static float crsfUnit01(uint16_t value)
{
    value = clampCrsf(value);
    return float(value - CPACK_CRSF_MIN) /
           float(CPACK_CRSF_MAX - CPACK_CRSF_MIN);
}

static int16_t crsfToGimbalSafe(uint16_t value)
{
    const float unit = crsfUnit01(value);
    return int16_t((unit * 2000.0f) - 1000.0f);
}

static int16_t crsfToPotSafe(uint16_t value)
{
    return int16_t(crsfUnit01(value) * 1000.0f);
}

static uint8_t crsfToTri(uint16_t value)
{
    const float unit = crsfUnit01(value);
    if (unit < 0.3333f) return 0;
    if (unit < 0.6666f) return 1;
    return 2;
}

static bool crsfToBoolHigh(uint16_t value)
{
    return crsfUnit01(value) > 0.5f;
}

static uint8_t crsfToBins(uint16_t value, uint8_t bins)
{
    if (bins == 0) return 0;
    const float unit = crsfUnit01(value);
    int index = int(unit * float(bins));
    if (index < 0) index = 0;
    if (index >= int(bins)) index = int(bins) - 1;
    return uint8_t(index);
}

static void clearRawInputs(ChannelPackInputs_t* out)
{
    if (!out) return;
    for (int i = 0; i < 4; ++i) out->gimbal[i] = 0;
    for (int i = 0; i < 2; ++i) out->pot[i] = 0;
    for (int i = 0; i < 2; ++i) out->encoder[i] = 0;
    for (int i = 0; i < 8; ++i) out->switches[i] = false;
    for (int i = 0; i < 4; ++i) out->buttons[i] = false;
    for (int i = 0; i < 2; ++i) out->toggles[i] = 1;
    for (int nav = 0; nav < 2; ++nav) {
        for (int dir = 0; dir < 5; ++dir) out->nav[nav][dir] = false;
    }
}

static void clearCommand(ControllerCommand* out)
{
    memset(out, 0, sizeof(*out));
    out->failsafe = true;
    out->mode = ControlMode::Walk;
    out->trick = TrickId::None;
}

} // namespace

BindingConfig defaultBindings()
{
    BindingConfig b;
    b.walk_x = AxisSource::GimbalLX;
    b.walk_y = AxisSource::GimbalLY;
    b.body_x = AxisSource::GimbalRX;
    b.body_y = AxisSource::GimbalRY;
    b.speed = AxisSource::Pot1;
    b.body_height = AxisSource::Pot2;
    b.stride = AxisSource::Enc1;
    b.step_height = AxisSource::Enc2;
    b.mode = TriSource::SwE;
    b.gait = TriSource::SwF;
    b.arm = BoolSource::SwA;
    b.estop = BoolSource::SwB;
    b.feat_foot_contact = BoolSource::SwC;
    b.feat_terrain_leveling = BoolSource::SwD;
    b.feat_passive_pose = BoolSource::SwG;
    b.host_authority = BoolSource::SwH;
    b.trim_pitch_up = BoolSource::Nav1Up;
    b.trim_pitch_down = BoolSource::Nav1Down;
    b.trim_roll_left = BoolSource::Nav1Left;
    b.trim_roll_right = BoolSource::Nav1Right;
    b.trim_reset = BoolSource::Nav1Center;
    b.trick_stand_up = BoolSource::Btn1;
    b.trick_sit_down = BoolSource::Btn2;
    b.trick_wave = BoolSource::Btn3;
    b.trick_crouch_toggle = BoolSource::Btn4;
    b.trick_twirl = BoolSource::Nav2Up;
    b.trick_stretch = BoolSource::Nav2Down;
    b.trick_lean_look = BoolSource::Nav2Left;
    b.trick_dance_loop = BoolSource::Nav2Center;
    return b;
}

ControllerBridge::ControllerBridge(const BindingConfig& bindings)
    : bindings_(bindings)
{
    reset();
}

void ControllerBridge::reset()
{
    clearRawInputs(&raw_);
    clearRawInputs(&prev_raw_);
    clearCommand(&cmd_);
    enc_accum_[0] = 0.0f;
    enc_accum_[1] = 0.0f;
    enc_last_[0] = 0;
    enc_last_[1] = 0;
    enc_seen_[0] = false;
    enc_seen_[1] = false;
    detected_profile_ = InputProfile::Unknown;
    profile_locked_ = false;
    custom_layout_streak_ = 0;
    tx_direct_layout_streak_ = 0;
}

float ControllerBridge::encoderAccum(uint8_t index) const
{
    return index < 2 ? enc_accum_[index] : 0.0f;
}

const ControllerCommand& ControllerBridge::update(const uint16_t ch[CPACK_NUM_CHANNELS],
                                                  bool link_up,
                                                  uint32_t now_ms)
{
    if (!link_up) {
        if (!profile_locked_) {
            detected_profile_ = InputProfile::Unknown;
            custom_layout_streak_ = 0;
            tx_direct_layout_streak_ = 0;
        }
        enterFailsafe(now_ms);
        return cmd_;
    }

    if (!profile_locked_) {
        attemptProfileDetection(ch);
        if (!profile_locked_) {
            enterFailsafe(now_ms);
            return cmd_;
        }
    }

    prev_raw_ = raw_;

    if (detected_profile_ == InputProfile::CustomControllerChannelPack) {
        ChannelPack::unpackChannels(ch, &raw_);
        integrateEncoders();
    } else if (detected_profile_ == InputProfile::Tx16sMk3Direct) {
        unpackTx16sMk3DirectChannels(ch, &raw_);
        updateTx16sDirectVirtualEncoders(ch);
    } else {
        enterFailsafe(now_ms);
        return cmd_;
    }

    buildCommand(now_ms);
    return cmd_;
}

void ControllerBridge::attemptProfileDetection(const uint16_t ch[CPACK_NUM_CHANNELS])
{
    if (profile_locked_) return;

    if (looksLikeCustomChannelPack(ch)) {
        if (custom_layout_streak_ < kProfileDetectFrames) ++custom_layout_streak_;
        tx_direct_layout_streak_ = 0;
        if (custom_layout_streak_ >= kProfileDetectFrames) {
            detected_profile_ = InputProfile::CustomControllerChannelPack;
            profile_locked_ = true;
        }
    } else {
        if (tx_direct_layout_streak_ < kProfileDetectFrames) ++tx_direct_layout_streak_;
        custom_layout_streak_ = 0;
        if (tx_direct_layout_streak_ >= kProfileDetectFrames) {
            detected_profile_ = InputProfile::Tx16sMk3Direct;
            profile_locked_ = true;
        }
    }
}

void ControllerBridge::unpackTx16sMk3DirectChannels(
    const uint16_t ch[CPACK_NUM_CHANNELS], ChannelPackInputs_t* out)
{
    clearRawInputs(out);

    out->gimbal[0] = crsfToGimbalSafe(ch[0]);
    out->gimbal[1] = crsfToGimbalSafe(ch[1]);
    out->gimbal[2] = crsfToGimbalSafe(ch[2]);
    out->gimbal[3] = crsfToGimbalSafe(ch[3]);

    out->pot[0] = crsfToPotSafe(ch[4]);
    out->pot[1] = crsfToPotSafe(ch[5]);

    out->toggles[0] = crsfToTri(ch[8]);
    out->toggles[1] = crsfToTri(ch[9]);

    const uint8_t safety_mask = crsfToBins(ch[10], 4);
    out->switches[0] = (safety_mask & 0x01) != 0;
    out->switches[1] = (safety_mask & 0x02) != 0;

    const uint8_t feature_mask = crsfToBins(ch[11], 16);
    out->switches[2] = (feature_mask & 0x01) != 0;
    out->switches[3] = (feature_mask & 0x02) != 0;
    out->switches[4] = (feature_mask & 0x04) != 0;
    out->switches[5] = (feature_mask & 0x08) != 0;

    const uint8_t action = crsfToBins(ch[12], 13);
    const bool fire = crsfToBoolHigh(ch[13]);
    if (fire) {
        switch (action) {
        case 0:  out->nav[0][CPACK_NAV_UP] = true; break;
        case 1:  out->nav[0][CPACK_NAV_DOWN] = true; break;
        case 2:  out->nav[0][CPACK_NAV_LEFT] = true; break;
        case 3:  out->nav[0][CPACK_NAV_RIGHT] = true; break;
        case 4:  out->nav[0][CPACK_NAV_CENTER] = true; break;
        case 5:  out->buttons[0] = true; break;
        case 6:  out->buttons[1] = true; break;
        case 7:  out->buttons[2] = true; break;
        case 8:  out->buttons[3] = true; break;
        case 9:  out->nav[1][CPACK_NAV_UP] = true; break;
        case 10: out->nav[1][CPACK_NAV_DOWN] = true; break;
        case 11: out->nav[1][CPACK_NAV_LEFT] = true; break;
        case 12: out->nav[1][CPACK_NAV_CENTER] = true; break;
        default: break;
        }
    }
}

void ControllerBridge::updateTx16sDirectVirtualEncoders(
    const uint16_t ch[CPACK_NUM_CHANNELS])
{
    const float enc0 = crsfUnit01(ch[6]);
    const float enc1 = crsfUnit01(ch[7]);

    raw_.encoder[0] = int32_t(enc0 * 2047.0f);
    raw_.encoder[1] = int32_t(enc1 * 2047.0f);

    enc_accum_[0] = clampfLocal(enc0, 0.0f, 1.0f);
    enc_accum_[1] = clampfLocal(enc1, 0.0f, 1.0f);

    enc_seen_[0] = true;
    enc_seen_[1] = true;
    enc_last_[0] = raw_.encoder[0];
    enc_last_[1] = raw_.encoder[1];
}

void ControllerBridge::integrateEncoders()
{
    for (int i = 0; i < 2; ++i) {
        const int32_t current = raw_.encoder[i] & 0x7FF;
        if (!enc_seen_[i]) {
            enc_seen_[i] = true;
            enc_last_[i] = current;
            continue;
        }

        int32_t delta = current - enc_last_[i];
        if (delta > 1024) delta -= 2048;
        if (delta < -1024) delta += 2048;
        enc_accum_[i] = clampfLocal(enc_accum_[i] + (float(delta) / 2047.0f),
                                    0.0f, 1.0f);
        enc_last_[i] = current;
    }
}

void ControllerBridge::enterFailsafe(uint32_t now_ms)
{
    (void)now_ms;
    clearCommand(&cmd_);
}

float ControllerBridge::readAxisBipolar(AxisSource source) const
{
    switch (source) {
    case AxisSource::GimbalLX: return clampfLocal(float(raw_.gimbal[0]) / 1000.0f, -1.0f, 1.0f);
    case AxisSource::GimbalLY: return clampfLocal(float(raw_.gimbal[1]) / 1000.0f, -1.0f, 1.0f);
    case AxisSource::GimbalRX: return clampfLocal(float(raw_.gimbal[2]) / 1000.0f, -1.0f, 1.0f);
    case AxisSource::GimbalRY: return clampfLocal(float(raw_.gimbal[3]) / 1000.0f, -1.0f, 1.0f);
    case AxisSource::Pot1: return clampfLocal((float(raw_.pot[0]) / 500.0f) - 1.0f, -1.0f, 1.0f);
    case AxisSource::Pot2: return clampfLocal((float(raw_.pot[1]) / 500.0f) - 1.0f, -1.0f, 1.0f);
    case AxisSource::Enc1: return clampfLocal((enc_accum_[0] * 2.0f) - 1.0f, -1.0f, 1.0f);
    case AxisSource::Enc2: return clampfLocal((enc_accum_[1] * 2.0f) - 1.0f, -1.0f, 1.0f);
    case AxisSource::None:
    default: return 0.0f;
    }
}

float ControllerBridge::readAxisUnipolar(AxisSource source) const
{
    switch (source) {
    case AxisSource::GimbalLX: return clampfLocal((float(raw_.gimbal[0]) + 1000.0f) / 2000.0f, 0.0f, 1.0f);
    case AxisSource::GimbalLY: return clampfLocal((float(raw_.gimbal[1]) + 1000.0f) / 2000.0f, 0.0f, 1.0f);
    case AxisSource::GimbalRX: return clampfLocal((float(raw_.gimbal[2]) + 1000.0f) / 2000.0f, 0.0f, 1.0f);
    case AxisSource::GimbalRY: return clampfLocal((float(raw_.gimbal[3]) + 1000.0f) / 2000.0f, 0.0f, 1.0f);
    case AxisSource::Pot1: return clampfLocal(float(raw_.pot[0]) / 1000.0f, 0.0f, 1.0f);
    case AxisSource::Pot2: return clampfLocal(float(raw_.pot[1]) / 1000.0f, 0.0f, 1.0f);
    case AxisSource::Enc1: return clampfLocal(enc_accum_[0], 0.0f, 1.0f);
    case AxisSource::Enc2: return clampfLocal(enc_accum_[1], 0.0f, 1.0f);
    case AxisSource::None:
    default: return 0.0f;
    }
}

bool ControllerBridge::readBool(BoolSource source) const
{
    switch (source) {
    case BoolSource::SwA: return raw_.switches[0];
    case BoolSource::SwB: return raw_.switches[1];
    case BoolSource::SwC: return raw_.switches[2];
    case BoolSource::SwD: return raw_.switches[3];
    case BoolSource::SwG: return raw_.switches[4];
    case BoolSource::SwH: return raw_.switches[5];
    case BoolSource::Btn1: return raw_.buttons[0];
    case BoolSource::Btn2: return raw_.buttons[1];
    case BoolSource::Btn3: return raw_.buttons[2];
    case BoolSource::Btn4: return raw_.buttons[3];
    case BoolSource::Nav1Up: return raw_.nav[0][CPACK_NAV_UP];
    case BoolSource::Nav1Down: return raw_.nav[0][CPACK_NAV_DOWN];
    case BoolSource::Nav1Left: return raw_.nav[0][CPACK_NAV_LEFT];
    case BoolSource::Nav1Right: return raw_.nav[0][CPACK_NAV_RIGHT];
    case BoolSource::Nav1Center: return raw_.nav[0][CPACK_NAV_CENTER];
    case BoolSource::Nav2Up: return raw_.nav[1][CPACK_NAV_UP];
    case BoolSource::Nav2Down: return raw_.nav[1][CPACK_NAV_DOWN];
    case BoolSource::Nav2Left: return raw_.nav[1][CPACK_NAV_LEFT];
    case BoolSource::Nav2Right: return raw_.nav[1][CPACK_NAV_RIGHT];
    case BoolSource::Nav2Center: return raw_.nav[1][CPACK_NAV_CENTER];
    case BoolSource::None:
    default: return false;
    }
}

uint8_t ControllerBridge::readTri(TriSource source) const
{
    switch (source) {
    case TriSource::SwE: return raw_.toggles[0] <= 2 ? raw_.toggles[0] : 1;
    case TriSource::SwF: return raw_.toggles[1] <= 2 ? raw_.toggles[1] : 1;
    case TriSource::None:
    default: return 1;
    }
}

bool ControllerBridge::risingEdge(BoolSource source)
{
    const bool now = readBool(source);
    bool prev = false;
    switch (source) {
    case BoolSource::SwA: prev = prev_raw_.switches[0]; break;
    case BoolSource::SwB: prev = prev_raw_.switches[1]; break;
    case BoolSource::SwC: prev = prev_raw_.switches[2]; break;
    case BoolSource::SwD: prev = prev_raw_.switches[3]; break;
    case BoolSource::SwG: prev = prev_raw_.switches[4]; break;
    case BoolSource::SwH: prev = prev_raw_.switches[5]; break;
    case BoolSource::Btn1: prev = prev_raw_.buttons[0]; break;
    case BoolSource::Btn2: prev = prev_raw_.buttons[1]; break;
    case BoolSource::Btn3: prev = prev_raw_.buttons[2]; break;
    case BoolSource::Btn4: prev = prev_raw_.buttons[3]; break;
    case BoolSource::Nav1Up: prev = prev_raw_.nav[0][CPACK_NAV_UP]; break;
    case BoolSource::Nav1Down: prev = prev_raw_.nav[0][CPACK_NAV_DOWN]; break;
    case BoolSource::Nav1Left: prev = prev_raw_.nav[0][CPACK_NAV_LEFT]; break;
    case BoolSource::Nav1Right: prev = prev_raw_.nav[0][CPACK_NAV_RIGHT]; break;
    case BoolSource::Nav1Center: prev = prev_raw_.nav[0][CPACK_NAV_CENTER]; break;
    case BoolSource::Nav2Up: prev = prev_raw_.nav[1][CPACK_NAV_UP]; break;
    case BoolSource::Nav2Down: prev = prev_raw_.nav[1][CPACK_NAV_DOWN]; break;
    case BoolSource::Nav2Left: prev = prev_raw_.nav[1][CPACK_NAV_LEFT]; break;
    case BoolSource::Nav2Right: prev = prev_raw_.nav[1][CPACK_NAV_RIGHT]; break;
    case BoolSource::Nav2Center: prev = prev_raw_.nav[1][CPACK_NAV_CENTER]; break;
    case BoolSource::None:
    default: return false;
    }
    return now && !prev;
}

void ControllerBridge::buildCommand(uint32_t now_ms)
{
    (void)now_ms;
    cmd_.failsafe = false;
    cmd_.estop = readBool(bindings_.estop);
    cmd_.arm_request = readBool(bindings_.arm) && !cmd_.estop;

    cmd_.walk_x = readAxisBipolar(bindings_.walk_x);
    cmd_.walk_y = readAxisBipolar(bindings_.walk_y);
    cmd_.body_x = readAxisBipolar(bindings_.body_x);
    cmd_.body_y = readAxisBipolar(bindings_.body_y);
    cmd_.speed = readAxisUnipolar(bindings_.speed);
    cmd_.body_height = readAxisUnipolar(bindings_.body_height);
    cmd_.stride = readAxisUnipolar(bindings_.stride);
    cmd_.step_height = readAxisUnipolar(bindings_.step_height);

    const uint8_t mode = readTri(bindings_.mode);
    cmd_.mode = mode == 0 ? ControlMode::Walk :
                mode == 1 ? ControlMode::TranslateBody :
                            ControlMode::RotateBody;
    cmd_.gait = readTri(bindings_.gait);

    cmd_.feat_foot_contact = readBool(bindings_.feat_foot_contact);
    cmd_.feat_terrain_leveling = readBool(bindings_.feat_terrain_leveling);
    cmd_.feat_passive_pose = readBool(bindings_.feat_passive_pose);
    cmd_.host_authority = readBool(bindings_.host_authority);

    cmd_.trim_pitch = 0;
    if (risingEdge(bindings_.trim_pitch_up)) cmd_.trim_pitch += 1;
    if (risingEdge(bindings_.trim_pitch_down)) cmd_.trim_pitch -= 1;
    cmd_.trim_roll = 0;
    if (risingEdge(bindings_.trim_roll_right)) cmd_.trim_roll += 1;
    if (risingEdge(bindings_.trim_roll_left)) cmd_.trim_roll -= 1;
    cmd_.trim_reset = risingEdge(bindings_.trim_reset);

    cmd_.trick = TrickId::None;
    if (risingEdge(bindings_.trick_stand_up)) cmd_.trick = TrickId::StandUp;
    else if (risingEdge(bindings_.trick_sit_down)) cmd_.trick = TrickId::SitDown;
    else if (risingEdge(bindings_.trick_wave)) cmd_.trick = TrickId::Wave;
    else if (risingEdge(bindings_.trick_crouch_toggle)) cmd_.trick = TrickId::CrouchToggle;
    else if (risingEdge(bindings_.trick_twirl)) cmd_.trick = TrickId::Twirl;
    else if (risingEdge(bindings_.trick_stretch)) cmd_.trick = TrickId::Stretch;
    else if (risingEdge(bindings_.trick_lean_look)) cmd_.trick = TrickId::LeanLook;
    else if (risingEdge(bindings_.trick_dance_loop)) cmd_.trick = TrickId::DanceLoop;
}
