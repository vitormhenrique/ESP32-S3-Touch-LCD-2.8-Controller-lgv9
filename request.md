
3) on the telemetry screen, implement the same horizontal scroll, and the configuration for it will depend on what robot is loaded, but all robots should have the status screen, another panel with a rolling log, implement a screen for an hexapod using DYNAMIXEL MX-28AR servos, one panel should display servo information, Another panel should show robot IMU data, virtual horizon, yaw, pitch, row, and any other information that is relevant for an hexapod. 
4) Change the setup screen title to a Config the Config screen will have a lto of buttons that will display basic settings for the radio. All configuration should be saved on ESP memory and survive reboots.
5) Implement A screen calibration on the config, where the user should click 4 parts of the screen and display map should use that.
6) Implement a robot selection screen, add the hexapod and a generic robot profile.
7) Implement a gimbal calibration screen.
8) Change current inputs on my InputConfig as follows

Gimbal 2 X is on ADS1115 0x49 input A0
Gimbal 2 y is on ADS1115 0x49 input A1
Gimbal 1 y is on ADS1115 0x48 input A1
Gimbal 1 X is on ADS1115 0x48 input A0

Navigation switch 2:
Left is on MCP23017 address 0x20 input A5
Down is on MCP23017 address 0x20  input A6
Right is on MCP23017 address 0x20  input A3
Up is on MCP23017 address 0x20  input A4
Center is on MCP23017 address 0x20  input A0

Endoder 2
A is on MCP23017 address 0x20  input A1
B is on MCP23017 address 0x20  input A2


Navigation switch 1:
Left is on MCP23017 address 0x21 input B5
Down is on MCP23017 address 0x21  input B6
Right is on MCP23017 address 0x21  input B3
Up is on MCP23017 address 0x21  input B4
Center is on MCP23017 address 0x21  input B0

Endoder 1
A is on MCP23017 address 0x21  input B1
B is on MCP23017 address 0x21  input B2

Button 1 on MCP23017 address 0x21 input B7
Button 2 on MCP23017 address 0x20 input A7

Assign the other inputs into non used inputs on MCP23017 and ADS1115

9) Implement a debug flag at compilation time, if that is true, add some metrics on the code that we measure loop performance on how often the screen is updating (fps) how often the main loop is running and how fast is the input loop running, every 5 seconds print the average values and any other good performance metric.

10) Divide the UI into multiple files, one for each screen, exporting components that can be reused into a ui library, so the code is reusable and follow best practices.

11) make sure that the code can compile, and is working