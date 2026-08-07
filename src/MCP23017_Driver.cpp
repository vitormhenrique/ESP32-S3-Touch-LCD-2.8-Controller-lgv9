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

#if MCP_USE_INTERRUPT
volatile bool MCP23017_Driver::_interruptPending = false;

void IRAM_ATTR MCP23017_Driver::_isrHandler() {
    _interruptPending = true;
}
#endif

MCP23017_Driver::MCP23017_Driver() {
    _initialized[0] = false;
    _initialized[1] = false;
    _lastUpdateMs = 0;
    _cachedGPIO[0] = 0xFFFF;  // Pull-ups = all high
    _cachedGPIO[1] = 0xFFFF;
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
    if (I2C_MutexTake(100)) {
        if (_mcp[0].begin_I2C(MCP23017_ADDR_1, &Wire)) {
            _initialized[0] = true;
            printf("MCP23017 #1 (0x%02X) initialized\r\n", MCP23017_ADDR_1);
            for (uint8_t pin = 0; pin < 16; pin++) {
                _mcp[0].pinMode(pin, INPUT_PULLUP);
            }
        } else {
            printf("MCP23017 #1 (0x%02X) initialization FAILED\r\n", MCP23017_ADDR_1);
        }
        I2C_MutexGive();
    }
    
    // Initialize second MCP23017
    if (I2C_MutexTake(100)) {
        if (_mcp[1].begin_I2C(MCP23017_ADDR_2, &Wire)) {
            _initialized[1] = true;
            printf("MCP23017 #2 (0x%02X) initialized\r\n", MCP23017_ADDR_2);
            for (uint8_t pin = 0; pin < 16; pin++) {
                _mcp[1].pinMode(pin, INPUT_PULLUP);
            }
        } else {
            printf("MCP23017 #2 (0x%02X) initialization FAILED\r\n", MCP23017_ADDR_2);
        }
        I2C_MutexGive();
    }
    
#if MCP_USE_INTERRUPT
    // Setup MCP23017 interrupt-on-change after init
    if (_initialized[0] || _initialized[1]) {
        configureInterrupts();
    }
#endif
    
    return _initialized[0] && _initialized[1];
}

#if MCP_USE_INTERRUPT
void MCP23017_Driver::configureInterrupts() {
    // Configure both MCPs: mirror INTA/INTB (tied together physically),
    // open-drain outputs (wire-OR safe), active-LOW
    for (uint8_t i = 0; i < 2; i++) {
        if (!_initialized[i]) continue;
        _mcp[i].setupInterrupts(true, true, LOW);
        // Enable interrupt-on-change for all 16 pins
        for (uint8_t pin = 0; pin < 16; pin++) {
            _mcp[i].setupInterruptPin(pin, CHANGE);
        }
        // Clear any pending interrupts by reading INTCAP
        _mcp[i].clearInterrupts();
    }

    // Configure ESP32 GPIO with internal pull-up (open-drain needs it)
    pinMode(MCP_INT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MCP_INT_PIN), _isrHandler, FALLING);

    printf("MCP23017: Interrupt mode enabled on GPIO%d\r\n", MCP_INT_PIN);
}
#endif

void MCP23017_Driver::update() {
    uint32_t now = millis();

#if MCP_USE_INTERRUPT
    // Interrupt mode: only do I2C read when interrupt fired or periodic fallback
    bool needsRead = _interruptPending;

    // Periodic fallback to catch any edge-case missed interrupts
    if ((now - _lastUpdateMs) >= MCP_INT_FALLBACK_MS) {
        needsRead = true;
    }

    if (needsRead) {
        _interruptPending = false;
        if (I2C_MutexTake(10)) {
            for (uint8_t exp = 0; exp < 2; exp++) {
                if (_initialized[exp]) {
                    _cachedGPIO[exp] = _mcp[exp].readGPIOAB();
                }
            }
            I2C_MutexGive();
        }
    }
#else
    // Polling mode: always bulk-read
    if (I2C_MutexTake(10)) {
        for (uint8_t exp = 0; exp < 2; exp++) {
            if (_initialized[exp]) {
                _cachedGPIO[exp] = _mcp[exp].readGPIOAB();
            }
        }
        I2C_MutexGive();
    }
#endif
    
    // Now process all switches/toggles from cached data (no I2C needed)
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        updateSwitch(i);
    }
    for (uint8_t i = 0; i < NUM_3POS_TOGGLES; i++) {
        updateToggle3Pos(i);
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
    
    if (!_initialized[cfg->expander]) return;
    
    // Read from cached GPIO (no I2C!)
    bool pinState = getCachedPin(cfg->expander, cfg->pin);
    
    if (cfg->inverted) {
        pinState = !pinState;
    }
    
    SwitchState_t newState = pinState ? SWITCH_ON : SWITCH_OFF;
    uint32_t now = millis();
    
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
    
    if (!_initialized[cfg->expander]) return;
    
    // Read from cached GPIO (no I2C!)
    bool pinUp = getCachedPin(cfg->expander, cfg->pin_up);
    bool pinDown = getCachedPin(cfg->expander, cfg->pin_down);
    
    if (cfg->inverted) {
        pinUp = !pinUp;
        pinDown = !pinDown;
    }
    
    Toggle3PosState_t newState;
    if (pinUp && !pinDown) {
        newState = TOGGLE_POS_UP;
    } else if (!pinUp && pinDown) {
        newState = TOGGLE_POS_DOWN;
    } else {
        newState = TOGGLE_POS_CENTER;
    }
    
    uint32_t now = millis();
    
    if (newState != state->state) {
        if ((now - state->last_change_ms) >= DEBOUNCE_MS) {
            state->prev_state = state->state;
            state->state = newState;
            state->last_change_ms = now;
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
    
    // Thread-safe I2C access using global mutex
    bool result = false;
    if (I2C_MutexTake(10)) {
        result = _mcp[expander].digitalRead(pin);
        I2C_MutexGive();
    }
    return result;
}

bool MCP23017_Driver::getCachedPin(uint8_t expander, uint8_t pin) {
    if (expander > 1 || pin > 15) return false;
    return (_cachedGPIO[expander] >> pin) & 0x01;
}

uint16_t MCP23017_Driver::getCachedGPIO(uint8_t expander) {
    if (expander > 1) return 0xFFFF;
    return _cachedGPIO[expander];
}

bool MCP23017_Driver::isReady() {
    return _initialized[0] && _initialized[1];
}

#if MCP_USE_INTERRUPT
bool MCP23017_Driver::checkAndUpdateInterrupt() {
    if (!_interruptPending) return false;

    _interruptPending = false;

    // Read BOTH chips to update cache AND clear ALL tied interrupts
    if (I2C_MutexTake(5)) {
        for (uint8_t exp = 0; exp < 2; exp++) {
            if (_initialized[exp]) {
                _cachedGPIO[exp] = _mcp[exp].readGPIOAB();
            }
        }
        I2C_MutexGive();
        return true;
    }
    // Mutex busy — re-flag so next caller retries
    _interruptPending = true;
    return false;
}
#endif
