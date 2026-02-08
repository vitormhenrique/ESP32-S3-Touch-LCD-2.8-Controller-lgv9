#include "DebugMetrics.h"

#if DEBUG_METRICS

// Global instance
DebugMetrics Debug;

DebugMetrics::DebugMetrics() {
    _lastPrintMs = 0;
    _printIntervalMs = 5000;  // Print every 5 seconds
    
    _loopStartUs = 0;
    _loopCount = 0;
    _totalLoopTimeUs = 0;
    _maxLoopTimeUs = 0;
    _currentLoopRate = 0;
    _avgLoopTimeUs = 0;
    
    _frameCount = 0;
    _currentFPS = 0;
    
    _inputCount = 0;
    _currentInputRate = 0;
    
    _lastCalcMs = 0;
}

void DebugMetrics::begin() {
    printf("=== Debug Metrics Enabled ===\r\n");
    printf("Metrics will be printed every %lu seconds\r\n", _printIntervalMs / 1000);
    _lastPrintMs = millis();
    _lastCalcMs = millis();
}

void DebugMetrics::loopStart() {
    _loopStartUs = micros();
}

void DebugMetrics::loopEnd() {
    uint32_t loopTimeUs = micros() - _loopStartUs;
    _totalLoopTimeUs += loopTimeUs;
    _loopCount++;
    
    if (loopTimeUs > _maxLoopTimeUs) {
        _maxLoopTimeUs = loopTimeUs;
    }
}

void DebugMetrics::frameRendered() {
    _frameCount++;
}

void DebugMetrics::inputUpdated() {
    _inputCount++;
}

bool DebugMetrics::update() {
    uint32_t now = millis();
    uint32_t elapsed = now - _lastCalcMs;
    
    // Calculate rates every second
    if (elapsed >= 1000) {
        float elapsedSec = elapsed / 1000.0f;
        
        _currentLoopRate = _loopCount / elapsedSec;
        _currentFPS = _frameCount / elapsedSec;
        _currentInputRate = _inputCount / elapsedSec;
        
        if (_loopCount > 0) {
            _avgLoopTimeUs = _totalLoopTimeUs / _loopCount;
        }
        
        // Reset counters
        _loopCount = 0;
        _totalLoopTimeUs = 0;
        _frameCount = 0;
        _inputCount = 0;
        _lastCalcMs = now;
    }
    
    // Print metrics periodically
    if ((now - _lastPrintMs) >= _printIntervalMs) {
        printMetrics();
        _maxLoopTimeUs = 0;  // Reset max after printing
        _lastPrintMs = now;
        return true;
    }
    
    return false;
}

void DebugMetrics::printMetrics() {
    printf("\r\n");
    printf("╔══════════════════════════════════════════╗\r\n");
    printf("║       PERFORMANCE METRICS                ║\r\n");
    printf("╠══════════════════════════════════════════╣\r\n");
    printf("║ Display FPS:      %6.1f Hz              ║\r\n", _currentFPS);
    printf("║ Main Loop Rate:   %6.1f Hz              ║\r\n", _currentLoopRate);
    printf("║ Input Rate:       %6.1f Hz              ║\r\n", _currentInputRate);
    printf("╠──────────────────────────────────────────╣\r\n");
    printf("║ Avg Loop Time:    %6lu µs              ║\r\n", _avgLoopTimeUs);
    printf("║ Max Loop Time:    %6lu µs              ║\r\n", _maxLoopTimeUs);
    printf("╠──────────────────────────────────────────╣\r\n");
    printf("║ Free Heap:        %6lu KB              ║\r\n", getFreeHeap() / 1024);
    printf("║ Largest Block:    %6lu KB              ║\r\n", getLargestFreeBlock() / 1024);
    printf("║ Total PSRAM:      %6lu KB              ║\r\n", ESP.getPsramSize() / 1024);
    printf("║ Free PSRAM:       %6lu KB              ║\r\n", ESP.getFreePsram() / 1024);
    printf("╚══════════════════════════════════════════╝\r\n");
    printf("\r\n");
}

#endif // DEBUG_METRICS
