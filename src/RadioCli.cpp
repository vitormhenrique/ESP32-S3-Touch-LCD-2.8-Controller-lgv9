#include <Arduino.h>
#include <string.h>

#include "RadioCli.h"
#include "elrs/crsf_protocol.h"
#include "elrs/elrs_service.h"

namespace {

char line[96];
size_t line_length = 0;
bool discovery_started = false;
uint8_t pending_write_id = 0;

void print_switch_mode()
{
    const ElrsParam *parameter = elrs_service_find("Switch Mode");
    if (!parameter) {
        Serial.println("RADIO SWITCH_MODE unavailable; TX parameters are not loaded.");
        return;
    }

    char current[48] = "?";
    if (parameter->value >= 0) {
        elrs_param_option(parameter, parameter->value, current, sizeof(current));
    }
    Serial.printf("RADIO SWITCH_MODE id=%u current=%s options=",
                  parameter->id, current);
    const int option_count = elrs_param_option_count(parameter);
    for (int index = 0; index < option_count; ++index) {
        char option[48];
        if (!elrs_param_option(parameter, index, option, sizeof(option))) continue;
        Serial.printf("%s%s", index == 0 ? "" : ";", option);
    }
    Serial.println();
}

void print_help()
{
    Serial.println("RADIO commands:");
    Serial.println("  radio refresh");
    Serial.println("  radio switch");
    Serial.println("  radio get switch");
    Serial.println("  radio set switch <exact discovered option>");
}

void process_line()
{
    if (strcmp(line, "radio help") == 0) {
        print_help();
        return;
    }
    if (strcmp(line, "radio refresh") == 0) {
        discovery_started = true;
        elrs_client_select_device(CRSF_ADDR_TX_MODULE);
        Serial.println("RADIO refresh requested");
        return;
    }
    if (strcmp(line, "radio switch") == 0 ||
        strcmp(line, "radio get switch") == 0) {
        print_switch_mode();
        return;
    }

    static const char prefix[] = "radio set switch ";
    if (strncmp(line, prefix, sizeof(prefix) - 1) == 0) {
        const char *option = line + sizeof(prefix) - 1;
        const ElrsParam *parameter = elrs_service_find("Switch Mode");
        if (!parameter) {
            Serial.println("RADIO ERROR: TX parameters are not loaded; run radio refresh.");
            return;
        }
        pending_write_id = parameter->id;
        if (elrs_service_set("Switch Mode", option)) {
            Serial.printf("RADIO SWITCH_MODE write queued: %s\n", option);
        } else {
            pending_write_id = 0;
            Serial.printf("RADIO ERROR: rejected option: %s\n", option);
            print_switch_mode();
        }
        return;
    }

    Serial.println("RADIO ERROR: unknown command; use radio help");
}

}  // namespace

void RadioCli_Init()
{
    Serial.println("RADIO CLI ready; use 'radio help'.");
}

void RadioCli_Poll()
{
    // Settings discovery was previously tied to opening the Radio UI. Start it
    // after USB and the CRSF task have had time to initialize.
    if (!discovery_started && millis() > 3000) {
        discovery_started = true;
        elrs_client_select_device(CRSF_ADDR_TX_MODULE);
    }

    while (Serial.available() > 0) {
        const char character = static_cast<char>(Serial.read());
        if (character == '\r') continue;
        if (character == '\n') {
            line[line_length] = '\0';
            if (line_length > 0) process_line();
            line_length = 0;
            continue;
        }
        if (line_length + 1 < sizeof(line)) {
            line[line_length++] = character;
        } else {
            line_length = 0;
            Serial.println("RADIO ERROR: command too long");
        }
    }
}

void RadioCli_OnElrsEvent(ElrsClientEvent event, uint8_t parameter_id)
{
    if (event == ELRS_EV_PARAMS_LOADED &&
        elrs_client_selected_device() == CRSF_ADDR_TX_MODULE) {
    }
    if (parameter_id != pending_write_id) return;
    if (event == ELRS_EV_WRITE_VERIFIED) {
        Serial.println("RADIO SWITCH_MODE VERIFIED");
        pending_write_id = 0;
        print_switch_mode();
    } else if (event == ELRS_EV_WRITE_FAILED) {
        Serial.println("RADIO SWITCH_MODE FAILED");
        pending_write_id = 0;
        print_switch_mode();
    }
}