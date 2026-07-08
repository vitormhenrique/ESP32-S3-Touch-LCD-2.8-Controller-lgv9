#pragma once

#include <stdint.h>
#include "ChannelPack.h"

enum class InputProfile : uint8_t {
    Unknown = 0,
    CustomControllerChannelPack,
    Tx16sMk3Direct,
};

enum class AxisSource : uint8_t {
    None = 0,
    GimbalLX,
    GimbalLY,
    GimbalRX,
    GimbalRY,
    Pot1,
    Pot2,
    Enc1,
    Enc2,
};

enum class BoolSource : uint8_t {
    None = 0,
    SwA,
    SwB,
    SwC,
    SwD,
    SwG,
    SwH,
    Btn1,
    Btn2,
    Btn3,
    Btn4,
    Nav1Up,
    Nav1Down,
    Nav1Left,
    Nav1Right,
    Nav1Center,
    Nav2Up,
    Nav2Down,
    Nav2Left,
    Nav2Right,
    Nav2Center,
};

enum class TriSource : uint8_t {
    None = 0,
    SwE,
    SwF,
};

enum class TrickId : uint8_t {
    None = 0,
    StandUp,
    SitDown,
    Wave,
    CrouchToggle,
    Twirl,
    Stretch,
    LeanLook,
    DanceLoop,
};

enum class ControlMode : uint8_t {
    Walk = 0,
    TranslateBody,
    RotateBody,
};

struct BindingConfig {
    AxisSource walk_x;
    AxisSource walk_y;
    AxisSource body_x;
    AxisSource body_y;
    AxisSource speed;
    AxisSource body_height;
    AxisSource stride;
    AxisSource step_height;

    TriSource mode;
    TriSource gait;

    BoolSource arm;
    BoolSource estop;
    BoolSource feat_foot_contact;
    BoolSource feat_terrain_leveling;
    BoolSource feat_passive_pose;
    BoolSource host_authority;

    BoolSource trim_pitch_up;
    BoolSource trim_pitch_down;
    BoolSource trim_roll_left;
    BoolSource trim_roll_right;
    BoolSource trim_reset;

    BoolSource trick_stand_up;
    BoolSource trick_sit_down;
    BoolSource trick_wave;
    BoolSource trick_crouch_toggle;
    BoolSource trick_twirl;
    BoolSource trick_stretch;
    BoolSource trick_lean_look;
    BoolSource trick_dance_loop;
};

BindingConfig defaultBindings();

struct ControllerCommand {
    bool failsafe;
    bool arm_request;
    bool estop;

    float walk_x;
    float walk_y;
    float body_x;
    float body_y;
    float speed;
    float body_height;
    float stride;
    float step_height;

    ControlMode mode;
    uint8_t gait;

    bool feat_foot_contact;
    bool feat_terrain_leveling;
    bool feat_passive_pose;
    bool host_authority;

    int8_t trim_pitch;
    int8_t trim_roll;
    bool trim_reset;

    TrickId trick;
};

class ControllerBridge {
public:
    explicit ControllerBridge(const BindingConfig& bindings = defaultBindings());

    void reset();
    const ControllerCommand& update(const uint16_t ch[CPACK_NUM_CHANNELS],
                                    bool link_up,
                                    uint32_t now_ms);

    InputProfile detectedProfile() const { return detected_profile_; }
    bool profileLocked() const { return profile_locked_; }
    const ChannelPackInputs_t& rawInputs() const { return raw_; }
    const ControllerCommand& command() const { return cmd_; }
    float encoderAccum(uint8_t index) const;

private:
    static constexpr uint8_t kProfileDetectFrames = 3;

    void attemptProfileDetection(const uint16_t ch[CPACK_NUM_CHANNELS]);
    void unpackTx16sMk3DirectChannels(const uint16_t ch[CPACK_NUM_CHANNELS],
                                      ChannelPackInputs_t* out);
    void updateTx16sDirectVirtualEncoders(const uint16_t ch[CPACK_NUM_CHANNELS]);
    void integrateEncoders();
    void enterFailsafe(uint32_t now_ms);
    void buildCommand(uint32_t now_ms);

    float readAxisBipolar(AxisSource source) const;
    float readAxisUnipolar(AxisSource source) const;
    bool readBool(BoolSource source) const;
    uint8_t readTri(TriSource source) const;
    bool risingEdge(BoolSource source);

    BindingConfig bindings_;
    ChannelPackInputs_t raw_;
    ChannelPackInputs_t prev_raw_;
    ControllerCommand cmd_;

    float enc_accum_[2];
    int32_t enc_last_[2];
    bool enc_seen_[2];

    InputProfile detected_profile_;
    bool profile_locked_;
    uint8_t custom_layout_streak_;
    uint8_t tx_direct_layout_streak_;
};
