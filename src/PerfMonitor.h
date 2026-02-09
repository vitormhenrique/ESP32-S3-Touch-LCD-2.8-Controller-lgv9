#pragma once

/******************************************************************************
 * Performance Monitor Module
 * 
 * Tracks and reports various performance metrics when DEBUG_PERF is enabled.
 * Measures loop frequencies, frame rates, and timing statistics.
 * 
 * Enable by adding -DDEBUG_PERF=1 to build_flags in platformio.ini
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// Configuration
//=============================================================================

#ifndef DEBUG_PERF
#define DEBUG_PERF 0
#endif

#define PERF_REPORT_INTERVAL_MS  5000  // Report every 5 seconds

//=============================================================================
// Performance Counter IDs
//=============================================================================

typedef enum {
    PERF_COUNTER_MAIN_LOOP = 0,    // Main loop() iterations
    PERF_COUNTER_DRIVER_LOOP,       // Driver task iterations
    PERF_COUNTER_INPUT_UPDATE,      // Input update calls
    PERF_COUNTER_LVGL_LOOP,         // LVGL loop calls
    PERF_COUNTER_LVGL_RENDER,       // LVGL actual renders (when dirty)
    PERF_COUNTER_UI_UPDATE,         // UI update calls
    PERF_COUNTER_COUNT              // Total number of counters
} PerfCounter_t;

//=============================================================================
// API - Only active when DEBUG_PERF is enabled
//=============================================================================

#if DEBUG_PERF

// Initialize performance monitor
void Perf_Init(void);

// Call at the start of a section to begin timing
void Perf_StartSection(PerfCounter_t counter);

// Call at the end of a section - increments count and records timing
void Perf_EndSection(PerfCounter_t counter);

// Simple increment for counting iterations without timing
void Perf_Increment(PerfCounter_t counter);

// Check if it's time to report and print stats (call periodically)
void Perf_CheckReport(void);

// Force immediate report
void Perf_Report(void);

// Get current FPS for display in UI
float Perf_GetFPS(void);

// Get main loop frequency
float Perf_GetMainLoopHz(void);

// Get driver loop frequency
float Perf_GetDriverLoopHz(void);

#else

// No-op macros when DEBUG_PERF is disabled
#define Perf_Init()                     ((void)0)
#define Perf_StartSection(counter)      ((void)0)
#define Perf_EndSection(counter)        ((void)0)
#define Perf_Increment(counter)         ((void)0)
#define Perf_CheckReport()              ((void)0)
#define Perf_Report()                   ((void)0)
#define Perf_GetFPS()                   (0.0f)
#define Perf_GetMainLoopHz()            (0.0f)
#define Perf_GetDriverLoopHz()          (0.0f)

#endif

#ifdef __cplusplus
}
#endif
