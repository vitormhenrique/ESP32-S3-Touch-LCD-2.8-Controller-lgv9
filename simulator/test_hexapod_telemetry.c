#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "telemetry/hexapod_telemetry.h"

static void test_decodes_version_one_status(void)
{
    const uint8_t payload[HEXAPOD_STATUS_PAYLOAD_SIZE] = {
        HEXAPOD_TELEMETRY_MAGIC_0, HEXAPOD_TELEMETRY_MAGIC_1,
        HEXAPOD_TELEMETRY_VERSION,
        HEXAPOD_FLAG_ARMED | HEXAPOD_FLAG_MOTION_GATE |
            HEXAPOD_FLAG_IMU_PRESENT | HEXAPOD_FLAG_IMU_FRESH,
        5, 1, 2, 0, 0, 191, 153, 0xDB,
        0x2E, 0xE0, 0x00, 0x28, 0x00, 0x3C, 0x00, 0x1E,
    };
    HexapodTelemetryStatus status;

    assert(hexapod_telemetry_decode(payload, sizeof(payload), &status));
    assert(status.safety_state == 5);
    assert(status.command_source == 1);
    assert(status.gait == 2);
    assert(status.control_mode == 0);
    assert(status.speed_x255 == 191);
    assert(status.duty_x255 == 153);
    assert(status.imu_calibration == 0xDB);
    assert(status.battery_mv == 12000);
    assert(status.body_height_mm == 40);
    assert(status.stride_mm == 60);
    assert(status.step_height_mm == 30);
    assert(strcmp(hexapod_safety_state_name(status.safety_state), "RC Manual") == 0);
    assert(strcmp(hexapod_gait_name(status.gait), "Tripod") == 0);
    assert(strcmp(hexapod_gait_name(0), "None") == 0);
    assert(strcmp(hexapod_gait_name(1), "None") == 0);
    assert(hexapod_telemetry_is_fresh(true, 1999u, 0u));
    assert(!hexapod_telemetry_is_fresh(true, 2000u, 0u));
    assert(!hexapod_telemetry_is_fresh(false, 1u, 0u));
    assert(hexapod_telemetry_is_fresh(true, 1000u, UINT32_MAX - 499u));
}

static void test_rejects_malformed_status(void)
{
    uint8_t payload[HEXAPOD_STATUS_PAYLOAD_SIZE] = {0};
    HexapodTelemetryStatus status;

    payload[0] = HEXAPOD_TELEMETRY_MAGIC_0;
    payload[1] = HEXAPOD_TELEMETRY_MAGIC_1;
    payload[2] = HEXAPOD_TELEMETRY_VERSION;
    assert(!hexapod_telemetry_decode(payload, sizeof(payload) - 1, &status));

    payload[2]++;
    assert(!hexapod_telemetry_decode(payload, sizeof(payload), &status));

    payload[2] = HEXAPOD_TELEMETRY_VERSION;
    payload[6] = 6;
    assert(!hexapod_telemetry_decode(payload, sizeof(payload), &status));

    assert(!hexapod_telemetry_decode(NULL, sizeof(payload), &status));
    assert(!hexapod_telemetry_decode(payload, sizeof(payload), NULL));
}

int main(void)
{
    test_decodes_version_one_status();
    test_rejects_malformed_status();
    puts("hexapod telemetry tests passed");
    return 0;
}
