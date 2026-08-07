/**
 * @file main.c
 * LVGL Desktop Simulator - SDL2 based
 * Runs the same UI as the ESP32-S3 microcontroller
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens/ui_screen_settings.h"
#include "ui/screens/ui_screen_telemetry.h"
#include "Settings.h"
#include "elrs_client.h"
#include "elrs_sim.h"

/* Simulator display/input driver declarations */
void lv_sdl_display_init(void);
void lv_sdl_mouse_init(void);
void lv_sdl_poll_events(void);
bool lv_sdl_should_quit(void);

/* Screen resolution */
#define DISP_HOR_RES 320
#define DISP_VER_RES 240

/* Tick period in ms */
#define TICK_PERIOD_MS 5

static uint32_t get_tick_ms(void)
{
    return SDL_GetTicks();
}

/* Simulate gimbal values for testing */
static void update_simulated_gimbals(void)
{
    /* Generate simulated gimbal values using keyboard control */
    static int16_t gimbal_base[4] = {2048, 2048, 2048, 2048};
    static uint32_t last_auto_update = 0;
    
    /* Check keyboard state for manual control */
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    
    /* If calibration is active, arrow keys control the current axis */
    /* Otherwise, use 1-4 keys to select axis and arrows to adjust */
    uint8_t axis = ui_gimbal_cal_is_active() ? ui_gimbal_cal_get_axis() : 0;
    
    /* Number keys 1-4 select axis when not in calibration */
    if (!ui_gimbal_cal_is_active()) {
        if (keys[SDL_SCANCODE_1]) axis = 0;
        if (keys[SDL_SCANCODE_2]) axis = 1;
        if (keys[SDL_SCANCODE_3]) axis = 2;
        if (keys[SDL_SCANCODE_4]) axis = 3;
    }
    
    if (axis < 4) {
        if (keys[SDL_SCANCODE_UP]) {
            gimbal_base[axis] = (gimbal_base[axis] < 4000) ? gimbal_base[axis] + 50 : 4095;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            gimbal_base[axis] = (gimbal_base[axis] > 95) ? gimbal_base[axis] - 50 : 0;
        }
        /* Quick presets: HOME for center, END for min, PAGEUP for max */
        if (keys[SDL_SCANCODE_HOME]) {
            gimbal_base[axis] = 2048;
        }
        if (keys[SDL_SCANCODE_END]) {
            gimbal_base[axis] = 100;
        }
        if (keys[SDL_SCANCODE_PAGEUP]) {
            gimbal_base[axis] = 3995;
        }
    }
    
    /* Auto-oscillate values slowly when not being controlled */
    uint32_t now = SDL_GetTicks();
    if (now - last_auto_update > 100) {
        last_auto_update = now;
        /* Add slight oscillation to make values look alive */
        for (int i = 0; i < 4; i++) {
            int16_t noise = (rand() % 5) - 2;  /* -2 to +2 */
            gimbal_base[i] = LV_CLAMP(0, gimbal_base[i] + noise, 4095);
        }
    }
    
    int16_t gimbal_values[4];
    for (int i = 0; i < 4; i++) {
        gimbal_values[i] = gimbal_base[i];
    }
    
    ui_gimbal_cal_update(gimbal_values);
}

static void update_simulated_hexapod(void)
{
    static const HexapodTelemetryStatus status = {
        .flags = HEXAPOD_FLAG_ARMED | HEXAPOD_FLAG_MOTION_GATE |
                 HEXAPOD_FLAG_IMU_PRESENT | HEXAPOD_FLAG_IMU_FRESH |
                 HEXAPOD_FLAG_BATTERY_VALID,
        .safety_state = 5,
        .command_source = 1,
        .gait = 2,
        .control_mode = 0,
        .fault_reason = 0,
        .speed_x255 = 166,
        .duty_x255 = 153,
        .imu_calibration = 0xFF,
        .battery_mv = 12000,
        .body_height_mm = 40,
        .stride_mm = 45,
        .step_height_mm = 24,
        .tune_flags = HEXAPOD_TUNE_ACTIVE | HEXAPOD_TUNE_PREVIEW |
                      (HEXAPOD_TUNE_PARAM_STEP_HEIGHT
                       << HEXAPOD_TUNE_PARAM_SHIFT) |
                      (1u << HEXAPOD_TUNE_SEVERITY_SHIFT),
        .error_code = 15,  // Goal Clamped
        .error_detail = 0,
        .error_sequence = 3,
        .error_count = 12,
        .error_suppressed = 480,
    };

    ui_telemetry_update_battery(12.0f, true, true);
    ui_telemetry_update_hexapod(&status, true, 20);
    ui_telemetry_set_imu_state(true, true);
    ui_telemetry_update_imu9(2.4f, -1.2f, 87.0f,
                             3.0f, 3.0f, 3.0f,
                             12.0f, 100.0f, 8.0f);
}

static int32_t minimum_button_height(lv_obj_t *root, uint32_t *count)
{
    int32_t minimum = INT32_MAX;
    const uint32_t child_count = lv_obj_get_child_count(root);
    for (uint32_t index = 0; index < child_count; ++index) {
        lv_obj_t *child = lv_obj_get_child(root, (int32_t)index);
        if (lv_obj_check_type(child, &lv_button_class)) {
            const int32_t height = lv_obj_get_height(child);
            if (height < minimum) minimum = height;
            ++(*count);
        }
        const int32_t nested = minimum_button_height(child, count);
        if (nested < minimum) minimum = nested;
    }
    return minimum;
}

static bool run_ui_smoke_test(void)
{
    lv_obj_update_layout(ui_MainScreen);
    lv_obj_t *main_menu = lv_obj_get_child(ui_SettingsScreen, 0);
    if (!main_menu || lv_obj_get_child_count(main_menu) < 6) {
        fprintf(stderr, "UI smoke: settings rows missing\n");
        return false;
    }

    lv_obj_t *first = lv_obj_get_child(main_menu, 0);
    lv_obj_t *second = lv_obj_get_child(main_menu, 1);
    const int32_t first_height = lv_obj_get_height(first);
    const int32_t row_gap = lv_obj_get_y(second) -
                            (lv_obj_get_y(first) + first_height);
    const int32_t scroll_bottom = lv_obj_get_scroll_bottom(main_menu);
    if (first_height < 32 || row_gap < 8 || scroll_bottom <= 0) {
        fprintf(stderr,
                "UI smoke: settings geometry height=%ld gap=%ld scroll=%ld\n",
                (long)first_height, (long)row_gap, (long)scroll_bottom);
        return false;
    }


    int32_t nested_minimum = INT32_MAX;
    uint32_t nested_buttons = 0;
    for (SettingsMenu_t menu = SETTINGS_MENU_RADIO;
         menu <= SETTINGS_MENU_ABOUT; ++menu) {
        lv_obj_t *root = ui_settings_debug_menu_root(menu);
        if (!root) {
            fprintf(stderr, "UI smoke: Settings submenu %d missing\n", menu);
            return false;
        }
        const int32_t menu_minimum = minimum_button_height(root,
                                                            &nested_buttons);
        if (menu_minimum < nested_minimum) nested_minimum = menu_minimum;
    }
    if (nested_buttons < 15 || nested_minimum < 28) {
        fprintf(stderr,
                "UI smoke: nested controls count=%lu minimum height=%ld\n",
                (unsigned long)nested_buttons, (long)nested_minimum);
        return false;
    }

    for (SettingsMenu_t menu = SETTINGS_MENU_ROBOT;
         menu <= SETTINGS_MENU_ABOUT; ++menu) {
        lv_obj_t *root = ui_settings_debug_menu_root(menu);
        lv_obj_t *body = root && lv_obj_get_child_count(root) > 1
                             ? lv_obj_get_child(root, 1)
                             : NULL;
        if (!body || !lv_obj_has_flag(body, LV_OBJ_FLAG_SCROLLABLE) ||
            lv_obj_get_y(body) < 30 ||
            lv_obj_get_y(body) + lv_obj_get_height(body) > UI_CONTENT_HEIGHT) {
            fprintf(stderr, "UI smoke: submenu %d body is not scroll-safe\n",
                    menu);
            return false;
        }
    }

    lv_obj_t *gimbal_body = lv_obj_get_child(
        ui_settings_debug_menu_root(SETTINGS_MENU_GIMBAL_CAL), 1);
    lv_obj_t *gimbal_actions = lv_obj_get_child(gimbal_body, 3);
    lv_obj_t *pot_body = lv_obj_get_child(
        ui_settings_debug_menu_root(SETTINGS_MENU_POT_CAL), 1);
    lv_obj_t *pot_actions = lv_obj_get_child(pot_body, 2);
    lv_obj_t *action_slots[] = {gimbal_actions, pot_actions};
    for (uint32_t index = 0; index < 2; ++index) {
        lv_obj_t *slot = action_slots[index];
        if (!slot || lv_obj_get_child_count(slot) != 2) {
            fprintf(stderr, "UI smoke: calibration action slot missing\n");
            return false;
        }
        lv_obj_t *start = lv_obj_get_child(slot, 0);
        lv_obj_t *record = lv_obj_get_child(slot, 1);
        if (lv_obj_get_height(start) < 32 ||
            lv_obj_get_x(start) != lv_obj_get_x(record) ||
            lv_obj_get_y(start) != lv_obj_get_y(record) ||
            lv_obj_get_width(start) != lv_obj_get_width(record) ||
            lv_obj_get_height(start) != lv_obj_get_height(record)) {
            fprintf(stderr, "UI smoke: calibration actions shift position\n");
            return false;
        }
    }

    lv_obj_t *radio = ui_settings_debug_menu_root(SETTINGS_MENU_RADIO);
    lv_obj_t *radio_header = radio ? lv_obj_get_child(radio, 0) : NULL;
    lv_obj_t *back = radio_header ? lv_obj_get_child(radio_header, 0) : NULL;
    if (!back || lv_obj_get_x(back) < 6 || lv_obj_get_y(back) < 2 ||
        lv_obj_get_x(back) + lv_obj_get_width(back) >=
            lv_obj_get_width(radio_header)) {
        fprintf(stderr, "UI smoke: Back button lacks outer padding\n");
        return false;
    }

    lv_obj_t *radio_bar = radio ? lv_obj_get_child(radio, 1) : NULL;
    if (!radio_bar) {
        fprintf(stderr, "UI smoke: Radio toolbar missing\n");
        return false;
    }
    const uint32_t toolbar_children = lv_obj_get_child_count(radio_bar);
    lv_area_t toolbar_area;
    lv_obj_get_coords(radio_bar, &toolbar_area);
    for (uint32_t index = 0; index < toolbar_children; ++index) {
        lv_obj_t *control = lv_obj_get_child(radio_bar, (int32_t)index);
        if (!lv_obj_check_type(control, &lv_button_class)) continue;
        lv_area_t control_area;
        lv_obj_get_coords(control, &control_area);
        if (control_area.y1 - toolbar_area.y1 < 2 ||
            toolbar_area.y2 - control_area.y2 < 2) {
            fprintf(stderr,
                    "UI smoke: Radio toolbar control %lu clipped "
                    "(top=%ld bottom=%ld)\n",
                    (unsigned long)index,
                    (long)(control_area.y1 - toolbar_area.y1),
                    (long)(toolbar_area.y2 - control_area.y2));
            return false;
        }
    }

    lv_obj_t *param_list = radio ? lv_obj_get_child(radio, 2) : NULL;
    lv_obj_t *first_param = param_list && lv_obj_get_child_count(param_list) > 0
                                ? lv_obj_get_child(param_list, 0)
                                : NULL;
    const int32_t param_height = first_param ? lv_obj_get_height(first_param) : 0;
    const int32_t param_gap = param_list
                                  ? lv_obj_get_style_pad_row(param_list,
                                                             LV_PART_MAIN)
                                  : 0;
    if (param_height < 30 || param_gap < 8) {
        fprintf(stderr,
                "UI smoke: Radio row height=%ld gap=%ld\n",
                (long)param_height, (long)param_gap);
        return false;
    }
    if (!lv_obj_has_flag(param_list, LV_OBJ_FLAG_SCROLLABLE) ||
        lv_obj_get_y(param_list) + lv_obj_get_height(param_list) >
            UI_CONTENT_HEIGHT) {
        fprintf(stderr, "UI smoke: Radio list is not scroll-safe\n");
        return false;
    }

    Settings_SetRobotProfile(ROBOT_PROFILE_HEXAPOD);
    ui_telemetry_refresh_for_profile(ROBOT_PROFILE_HEXAPOD);
    lv_obj_update_layout(ui_MainScreen);
    lv_obj_t *panel_container = lv_obj_get_child(ui_TelemetryScreen, 0);
    const uint32_t page_count = panel_container
                                    ? lv_obj_get_child_count(panel_container)
                                    : 0;
    if (page_count != 4) {
        fprintf(stderr, "UI smoke: expected 4 Hexapod pages, got %lu\n",
                (unsigned long)page_count);
        return false;
    }
    update_simulated_hexapod();
        printf("UI smoke: settings height=%ld gap=%ld scroll=%ld, "
            "nested min=%ld, Radio row=%ld/%ld, pages=%lu\n",
           (long)first_height, (long)row_gap, (long)scroll_bottom,
            (long)nested_minimum, (long)param_height, (long)param_gap,
           (unsigned long)page_count);
    return true;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   LVGL Simulator - RC Remote Controller    ║\n");
    printf("║   Resolution: %dx%d                      ║\n", DISP_HOR_RES, DISP_VER_RES);
    printf("║   Mouse = Touch Input, ESC = Quit          ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "Error: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    /* Initialize LVGL */
    lv_init();
    
    /* Set up tick function */
    lv_tick_set_cb(get_tick_ms);
    
    /* Initialize SDL display driver */
    lv_sdl_display_init();
    
    /* Initialize SDL mouse (touch emulation) */
    lv_sdl_mouse_init();
    
    /* Initialize settings (stub) */
    Settings_Init();
    
    /* Initialize ELRS client against the simulated ES24TX Pro module */
    elrs_client_init(elrs_sim_send_frame, NULL);
    
    /* Initialize UI */
    printf("Initializing UI...\n");
    fflush(stdout);
    ui_init();
    printf("UI initialized successfully!\n\n");
    fflush(stdout);

    if (argc > 1 && strcmp(argv[1], "--smoke-test") == 0) {
        const bool passed = run_ui_smoke_test();
        ui_destroy();
        SDL_Quit();
        return passed ? 0 : 1;
    }
    
    /* Force initial screen refresh */
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(lv_display_get_default());
    
    /* Main loop */
    printf("Running... Press ESC or close window to exit.\n");
    printf("  During gimbal calibration:\n");
    printf("    Arrow UP/DOWN = Adjust gimbal value\n");
    printf("    HOME = Center (2048), END = Min (100), PAGEUP = Max (3995)\n");
    while (!lv_sdl_should_quit()) {
        /* Poll SDL events */
        lv_sdl_poll_events();
        
        /* Update simulated gimbals for calibration */
        update_simulated_gimbals();
        update_simulated_hexapod();
        
        /* Deliver simulated ELRS module responses */
        elrs_sim_poll(SDL_GetTicks());
        
        /* Handle LVGL tasks */
        uint32_t time_till_next = lv_timer_handler();
        
        /* Sleep to reduce CPU usage */
        if (time_till_next > 0) {
            SDL_Delay(time_till_next < TICK_PERIOD_MS ? time_till_next : TICK_PERIOD_MS);
        }
    }
    
    /* Cleanup */
    printf("\nShutting down...\n");
    ui_destroy();
    SDL_Quit();
    
    printf("Goodbye!\n");
    return 0;
}
