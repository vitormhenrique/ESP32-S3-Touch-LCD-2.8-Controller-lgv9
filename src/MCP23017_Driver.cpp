#include "MCP23017_Driver.h"
#include "I2C_Driver.h"

// Global instance
MCP23017_Driver SwitchInput;

// Static configuration arrays
static const SwitchConfig_t _defaultSwitchConfigs[NUM_SWITCHES] = SWITCH_CONFIGS;
static const Toggle3PosConfig_t _defaultToggle3PosConfigs[NUM_3POS_TOGGLES] = TOGGLE_3POS_CONFIGS;

MCP23017_Driver::MCP23017_Driver() {
    _initialized[0] = false;
    _initialized[1] = false;
    _lastUpdateMs = 0;
    _cachedGPIO[0] = 0xFFFF;  // Pull-ups = all high
    _cachedGPIO[1] = 0xFFFF;
}

void MCP23017_Driver::initConfigs() {
    // Copy switch configurations
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        _switchConfigs[i] = _defaultSwitchConfigs[i];
        _switchStates[i].state = SWITCH_OFF;
        _switchStates[i].prev_state = SWITCH_OFF;
        _switchStates[i].last_change_ms = 0;
    }
    
    // Copy 3-position toggle configurations
    for (uint8_t i = 0; i < NUM_3POS_TOGGLES; i++) {
        _toggle3PosConfigs[i] = _defaultToggle3PosConfigs[i];
        _toggle3PosStates[i].state = TOGGLE_POS_CENTER;
        _toggle3PosStates[i].prev_state = TOGGLE_POS_CENTER;
        _toggle3PosStates[i].last_change_ms = 0;
    }
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
    
    return _initialized[0] && _initialized[1];
}

void MCP23017_Driver::update() {
    uint32_t now = millis();
    
    // Bulk-read all 16 pins from each expander in ONE I2C transaction each
    if (I2C_MutexTake(10)) {
        for (uint8_t exp = 0; exp < 2; exp++) {
            if (_initialized[exp]) {
                _cachedGPIO[exp] = _mcp[exp].readGPIOAB();
            }
        }
        I2C_MutexGive();
    }
    
    // Now process all switches/toggles from cached data (no I2C needed)
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        updateSwitch(i);
    }
    for (uint8_t i = 0; i < NUM_3POS_TOGGLES; i++) {
        updateToggle3Pos(i);
    }
    
    _lastUpdateMs = now;
}

void MCP23017_Driver::updateSwitch(uint8_t index) {
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
}

void MCP23017_Driver::updateToggle3Pos(uint8_t index) {
    if (index >= NUM_3POS_TOGGLES) return;
    
    Toggle3PosConfig_t* cfg = &_toggle3PosConfigs[index];
    Toggle3PosState_Runtime_t* state = &_toggle3PosStates[index];
    
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
}

SwitchState_t MCP23017_Driver::getSwitchState(uint8_t index) {
    if (index >= NUM_SWITCHES) return SWITCH_OFF;
    return _switchStates[index].state;
}

bool MCP23017_Driver::switchPressed(uint8_t index) {
    if (index >= NUM_SWITCHES) return false;
    return (_switchStates[index].state == SWITCH_ON && 
            _switchStates[index].prev_state == SWITCH_OFF);
}

bool MCP23017_Driver::switchReleased(uint8_t index) {
    if (index >= NUM_SWITCHES) return false;
    return (_switchStates[index].state == SWITCH_OFF && 
            _switchStates[index].prev_state == SWITCH_ON);
}

Toggle3PosState_t MCP23017_Driver::getToggle3PosState(uint8_t index) {
    if (index >= NUM_3POS_TOGGLES) return TOGGLE_POS_CENTER;
    return _toggle3PosStates[index].state;
}

bool MCP23017_Driver::toggle3PosChanged(uint8_t index) {
    if (index >= NUM_3POS_TOGGLES) return false;
    return (_toggle3PosStates[index].state != _toggle3PosStates[index].prev_state);
}

const char* MCP23017_Driver::getSwitchName(uint8_t index) {
    if (index >= NUM_SWITCHES) return "UNKNOWN";
    return _switchConfigs[index].name;
}

const char* MCP23017_Driver::getToggle3PosName(uint8_t index) {
    if (index >= NUM_3POS_TOGGLES) return "UNKNOWN";
    return _toggle3PosConfigs[index].name;
}

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
