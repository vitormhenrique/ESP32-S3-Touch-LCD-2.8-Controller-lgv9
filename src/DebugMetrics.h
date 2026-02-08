#pragma once
#include <Arduino.h>

/******************************************************************************
 * Debug Metrics System
 * 
 * Performance monitoring for the RC Controller
 * Only active when DEBUG_METRICS is defined
 ******************************************************************************/

// Set to true to enable debug metrics
#ifndef DEBUG_METRICS
#define DEBUG_METRICS 0
#endif

#if DEBUG_METRICS

class DebugMetrics {
public:
    DebugMetrics();
    
    /**
     * Initialize the metrics system
     */
    void begin();
    
    /**
     * Call at the start of main loop
     */
    void loopStart();
    
    /**
     * Call at the end of main loop
     */
    void loopEnd();
    
    /**
     * Call when LVGL rendering completes
     */
    void frameRendered();
    
    /**
     * Call when input update completes
     */
    void inputUpdated();
    
    /**
     * Update metrics (call periodically)
     * @return true if metrics were just printed
     */
    bool update();
    
    /**
     * Get current FPS
     */
    float getFPS() { return _currentFPS; }
    
    /**
     * Get main loop rate (Hz)
     */
    float getLoopRate() { return _currentLoopRate; }
    
    /**
     * Get input update rate (Hz)
     */
    float getInputRate() { return _currentInputRate; }
    
    /**
     * Get average loop time (microseconds)
     */
    uint32_t getAvgLoopTime() { return _avgLoopTimeUs; }
    
    /**
     * Get max loop time (microseconds)
     */
    uint32_t getMaxLoopTime() { return _maxLoopTimeUs; }
    
    /**
     * Get free heap memory
     */
    uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    
    /**
     * Get largest free block
     */
    uint32_t getLargestFreeBlock() { return ESP.getMaxAllocHeap(); }
    
    /**
     * Print metrics to serial
     */
    void printMetrics();

private:
    // Timing
    uint32_t _lastPrintMs;
    uint32_t _printIntervalMs;
    
    // Loop timing
    uint32_t _loopStartUs;
    uint32_t _loopCount;
    uint32_t _totalLoopTimeUs;
    uint32_t _maxLoopTimeUs;
    float _currentLoopRate;
    uint32_t _avgLoopTimeUs;
    
    // Frame timing
    uint32_t _frameCount;
    float _currentFPS;
    
    // Input timing
    uint32_t _inputCount;
    float _currentInputRate;
    
    // Reference time for rate calculations
    uint32_t _lastCalcMs;
};

// Global instance
extern DebugMetrics Debug;

// Convenience macros for debug builds
#define DEBUG_LOOP_START() Debug.loopStart()
#define DEBUG_LOOP_END() Debug.loopEnd()
#define DEBUG_FRAME_RENDERED() Debug.frameRendered()
#define DEBUG_INPUT_UPDATED() Debug.inputUpdated()
#define DEBUG_UPDATE() Debug.update()

#else // DEBUG_METRICS disabled

// Empty macros when debug is disabled
#define DEBUG_LOOP_START()
#define DEBUG_LOOP_END()
#define DEBUG_FRAME_RENDERED()
#define DEBUG_INPUT_UPDATED()
#define DEBUG_UPDATE()

#endif // DEBUG_METRICS
