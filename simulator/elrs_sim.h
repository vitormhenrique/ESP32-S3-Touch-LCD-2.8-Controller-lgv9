/**
 * @file elrs_sim.h
 * Simulated ExpressLRS ES24TX Pro TX module + receiver for the desktop
 * simulator. Acts as the transport for elrs_client.
 */
#ifndef ELRS_SIM_H
#define ELRS_SIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ElrsSendFn-compatible transport: give this to elrs_client_init(). */
void elrs_sim_send_frame(uint8_t frame_type, const uint8_t *payload,
                         uint8_t len, void *user);

/** Deliver delayed responses / advance command scripts. Call every loop. */
void elrs_sim_poll(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // ELRS_SIM_H
