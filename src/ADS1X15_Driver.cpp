#include "ADS1X15_Driver.h"
#include "I2C_Driver.h"

// Global instance
ADS1X15_Driver AnalogInput;

// Static configuration arrays
static const AnalogAxisConfig_t _defaultGimbalConfigs[NUM_GIMBAL_AXES] = GIMBAL_CONFIGS;
static const AnalogAxisConfig_t _defaultPotConfigs[NUM_POTENTIOMETERS] = POT_CONFIGS;

ADS1X15_Driver::ADS1X15_Driver() {
    _initialized[0] = false;
    _initialized[1] = false;
    _initialized[2] = false;
    _filterIndex = 0;
}

void ADS1X15_Driver::initConfigs() {
    // Copy gimbal configurations
    for (uint8_t i = 0; i < NUM_GIMBAL_AXES; i++) {
        _gimbalConfigs[i] = _defaultGimbalConfigs[i];
        _gimbalStates[i].raw = 0;
        _gimbalStates[i].calibrated = 0;
        _gimbalStates[i].filtered = 0;
        
        // Initialize filter buffer
        for (uint8_t j = 0; j < ANALOG_FILTER_SAMPLES; j++) {
            _gimbalFilterBuf[i][j] = 0;
        }
    }
    
    // Copy potentiometer configurations
    for (uint8_t i = 0; i < NUM_POTENTIOMETERS; i++) {
        _potConfigs[i] = _defaultPotConfigs[i];
        _potStates[i].raw = 0;
        _potStates[i].calibrated = 0;
        _potStates[i].filtered = 0;
        
        // Initialize filter buffer
        for (uint8_t j = 0; j < ANALOG_FILTER_SAMPLES; j++) {
            _potFilterBuf[i][j] = 0;
        }
    }
}

bool ADS1X15_Driver::begin() {
    initConfigs();
    
    // Initialize first ADS1115 (gimbals)
    if (_ads[0].begin(ADS1X15_ADDR_1, &Wire)) {
        _initialized[0] = true;
        _ads[0].setGain(GAIN_ONE);      // +/- 4.096V range
        _ads[0].setDataRate(RATE_ADS1115_250SPS);  // Fast sampling
        printf("ADS1115 #1 (0x%02X) initialized - Gimbals\r\n", ADS1X15_ADDR_1);
    } else {
        printf("ADS1115 #1 (0x%02X) initialization FAILED\r\n", ADS1X15_ADDR_1);
    }
    
    // Initialize second ADS1115 (potentiometers + spare)
    if (_ads[1].begin(ADS1X15_ADDR_2, &Wire)) {
        _initialized[1] = true;
        _ads[1].setGain(GAIN_ONE);
        _ads[1].setDataRate(RATE_ADS1115_250SPS);
        printf("ADS1115 #2 (0x%02X) initialized - Pots\r\n", ADS1X15_ADDR_2);
    } else {
        printf("ADS1115 #2 (0x%02X) initialization FAILED\r\n", ADS1X15_ADDR_2);
    }
    
    // Initialize third ADS1115 (spare/expansion)
    if (_ads[2].begin(ADS1X15_ADDR_3, &Wire)) {
        _initialized[2] = true;
        _ads[2].setGain(GAIN_ONE);
        _ads[2].setDataRate(RATE_ADS1115_250SPS);
        printf("ADS1115 #3 (0x%02X) initialized - Spare\r\n", ADS1X15_ADDR_3);
    } else {
        printf("ADS1115 #3 (0x%02X) not found (optional)\r\n", ADS1X15_ADDR_3);
    }
    
    // At least ADC 0 and 1 must be initialized for basic operation
    return _initialized[0] && _initialized[1];
}

void ADS1X15_Driver::update() {
    // All ADC reads share the I2C bus - take mutex once for entire update
    if (!I2C_MutexTake(20)) return;  // Skip this cycle if bus is busy
    
    // Update all gimbal axes
    for (uint8_t i = 0; i < NUM_GIMBAL_AXES; i++) {
        updateGimbalAxis(i);
    }
    
    // Update all potentiometers
    for (uint8_t i = 0; i < NUM_POTENTIOMETERS; i++) {
        updatePotentiometer(i);
    }
    
    I2C_MutexGive();
    
    // Advance filter index
    _filterIndex = (_filterIndex + 1) % ANALOG_FILTER_SAMPLES;
}

void ADS1X15_Driver::updateGimbalAxis(uint8_t index) {
    if (index >= NUM_GIMBAL_AXES) return;
    
    AnalogAxisConfig_t* cfg = &_gimbalConfigs[index];
    AnalogAxisState_t* state = &_gimbalStates[index];
    
    // Check if ADC is initialized
    if (!_initialized[cfg->adc]) return;
    
    // Read raw ADC value
    int16_t raw = _ads[cfg->adc].readADC_SingleEnded(cfg->channel);
    state->raw = raw;
    
    // Apply calibration
    state->calibrated = calibrateGimbal(raw, cfg);
    
    // Apply filter
    state->filtered = applyFilter(_gimbalFilterBuf[index], state->calibrated);
}

void ADS1X15_Driver::updatePotentiometer(uint8_t index) {
    if (index >= NUM_POTENTIOMETERS) return;
    
    AnalogAxisConfig_t* cfg = &_potConfigs[index];
    AnalogAxisState_t* state = &_potStates[index];
    
    // Check if ADC is initialized
    if (!_initialized[cfg->adc]) return;
    
    // Read raw ADC value
    int16_t raw = _ads[cfg->adc].readADC_SingleEnded(cfg->channel);
    state->raw = raw;
    
    // Apply calibration
    state->calibrated = calibratePot(raw, cfg);
    
    // Apply filter
    state->filtered = applyFilter(_potFilterBuf[index], state->calibrated);
}

int16_t ADS1X15_Driver::calibrateGimbal(int16_t raw, const AnalogAxisConfig_t* cfg) {
    int16_t value;
    int16_t center = cfg->center_raw;
    
    // Apply deadzone around center
    if (abs(raw - center) < cfg->deadzone) {
        return ANALOG_OUTPUT_CENTER;
    }
    
    // Map to output range with center at 0
    if (raw < center) {
        // Below center: map min_raw..center to -1000..0
        value = map(raw, cfg->min_raw, center - cfg->deadzone, 
                    ANALOG_OUTPUT_MIN, ANALOG_OUTPUT_CENTER);
    } else {
        // Above center: map center..max_raw to 0..1000
        value = map(raw, center + cfg->deadzone, cfg->max_raw, 
                    ANALOG_OUTPUT_CENTER, ANALOG_OUTPUT_MAX);
    }
    
    // Clamp to valid range
    value = constrain(value, ANALOG_OUTPUT_MIN, ANALOG_OUTPUT_MAX);
    
    // Apply inversion if needed
    if (cfg->inverted) {
        value = -value;
    }
    
    return value;
}

int16_t ADS1X15_Driver::calibratePot(int16_t raw, const AnalogAxisConfig_t* cfg) {
    // Map to output range (0 to 1000 for pots)
    int16_t value = map(raw, cfg->min_raw, cfg->max_raw, POT_OUTPUT_MIN, POT_OUTPUT_MAX);
    
    // Clamp to valid range
    value = constrain(value, POT_OUTPUT_MIN, POT_OUTPUT_MAX);
    
    // Apply inversion if needed
    if (cfg->inverted) {
        value = POT_OUTPUT_MAX - value;
    }
    
    return value;
}

int16_t ADS1X15_Driver::applyFilter(int16_t* buffer, int16_t newValue) {
    // Store new value in circular buffer
    buffer[_filterIndex] = newValue;
    
    // Calculate moving average
    int32_t sum = 0;
    for (uint8_t i = 0; i < ANALOG_FILTER_SAMPLES; i++) {
        sum += buffer[i];
    }
    
    return (int16_t)(sum / ANALOG_FILTER_SAMPLES);
}

int16_t ADS1X15_Driver::getGimbalAxis(uint8_t index) {
    if (index >= NUM_GIMBAL_AXES) return 0;
    return _gimbalStates[index].filtered;
}

int16_t ADS1X15_Driver::getGimbalAxisRaw(uint8_t index) {
    if (index >= NUM_GIMBAL_AXES) return 0;
    return _gimbalStates[index].raw;
}

int16_t ADS1X15_Driver::getPotentiometer(uint8_t index) {
    if (index >= NUM_POTENTIOMETERS) return 0;
    return _potStates[index].filtered;
}

int16_t ADS1X15_Driver::getPotentiometerRaw(uint8_t index) {
    if (index >= NUM_POTENTIOMETERS) return 0;
    return _potStates[index].raw;
}

const char* ADS1X15_Driver::getGimbalAxisName(uint8_t index) {
    if (index >= NUM_GIMBAL_AXES) return "UNKNOWN";
    return _gimbalConfigs[index].name;
}

const char* ADS1X15_Driver::getPotentiometerName(uint8_t index) {
    if (index >= NUM_POTENTIOMETERS) return "UNKNOWN";
    return _potConfigs[index].name;
}

void ADS1X15_Driver::calibrateGimbalAxis(uint8_t index, int16_t min_val, int16_t center_val, int16_t max_val, int16_t deadzone, bool inverted) {
    if (index >= NUM_GIMBAL_AXES) return;
    _gimbalConfigs[index].min_raw = min_val;
    _gimbalConfigs[index].center_raw = center_val;
    _gimbalConfigs[index].max_raw = max_val;
    _gimbalConfigs[index].deadzone = deadzone;
    _gimbalConfigs[index].inverted = inverted;
    printf("ADS1X15: Calibrated axis %d: min=%d, center=%d, max=%d, dz=%d, inv=%d\r\n",
           index, min_val, center_val, max_val, deadzone, inverted);
}

void ADS1X15_Driver::calibratePotentiometer(uint8_t index, int16_t min_val, int16_t max_val) {
    if (index >= NUM_POTENTIOMETERS) return;
    _potConfigs[index].min_raw = min_val;
    _potConfigs[index].max_raw = max_val;
}

bool ADS1X15_Driver::isReady() {
    return _initialized[0] && _initialized[1];
}

int16_t ADS1X15_Driver::readRaw(uint8_t adc, uint8_t channel) {
    if (adc > 2 || channel > 3) return 0;
    if (!_initialized[adc]) return 0;
    
    int16_t result = 0;
    if (I2C_MutexTake(20)) {
        result = _ads[adc].readADC_SingleEnded(channel);
        I2C_MutexGive();
    }
    return result;
}
