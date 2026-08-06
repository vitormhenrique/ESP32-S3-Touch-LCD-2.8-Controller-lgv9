# Hexapod Controller Map

This is the default physical control map sent by the ESP32-S3 controller to the
OpenRB-150 through ChannelPack CRSF. The OpenRB firmware remains the final
safety authority: feature switches request behavior but cannot bypass arming,
E-stop, authority, capability, reach, or servo limits.

## Gimbals And Adjustments

The gimbals have fixed jobs. The **left gimbal always walks**; the **right
gimbal turns the robot or moves the body**. `SW_E` changes how the left gimbal
walks (gait pattern) and `SW_F` changes what the right gimbal does. Body
translation and rotation therefore need no special gait — the selected gait
keeps running in every `SW_F` position.

| Physical control | Robot behavior |
| --- | --- |
| Left gimbal X | Strafe left/right, in every `SW_F` position |
| Left gimbal Y | Walk forward/backward, in every `SW_F` position |
| Right gimbal X, `SW_F` UP | Turn the robot left/right |
| Right gimbal Y, `SW_F` UP | Unassigned |
| Right gimbal X/Y, `SW_F` CENTER | Shift body Y/X while the left gimbal keeps walking |
| Right gimbal X/Y, `SW_F` DOWN | Body roll/pitch while the left gimbal keeps walking |
| Pot 1 | Gait cadence and torque-enable recovery speed, 0..100% |
| Pot 2 | Body height, 65..150 mm; center is 132 mm. Fully down is the belly-near-ground crouch |
| Encoder 1 | Unassigned |
| Encoder 2 | Unassigned |

Turning is available only in `SW_F` UP: the two body modes use both right-hand
axes for the pose overlay.

Stride and step lift moved off the encoders onto the NAV1 gait-tune editor (see
below), so both encoders are free. The encoder A/B pins are not additional
robot buttons.

## Switches

| Switch | Type / positions | Robot behavior |
| --- | --- | --- |
| `SW_A` | 2-position, ON | Request arm; ignored while kill/failsafe is active |
| `SW_B` | 2-position, ON | Immediate E-stop/kill and disarm request |
| `SW_C` | 2-position, ON | Request foot-contact detection |
| `SW_D` | 2-position, ON | Request terrain leveling |
| `SW_E` | 3-position UP/CENTER/DOWN | **Left gimbal walk pattern**: wave / ripple / tripod |
| `SW_F` | 3-position UP/CENTER/DOWN | **Right gimbal job**: turn robot / translate body / rotate body |
| `SW_G` | 2-position, ON | Request torque-off passive-pose streaming |
| `SW_H` | 2-position, ON | Hand motion authority to USB host or Jetson |

`SW_A`, `SW_B`, and the feature switches are levels, not one-shot buttons.
`SW_B` and CRSF failsafe always override every other command source.

## Buttons And Navigation

Buttons and nav actions trigger on the press edge with a 150 ms refractory
window. Moving a gait/body stick cancels an active choreography.

| Input | Robot behavior |
| --- | --- |
| `BTN_1` | Stand-up choreography |
| `BTN_2` | Sit-down choreography |
| `BTN_3` | Standing body-rock wave choreography |
| `BTN_4` | Toggle crouched/tall stance |
| `NAV1 Up` | Add 1 degree pitch trim / next gait parameter while tuning |
| `NAV1 Down` | Subtract 1 degree pitch trim / previous gait parameter while tuning |
| `NAV1 Left` | Add 1 degree left roll trim / decrease the parameter while tuning |
| `NAV1 Right` | Add 1 degree right roll trim / increase the parameter while tuning |
| `NAV1 Center` | Reset roll and pitch trim / save gait settings while tuning |
| `NAV2 Up` | Twirl in place |
| `NAV2 Down` | Stretch/push-up sequence |
| `NAV2 Left` | Hold lean/look pose until cancelled |
| `NAV2 Right` | Engage / leave the gait-tune editor |
| `NAV2 Center` | Loop dance until stick input cancels it |

## Gait Tune Editor

`NAV2 Right` toggles the editor. While it is engaged NAV1 edits gait shape
instead of pose trim, cycling step height -> stride -> duty and changing the
selected value by 5% of its safe range per press. `NAV1 Center` persists the
result to the robot's 24LC32 config; the robot refuses to write EEPROM while it
is walking and reports the reason on the telemetry `Error` row.

With the robot armed and the sticks centred, leg 1 traces the swing arc once
every 2 seconds using the live values, so a change is visible on the robot
itself. Moving a stick cancels the preview immediately.

The robot raises its status downlink from 5 Hz to 20 Hz for 3 seconds after
every edit, so the `Tuning` row on the Hexapod telemetry page tracks the knob
without lag.

## Hardware Input Assignment

These names are defined in `src/InputConfig.h` and packed in
`src/CRSF_Manager.cpp`.

| Input | Device and pin/channel |
| --- | --- |
| `SW_A` | MCP23017 `0x21` GPA5 |
| `SW_B` | MCP23017 `0x20` GPB2 |
| `SW_C` | MCP23017 `0x21` GPA0 |
| `SW_D` | MCP23017 `0x21` GPA3 |
| `SW_G` | MCP23017 `0x21` GPA4 |
| `SW_H` | MCP23017 `0x20` GPB3 |
| `SW_E` UP/DOWN | MCP23017 `0x21` GPA6/GPA7 |
| `SW_F` UP/DOWN | MCP23017 `0x20` GPB1/GPB0 |
| `BTN_1` | MCP23017 `0x21` GPB7 |
| `BTN_2` | MCP23017 `0x20` GPA7 |
| `BTN_3` | MCP23017 `0x21` GPA1 |
| `BTN_4` | MCP23017 `0x21` GPA2 |
| `NAV1` U/D/L/R/C | MCP23017 `0x21` GPB4/GPB6/GPB5/GPB3/GPB0 |
| `NAV2` U/D/L/R/C | MCP23017 `0x20` GPA4/GPA6/GPA5/GPA3/GPA0 |
| Encoder 1 A/B | MCP23017 `0x21` GPB1/GPB2 |
| Encoder 2 A/B | MCP23017 `0x20` GPA1/GPA2 |
| Left gimbal X/Y | ADS1115 `0x48` A1/A0 |
| Right gimbal X/Y | ADS1115 `0x49` A1/A0 |
| Pot 1/2 | ADS1115 `0x49` A3/A2 |

## Wire Packing

| Channel group | Packed values |
| --- | --- |
| CH1..CH4 | Left X, Left Y, Right X, Right Y |
| CH5..CH6 | Pot 1, Pot 2 |
| CH7..CH8 | Encoder 1, Encoder 2 |
| CH9 | `SW_A`, `SW_B`, `SW_C`, `SW_D`, `SW_G`, `SW_H` compact mask |
| CH10 | `BTN_1..4`, `SW_E`, `SW_F` compact state |
| CH11 | `NAV1`, `NAV2` compact state |
| CH12..CH16 | Reserved/centered |

The controller must use an ELRS full 16-channel switch mode. Modes that drop
CH9..CH16 remove safety switches, buttons, toggles, and nav controls.
