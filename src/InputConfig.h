#pragma once
#include <Arduino.h>

/******************************************************************************
 * RC Remote Input Configuration
 * 
 * Hardware:
 * - 2x MCP23017 GPIO Expanders (16 pins each = 32 digital inputs)
 * - 2x ADS1115 ADC (4 channels each = 8 analog inputs)
 * 
 * Inputs:
 * - 2x Gimbals (4 axes total: 2 per gimbal)
 * - 2x Potentiometers
 * - 2x Navigation Switches (5 buttons each: Up, Down, Left, Right, Center)
 * - 2x Rotary Encoders (A/B pins)
 * - 2x Buttons
 ******************************************************************************/

//=============================================================================
// I2C Addresses
//=============================================================================

// MCP23017 addresses (A0, A1, A2 pins determine address 0x20-0x27)
#define MCP23017_ADDR_1     0x20    // First expander (Navigation 2, Encoder 2, Button 2)
#define MCP23017_ADDR_2     0x21    // Second expander (Navigation 1, Encoder 1, Button 1)

// ADS1X15 addresses (ADDR pin: GND=0x48, VDD=0x49, SDA=0x4A, SCL=0x4B)
#define ADS1X15_ADDR_1      0x48    // Gimbal 1 (X/Y)
#define ADS1X15_ADDR_2      0x49    // Gimbal 2 (X/Y)
#define ADS1X15_ADDR_3      0x4A    // Potentiometers (optional)

//=============================================================================
// Enums
//=============================================================================

/* Switch state for standard 2-position switches */
typedef enum {
    SWITCH_OFF = 0,
    SWITCH_ON = 1
} SwitchState_t;

/* State for 3-position toggle switches */
typedef enum {
    TOGGLE_POS_UP = 0,      // Position 1 (up)
    TOGGLE_POS_CENTER = 1,  // Position 2 (center/neutral)
    TOGGLE_POS_DOWN = 2     // Position 3 (down)
} Toggle3PosState_t;

/* Switch types */
typedef enum {
    SWITCH_TYPE_MOMENTARY,      // Push button (returns when released)
    SWITCH_TYPE_TOGGLE_2POS,    // 2-position toggle (latching)
    SWITCH_TYPE_TOGGLE_3POS     // 3-position toggle (uses 2 pins)
} SwitchType_t;

/* Navigation switch directions */
typedef enum {
    NAV_UP = 0,
    NAV_DOWN,
    NAV_LEFT,
    NAV_RIGHT,
    NAV_CENTER,
    NAV_DIR_COUNT
} NavDirection_t;

//=============================================================================
// Structs
//=============================================================================

/* Configuration for a single switch input */
typedef struct {
    const char* name;           // Human-readable name (e.g., "ARM", "MODE")
    uint8_t expander;           // MCP23017 index (0 or 1)
    uint8_t pin;                // Pin on expander (0-15)
    SwitchType_t type;          // Switch type
    bool inverted;              // True if logic is inverted (pull-up)
} SwitchConfig_t;

/* Configuration for a 3-position toggle (uses 2 pins) */
typedef struct {
    const char* name;           // Human-readable name
    uint8_t expander;           // MCP23017 index (0 or 1)
    uint8_t pin_up;             // Pin for UP position
    uint8_t pin_down;           // Pin for DOWN position
    bool inverted;              // True if logic is inverted (pull-up)
} Toggle3PosConfig_t;

/* Configuration for a navigation switch (5-way: up, down, left, right, center) */
typedef struct {
    const char* name;           // Human-readable name (e.g., "NAV_1")
    uint8_t expander;           // MCP23017 index (0 or 1)
    uint8_t pin_up;             // Pin for UP direction
    uint8_t pin_down;           // Pin for DOWN direction
    uint8_t pin_left;           // Pin for LEFT direction
    uint8_t pin_right;          // Pin for RIGHT direction
    uint8_t pin_center;         // Pin for CENTER button
    bool inverted;              // True if logic is inverted (pull-up)
} NavSwitchConfig_t;

/* Configuration for a rotary encoder */
typedef struct {
    const char* name;           // Human-readable name (e.g., "ENC_1")
    uint8_t expander;           // MCP23017 index (0 or 1)
    uint8_t pin_a;              // Pin for encoder A signal
    uint8_t pin_b;              // Pin for encoder B signal
    bool inverted;              // True to reverse direction
} EncoderConfig_t;

/* Configuration for an analog axis (gimbal or pot) */
typedef struct {
    const char* name;           // Human-readable name (e.g., "THROTTLE", "ROLL")
    uint8_t adc;                // ADS1X15 index (0, 1, or 2)
    uint8_t channel;            // ADC channel (0-3)
    int16_t min_raw;            // Raw ADC value at minimum position
    int16_t max_raw;            // Raw ADC value at maximum position
    int16_t center_raw;         // Raw ADC value at center (for gimbals)
    int16_t deadzone;           // Deadzone around center (for gimbals)
    bool inverted;              // True to invert axis direction
} AnalogAxisConfig_t;

/* Runtime state for a switch */
typedef struct {
    SwitchState_t state;        // Current state
    SwitchState_t prev_state;   // Previous state (for edge detection)
    uint32_t last_change_ms;    // Timestamp of last state change
} SwitchState_Runtime_t;

/* Runtime state for a 3-position toggle */
typedef struct {
    Toggle3PosState_t state;        // Current position
    Toggle3PosState_t prev_state;   // Previous position
    uint32_t last_change_ms;        // Timestamp of last position change
} Toggle3PosState_Runtime_t;

/* Runtime state for navigation switch */
typedef struct {
    bool directions[NAV_DIR_COUNT];      // Current state of each direction
    bool prev_directions[NAV_DIR_COUNT]; // Previous state for edge detection
    uint32_t last_change_ms;             // Timestamp of last state change
} NavSwitchState_Runtime_t;

/* Runtime state for encoder */
typedef struct {
    int32_t position;           // Current encoder position
    int32_t prev_position;      // Previous position (for change detection)
    bool last_a;                // Last state of A pin
    bool last_b;                // Last state of B pin
} EncoderState_Runtime_t;

/* Runtime state for an analog axis */
typedef struct {
    int16_t raw;                // Raw ADC reading
    int16_t calibrated;         // Calibrated value (-1000 to +1000 for gimbals, 0-1000 for pots)
    int16_t filtered;           // Filtered/smoothed value
} AnalogAxisState_t;

//=============================================================================
// Input Definitions - HARDWARE CONFIGURATION
//=============================================================================

// Number of each input type
#define NUM_SWITCHES        26      // 12 original + 10 Nav switches (2x5) + 4 Encoder pins
#define NUM_3POS_TOGGLES    2       // Two 3-position toggle switches
#define NUM_GIMBAL_AXES     4       // 2 gimbals x 2 axes each
#define NUM_POTENTIOMETERS  2       // 2 potentiometers

// Total analog axes
#define NUM_ANALOG_AXES     (NUM_GIMBAL_AXES + NUM_POTENTIOMETERS)

//=============================================================================
// MCP23017 Pin Mapping
// Expander 0 (0x20): pins 0-15 (GPA0-7 = 0-7, GPB0-7 = 8-15)
// Expander 1 (0x21): pins 0-15 (GPA0-7 = 0-7, GPB0-7 = 8-15)
// Note: Port A pins = 0-7, Port B pins = 8-15
//=============================================================================

// Navigation Switch 2: MCP23017 address 0x20 (expander 0)
// Left=A5(5), Down=A6(6), Right=A3(3), Up=A4(4), Center=A0(0)
#define NAV2_PIN_UP      4   // A4
#define NAV2_PIN_DOWN    6   // A6
#define NAV2_PIN_LEFT    5   // A5
#define NAV2_PIN_RIGHT   3   // A3
#define NAV2_PIN_CENTER  0   // A0

// Encoder 2: MCP23017 address 0x20 (expander 0)
// A=A1(1), B=A2(2)
#define ENC2_PIN_A       1   // A1
#define ENC2_PIN_B       2   // A2

// Button 2: MCP23017 address 0x20 (expander 0)
#define BTN2_PIN         7   // A7

// Navigation Switch 1: MCP23017 address 0x21 (expander 1)
// Left=B5(13), Down=B6(14), Right=B3(11), Up=B4(12), Center=B0(8)
#define NAV1_PIN_UP      12  // B4
#define NAV1_PIN_DOWN    14  // B6
#define NAV1_PIN_LEFT    13  // B5
#define NAV1_PIN_RIGHT   11  // B3
#define NAV1_PIN_CENTER  8   // B0

// Encoder 1: MCP23017 address 0x21 (expander 1)
// A=B1(9), B=B2(10)
#define ENC1_PIN_A       9   // B1
#define ENC1_PIN_B       10  // B2

// Button 1: MCP23017 address 0x21 (expander 1)
#define BTN1_PIN         15  // B7

//=============================================================================
// Switch Pin Assignments (MCP23017) - Standalone Buttons
//=============================================================================

// Format: { "NAME", expander, pin, type, inverted }
#define SWITCH_CONFIGS { \
    /* Switches assigned to Expander 0 Port B (Pins 8-15) */ \
    { "SW_A",    0,  8, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_B",    0,  9, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_C",    0, 10, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_D",    0, 11, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_E",    0, 12, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_F",    0, 13, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_G",    0, 14, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_H",    0, 15, SWITCH_TYPE_TOGGLE_2POS, true }, \
    /* Buttons */ \
    { "BTN_1",   1, 15, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 B7 */ \
    { "BTN_2",   0,  7, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 0 A7 */ \
    { "BTN_3",   1,  4, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 A4 */ \
    { "BTN_4",   1,  5, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 A5 */ \
    /* Nav Switch 1 (Left) - Expander 1 Port B */ \
    { "NAV1_U",  1, 12, SWITCH_TYPE_MOMENTARY,   true }, /* B4 */ \
    { "NAV1_D",  1, 14, SWITCH_TYPE_MOMENTARY,   true }, /* B6 */ \
    { "NAV1_L",  1, 13, SWITCH_TYPE_MOMENTARY,   true }, /* B5 */ \
    { "NAV1_R",  1, 11, SWITCH_TYPE_MOMENTARY,   true }, /* B3 */ \
    { "NAV1_C",  1,  8, SWITCH_TYPE_MOMENTARY,   true }, /* B0 */ \
    /* Nav Switch 2 (Right) - Expander 0 Port A */ \
    { "NAV2_U",  0,  4, SWITCH_TYPE_MOMENTARY,   true }, /* A4 */ \
    { "NAV2_D",  0,  6, SWITCH_TYPE_MOMENTARY,   true }, /* A6 */ \
    { "NAV2_L",  0,  5, SWITCH_TYPE_MOMENTARY,   true }, /* A5 */ \
    { "NAV2_R",  0,  3, SWITCH_TYPE_MOMENTARY,   true }, /* A3 */ \
    { "NAV2_C",  0,  0, SWITCH_TYPE_MOMENTARY,   true }, /* A0 */ \
    /* Encoders (Treated as switches for raw input) */ \
    { "ENC1_A",  1,  9, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 B1 */ \
    { "ENC1_B",  1, 10, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 B2 */ \
    { "ENC2_A",  0,  1, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 0 A1 */ \
    { "ENC2_B",  0,  2, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 0 A2 */ \
}

// 3-position toggle switches configuration
// Format: { "NAME", expander, pin_up, pin_down, inverted }
// CENTER is detected when neither UP nor DOWN pin is active
// Assigned to Expander 1 Port A 0-3
#define TOGGLE_3POS_CONFIGS { \
    { "SW_3POS_1", 1, 0, 1, true }, \
    { "SW_3POS_2", 1, 2, 3, true }, \
}

//=============================================================================
// Encoder Configurations
//=============================================================================

// Format: { "NAME", expander, pin_a, pin_b, inverted }
#define ENCODER_CONFIGS { \
    { "ENC_1", 1, ENC1_PIN_A, ENC1_PIN_B, false }, \
    { "ENC_2", 0, ENC2_PIN_A, ENC2_PIN_B, false }, \
}

//=============================================================================
// 3-Position Toggle Configurations (empty - using nav switches)
//=============================================================================

#define TOGGLE_3POS_CONFIGS { }

//=============================================================================
// Analog Input Assignments (ADS1X15)
// ADC 0 (0x48): Gimbal 1 X/Y
// ADC 1 (0x49): Gimbal 2 X/Y
// ADC 2 (0x4A): Potentiometers (optional)
//=============================================================================

// Gimbal axes configuration
// Gimbal 1 X: ADS1115 0x48 input A0 (adc 0, channel 0)
// Gimbal 1 Y: ADS1115 0x48 input A1 (adc 0, channel 1)
// Gimbal 2 X: ADS1115 0x49 input A0 (adc 1, channel 0)
// Gimbal 2 Y: ADS1115 0x49 input A1 (adc 1, channel 1)
// Format: { "NAME", adc, channel, min_raw, max_raw, center_raw, deadzone, inverted }
#define GIMBAL_CONFIGS { \
    { "LEFT_X",   0, 0,  0, 32767, 16383, 200, false }, /* ADC 0 (0x48) Ch 0 */ \
    { "LEFT_Y",   0, 1,  0, 32767, 16383, 200, false }, /* ADC 0 (0x48) Ch 1 */ \
    { "RIGHT_X",  1, 0,  0, 32767, 16383, 200, false }, /* ADC 1 (0x49) Ch 0 */ \
    { "RIGHT_Y",  1, 1,  0, 32767, 16383, 200, false }, /* ADC 1 (0x49) Ch 1 */ \
}

// Potentiometer configurations (on spare ADC channels)
// Format: { "NAME", adc, channel, min_raw, max_raw, center_raw, deadzone, inverted }
#define POT_CONFIGS { \
    { "POT_1",    1, 2,  0, 32767, 0, 0, false }, /* ADC 1 (0x49) Ch 2 */ \
    { "POT_2",    1, 3,  0, 32767, 0, 0, false }, /* ADC 1 (0x49) Ch 3 */ \
}

//=============================================================================
// Calibration Constants
//=============================================================================

#define ANALOG_OUTPUT_MIN       -1000   // Output range minimum (for gimbals)
#define ANALOG_OUTPUT_MAX        1000   // Output range maximum
#define ANALOG_OUTPUT_CENTER        0   // Output center value

#define POT_OUTPUT_MIN              0   // Potentiometer output minimum
#define POT_OUTPUT_MAX           1000   // Potentiometer output maximum

#define DEBOUNCE_MS                20   // Switch debounce time in milliseconds
#define ANALOG_FILTER_SAMPLES       4   // Number of samples for moving average

