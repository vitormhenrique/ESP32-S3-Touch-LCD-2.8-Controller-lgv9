#include "InputSim.h"

#if UI_INPUT_SIM

static input_sim_state_t s_sim = {
    .gimbal  = {0, 0, 0, 0},
    .sw      = {false, false, false, false, false, false},
    .btn     = {false, false, false, false},
    .toggle3 = {1, 1},  // center
};

const input_sim_state_t* input_sim_get(void)
{
    return &s_sim;
}

void input_sim_set_gimbal(uint8_t axis, int16_t value)
{
    if (axis >= 4) return;
    if (value < -1000) value = -1000;
    if (value > 1000) value = 1000;
    s_sim.gimbal[axis] = value;
}

void input_sim_set_button(uint8_t index, bool pressed)
{
    if (index >= 4) return;
    s_sim.btn[index] = pressed;
}

void input_sim_toggle_switch(uint8_t index)
{
    if (index >= 6) return;
    s_sim.sw[index] = !s_sim.sw[index];
}

void input_sim_cycle_toggle3(uint8_t index)
{
    if (index >= 2) return;
    s_sim.toggle3[index] = (uint8_t)((s_sim.toggle3[index] + 1) % 3);
}

#endif // UI_INPUT_SIM
