#include "PerfMonitor.h"

#if DEBUG_PERF

#include <Arduino.h>
#include <stdio.h>

//=============================================================================
// Internal Data Structures
//=============================================================================

typedef struct {
    uint32_t count;              // Number of iterations
    uint32_t total_time_us;      // Total time spent in microseconds
    uint32_t min_time_us;        // Minimum iteration time
    uint32_t max_time_us;        // Maximum iteration time
    uint32_t start_time_us;      // Start time for current section
} PerfData_t;

static const char* counter_names[PERF_COUNTER_COUNT] = {
    "Main Loop",
    "Driver Loop", 
    "Input Update",
    "LVGL Loop",
    "LVGL Render",
    "UI Update"
};

static PerfData_t perf_data[PERF_COUNTER_COUNT];
static uint32_t last_report_time = 0;
static uint32_t report_interval_ms = PERF_REPORT_INTERVAL_MS;
static bool initialized = false;

// Cached values for UI display
static float cached_fps = 0.0f;
static float cached_main_hz = 0.0f;
static float cached_driver_hz = 0.0f;

//=============================================================================
// Implementation
//=============================================================================

void Perf_Init(void) {
    memset(perf_data, 0, sizeof(perf_data));
    
    // Initialize min times to max value
    for (int i = 0; i < PERF_COUNTER_COUNT; i++) {
        perf_data[i].min_time_us = UINT32_MAX;
    }
    
    last_report_time = millis();
    initialized = true;
    
    Serial.println("\n===== Performance Monitor Initialized =====");
    Serial.printf("Reporting every %d ms\n", report_interval_ms);
    Serial.println("============================================\n");
}

void Perf_StartSection(PerfCounter_t counter) {
    if (!initialized || counter >= PERF_COUNTER_COUNT) return;
    perf_data[counter].start_time_us = micros();
}

void Perf_EndSection(PerfCounter_t counter) {
    if (!initialized || counter >= PERF_COUNTER_COUNT) return;
    
    uint32_t end_time = micros();
    uint32_t elapsed = end_time - perf_data[counter].start_time_us;
    
    perf_data[counter].count++;
    perf_data[counter].total_time_us += elapsed;
    
    if (elapsed < perf_data[counter].min_time_us) {
        perf_data[counter].min_time_us = elapsed;
    }
    if (elapsed > perf_data[counter].max_time_us) {
        perf_data[counter].max_time_us = elapsed;
    }
}

void Perf_Increment(PerfCounter_t counter) {
    if (!initialized || counter >= PERF_COUNTER_COUNT) return;
    perf_data[counter].count++;
}

void Perf_CheckReport(void) {
    if (!initialized) return;
    
    uint32_t now = millis();
    if (now - last_report_time >= report_interval_ms) {
        Perf_Report();
        last_report_time = now;
    }
}

void Perf_Report(void) {
    if (!initialized) return;
    
    float elapsed_sec = (float)report_interval_ms / 1000.0f;
    
    Serial.println("\n╔══════════════════════════════════════════════════════════════════╗");
    Serial.println("║              PERFORMANCE REPORT (5 second average)               ║");
    Serial.println("╠══════════════════════════════════════════════════════════════════╣");
    Serial.println("║ Counter         │  Count  │  Hz    │ Avg(us) │ Min(us) │ Max(us) ║");
    Serial.println("╠─────────────────┼─────────┼────────┼─────────┼─────────┼─────────╣");
    
    for (int i = 0; i < PERF_COUNTER_COUNT; i++) {
        PerfData_t* p = &perf_data[i];
        
        float hz = (float)p->count / elapsed_sec;
        float avg_us = p->count > 0 ? (float)p->total_time_us / p->count : 0;
        uint32_t min_us = p->min_time_us == UINT32_MAX ? 0 : p->min_time_us;
        
        // Cache values for UI
        if (i == PERF_COUNTER_LVGL_RENDER) {
            cached_fps = hz;
        } else if (i == PERF_COUNTER_MAIN_LOOP) {
            cached_main_hz = hz;
        } else if (i == PERF_COUNTER_DRIVER_LOOP) {
            cached_driver_hz = hz;
        }
        
        Serial.printf("║ %-15s │ %7u │ %6.1f │ %7.1f │ %7u │ %7u ║\n",
                      counter_names[i],
                      p->count,
                      hz,
                      avg_us,
                      min_us,
                      p->max_time_us);
    }
    
    // Calculate and display additional metrics
    Serial.println("╠══════════════════════════════════════════════════════════════════╣");
    
    // Main loop duty cycle
    if (perf_data[PERF_COUNTER_MAIN_LOOP].count > 0) {
        float main_avg_ms = (float)perf_data[PERF_COUNTER_MAIN_LOOP].total_time_us / 
                           perf_data[PERF_COUNTER_MAIN_LOOP].count / 1000.0f;
        float main_duty = (main_avg_ms * cached_main_hz) / 10.0f;  // % of time in loop
        Serial.printf("║ Main Loop Duty Cycle: %.1f%% (avg %.2f ms/iteration)            ║\n", 
                      main_duty, main_avg_ms);
    }
    
    // LVGL render efficiency
    if (perf_data[PERF_COUNTER_LVGL_LOOP].count > 0 && 
        perf_data[PERF_COUNTER_LVGL_RENDER].count > 0) {
        float render_ratio = (float)perf_data[PERF_COUNTER_LVGL_RENDER].count / 
                            perf_data[PERF_COUNTER_LVGL_LOOP].count * 100.0f;
        Serial.printf("║ LVGL Render Ratio: %.1f%% of loops resulted in render           ║\n", 
                      render_ratio);
    }
    
    // Free heap memory
    Serial.printf("║ Free Heap: %u bytes (Min: %u bytes)                        ║\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap());
    
    // PSRAM if available
    if (ESP.getPsramSize() > 0) {
        Serial.printf("║ Free PSRAM: %u bytes                                       ║\n",
                      ESP.getFreePsram());
    }
    
    Serial.println("╚══════════════════════════════════════════════════════════════════╝\n");
    
    // Reset counters for next period
    for (int i = 0; i < PERF_COUNTER_COUNT; i++) {
        perf_data[i].count = 0;
        perf_data[i].total_time_us = 0;
        perf_data[i].min_time_us = UINT32_MAX;
        perf_data[i].max_time_us = 0;
    }
}

float Perf_GetFPS(void) {
    return cached_fps;
}

float Perf_GetMainLoopHz(void) {
    return cached_main_hz;
}

float Perf_GetDriverLoopHz(void) {
    return cached_driver_hz;
}

#endif // DEBUG_PERF
