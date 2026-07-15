#pragma once

#include "elrs/elrs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

void RadioCli_Init();
void RadioCli_Poll();
void RadioCli_OnElrsEvent(ElrsClientEvent event, uint8_t parameter_id);

#ifdef __cplusplus
}
#endif