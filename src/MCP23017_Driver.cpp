#include "MCP23017_Driver.h"
#include "I2C_Driver.h"

// Global instance
MCP23017_Driver SwitchInput;

// Static configuration arrays
#if NUM_SWITCHES > 0
static const SwitchConfig_t _defaultSwitchConfigs[NUM_SWITCHES] = SWITCH_CONFIGS;
#endif

#if NUM_NAV_SWITCHES > 0
static const NavSwitchConfig_t _defaultNavSwitchConfigs[NUM_NAV_SWITCHES] = NAV_SWITCH_CONFIGS;
#endif

#if NUM_ENCODERS > 0
static const EncoderConfig_t _defaultEncoderConfigs[NUM_ENCODERS] = ENCODER_CONFIGS;
#endif

MCP23017_Driver::MCP23017_Driver() {
    _initialized[0] = false;
    _initialized[1] = false;
    _lastUpdateMs = 0;
}

void MCP23017_Driver::initConfigs() {
    // Copy switch configurations
#if NUM_SWITCHES > 0
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        _switchConfigs[i] = _defaultSwitchConfigs[i];
        _switchStates[i].state = SWITCH_OFF;
        _switchStates[i].prev_state = SWITCH_OFF;
        _switchStates[i].last_change_ms = 0;
    }
#endif
    
    // Copy navigation switch configurations
#if NUM_NAV_SWITCHES > 0
    for (uint8_t i = 0; i < NUM_NAV_SWITCHES; i++) {
        _navSwitchConfigs[i] = _defaultNavSwitchConfigs[i];
        for (uint8_t d = 0; d < NAV_DIR_COUNT; d++) {
            _navSwitchStates[i].directions[d] = false;
            _navSwitchStates[i].prev_directions[d] = false;
        }
        _navSwitchStates[i].last_change_ms = 0;
    }
#endif
    
    // Copy encoder configurations
#if NUM_ENCODERS > 0
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        _encoderConfigs[i] = _defaultEncoderConfigs[i];
        _encoderStates[i].position = 0;
        _encoderStates[i].prev_position = 0;
        _encoderStates[i].last_a = false;
        _encoderStates[i].last_b = false;
    }
#endif
}

bool MCP23017_Driver::begin() {
    initConfigs();
    
    // Initialize first MCP23017
    if (_mcp[0].begin_I2C(MCP23017_ADDR_1, &Wire)) {
        _initialized[0] = true;
        printf("MCP23017 #1 (0x%02X) initialized\r\n", MCP23017_ADDR_1);
        
        // Configure all pins as inputs with pull-ups
        for (uint8_t pin = 0; pin < 16; pin++) {
            _mcp[0].pinMode(pin, INPUT_PULLUP);
        }
    } else {
        printf("MCP23017 #1 (0x%02X) initialization FAILED\r\n", MCP23017_ADDR_1);
    }
    
    // Initialize second MCP23017
    if (_mcp[1].begin_I2C(MCP23017_ADDR_2, &Wire)) {
        _initialized[1] = true;
        printf("MCP23017 #2 (0x%02X) initialized\r\n", MCP23017_ADDR_2);
        
        // Configure all pins as inputs with pull-ups
        for (uint8_t pin = 0; pin < 16; pin++) {
            _mcp[1].pinMode(pin, INPUT_PULLUP);
        }
    } else {
        printf("MCP23017 #2 (0x%02X) initialization FAILED\r\n", MCP23017_ADDR_2);
    }
    
    // Initialize encoder states by reading initial pin states
#if NUM_ENCODERS > 0
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        if (_initialized[_encoderConfigs[i].expander]) {
            _encoderStates[i].last_a = _mcp[_encoderConfigs[i].expander].digitalRead(_encoderConfigs[i].pin_a);
            _encoderStates[i].last_b = _mcp[_encoderConfigs[i].expander].digitalRead(_encoderConfigs[i].pin_b);
            if (_encoderConfigs[i].inverted) {
                _encoderStates[i].last_a = !_encoderStates[i].last_a;
                _encoderStates[i].last_b = !_encoderStates[i].last_b;
            }
        }
    }
#endif
    
    return _initialized[0] && _initialized[1];
}

void MCP23017_Driver::update() {
    uint32_t now = millis();
    
    // Update all standard switches
#if NUM_SWITCHES > 0
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        updateSwitch(i);
    }
#endif
    
    // Update all navigation switches
#if NUM_NAV_SWITCHES > 0
    for (uint8_t i = 0; i < NUM_NAV_SWITCHES; i++) {
        updateNavSwitch(i);
    }
#endif
    
    // Update all encoders
#if NUM_ENCODERS > 0
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        updateEncoder(i);
    }
#endif
    
    _lastUpdateMs = now;
}

void MCP23017_Driver::updateSwitch(uint8_t index) {
#if NUM_SWITCHES > 0
    if (index >= NUM_SWITCHES) return;
    
    SwitchConfig_t* cfg = &_switchConfigs[index];
    SwitchState_Runtime_t* state = &_switchStates[index];
    
    // Check if expander is initialized
    if (!_initialized[cfg->expander]) return;
    
    // Read pin state
    bool pinState = _mcp[cfg->expander].digitalRead(cfg->pin);
    
    // Apply inversion if needed
    if (cfg->inverted) {
        pinState = !pinState;
    }
    
    SwitchState_t newState = pinState ? SWITCH_ON : SWITCH_OFF;
    uint32_t now = millis();
    
    // Debounce: only accept change if enough time has passed
    if (newState != state->state) {
        if ((now - state->last_change_ms) >= DEBOUNCE_MS) {
            state->prev_state = state->state;
            state->state = newState;
            state->last_change_ms = now;
        }
    }
#endif
}

void MCP23017_Driver::updateNavSwitch(uint8_t index) {
#if NUM_NAV_SWITCHES > 0
    if (index >= NUM_NAV_SWITCHES) return;
    
    NavSwitchConfig_t* cfg = &_navSwitchConfigs[index];
    NavSwitchState_Runtime_t* state = &_navSwitchStates[index];
    
    // Check if expander is initialized
    if (!_initialized[cfg->expander]) return;
    
    uint32_t now = millis();
    
    // Read all direction pins
    uint8_t pins[NAV_DIR_COUNT] = {
        cfg->pin_up, cfg->pin_down, cfg->pin_left, cfg->pin_right, cfg->pin_center
    };
    
    for (uint8_t d = 0; d < NAV_DIR_COUNT; d++) {
        bool pinState = _mcp[cfg->expander].digitalRead(pins[d]);
        
        // Apply inversion if needed
        if (cfg->inverted) {
            pinState = !pinState;
        }
        
        // Debounce: only accept change if enough time has passed
        if (pinState != state->directions[d]) {
            if ((now - state->last_change_ms) >= DEBOUNCE_MS) {
                state->prev_directions[d] = state->directions[d];
                state->directions[d] = pinState;
                state->last_change_ms = now;
            }
        }
    }
#endif
}

void MCP23017_Driver::updateEncoder(uint8_t index) {
#if NUM_ENCODERS > 0
    if (index >= NUM_ENCODERS) return;
    
    EncoderConfig_t* cfg = &_encoderConfigs[index];
    EncoderState_Runtime_t* state = &_encoderStates[index];
    
    // Check if expander is initialized
    if (!_initialized[cfg->expander]) return;
    
    // Save previous position for delta calculation
    state->prev_position = state->position;
    
    // Read current pin states
    bool a = _mcp[cfg->expander].digitalRead(cfg->pin_a);
    bool b = _mcp[cfg->expander].digitalRead(cfg->pin_b);
    
    // Apply inversion if needed (for pull-up resistors)
    if (cfg->inverted) {
        a = !a;
        b = !b;
    }
    
    // Quadrature decoding - detect direction from state changes
    if (a != state->last_a) {
        // A changed
        if (a == b) {
            state->position--;
        } else {
            state->position++;
        }
    }
    if (b != state->last_b) {
        // B changed
        if (a == b) {
            state->position++;
        } else {
            state->position--;
        }
    }
    
    state->last_a = a;
    state->last_b = b;
#endif
}

//=============================================================================
// Standard Switch Access
//=============================================================================

SwitchState_t MCP23017_Driver::getSwitchState(uint8_t index) {
#if NUM_SWITCHES > 0
    if (index >= NUM_SWITCHES) return SWITCH_OFF;
    return _switchStates[index].state;
#else
    return SWITCH_OFF;
#endif
}

bool MCP23017_Driver::switchPressed(uint8_t index) {
#if NUM_SWITCHES > 0
    if (index >= NUM_SWITCHES) return false;
    return (_switchStates[index].state == SWITCH_ON && 
            _switchStates[index].prev_state == SWITCH_OFF);
#else
    return false;
#endif
}

bool MCP23017_Driver::switchReleased(uint8_t index) {
#if NUM_SWITCHES > 0
    if (index >= NUM_SWITCHES) return false;
    return (_switchStates[index].state == SWITCH_OFF && 
            _switchStates[index].prev_state == SWITCH_ON);
#else
    return false;
#endif
}

const char* MCP23017_Driver::getSwitchName(uint8_t index) {
#if NUM_SWITCHES > 0
    if (index >= NUM_SWITCHES) return "UNKNOWN";
    return _switchConfigs[index].name;
#else
    return "UNKNOWN";
#endif
}

//=============================================================================
// Navigation Switch Access
//=============================================================================

bool MCP23017_Driver::getNavDirection(uint8_t navIndex, NavDirection_t direction) {
#if NUM_NAV_SWITCHES > 0
    if (navIndex >= NUM_NAV_SWITCHES || direction >= NAV_DIR_COUNT) return false;
    return _navSwitchStates[navIndex].directions[direction];
#else
    return false;
#endif
}

bool MCP23017_Driver::navPressed(uint8_t navIndex, NavDirection_t direction) {
#if NUM_NAV_SWITCHES > 0
    if (navIndex >= NUM_NAV_SWITCHES || direction >= NAV_DIR_COUNT) return false;
    return (_navSwitchStates[navIndex].directions[direction] && 
            !_navSwitchStates[navIndex].prev_directions[direction]);
#else
    return false;
#endif
}

bool MCP23017_Driver::navReleased(uint8_t navIndex, NavDirection_t direction) {
#if NUM_NAV_SWITCHES > 0
    if (navIndex >= NUM_NAV_SWITCHES || direction >= NAV_DIR_COUNT) return false;
    return (!_navSwitchStates[navIndex].directions[direction] && 
            _navSwitchStates[navIndex].prev_directions[direction]);
#else
    return false;
#endif
}

const char* MCP23017_Driver::getNavSwitchName(uint8_t navIndex) {
#if NUM_NAV_SWITCHES > 0
    if (navIndex >= NUM_NAV_SWITCHES) return "UNKNOWN";
    return _navSwitchConfigs[navIndex].name;
#else
    return "UNKNOWN";
#endif
}

//=============================================================================
// Encoder Access
//=============================================================================

int32_t MCP23017_Driver::getEncoderPosition(uint8_t encIndex) {
#if NUM_ENCODERS > 0
    if (encIndex >= NUM_ENCODERS) return 0;
    return _encoderStates[encIndex].position;
#else
    return 0;
#endif
}

void MCP23017_Driver::resetEncoder(uint8_t encIndex) {
#if NUM_ENCODERS > 0
    if (encIndex >= NUM_ENCODERS) return;
    _encoderStates[encIndex].position = 0;
    _encoderStates[encIndex].prev_position = 0;
#endif
}

void MCP23017_Driver::setEncoderPosition(uint8_t encIndex, int32_t position) {
#if NUM_ENCODERS > 0
    if (encIndex >= NUM_ENCODERS) return;
    _encoderStates[encIndex].position = position;
    _encoderStates[encIndex].prev_position = position;
#endif
}

bool MCP23017_Driver::encoderChanged(uint8_t encIndex) {
#if NUM_ENCODERS > 0
    if (encIndex >= NUM_ENCODERS) return false;
    return _encoderStates[encIndex].position != _encoderStates[encIndex].prev_position;
#else
    return false;
#endif
}

int32_t MCP23017_Driver::getEncoderDelta(uint8_t encIndex) {
#if NUM_ENCODERS > 0
    if (encIndex >= NUM_ENCODERS) return 0;
    return _encoderStates[encIndex].position - _encoderStates[encIndex].prev_position;
#else
    return 0;
#endif
}

const char* MCP23017_Driver::getEncoderName(uint8_t encIndex) {
#if NUM_ENCODERS > 0
    if (encIndex >= NUM_ENCODERS) return "UNKNOWN";
    return _encoderConfigs[encIndex].name;
#else
    return "UNKNOWN";
#endif
}

//=============================================================================
// 3-Position Toggle Access (legacy compatibility)
//=============================================================================

Toggle3PosState_t MCP23017_Driver::getToggle3PosState(uint8_t index) {
    return TOGGLE_POS_CENTER;
}

bool MCP23017_Driver::toggle3PosChanged(uint8_t index) {
    return false;
}

const char* MCP23017_Driver::getToggle3PosName(uint8_t index) {
    return "UNKNOWN";
}

//=============================================================================
// Utility Functions
//=============================================================================

bool MCP23017_Driver::readPin(uint8_t expander, uint8_t pin) {
    if (expander > 1 || pin > 15) return false;
    if (!_initialized[expander]) return false;
    return _mcp[expander].digitalRead(pin);
}

bool MCP23017_Driver::isReady() {
    return _initialized[0] && _initialized[1];
}
