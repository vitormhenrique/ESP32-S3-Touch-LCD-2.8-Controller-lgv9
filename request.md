
3) on the telemetry screen, implement the same horizontal scroll, and the configuration for it will depend on what robot is loaded, but all robots should have the status screen, another panel with a rolling log, implement a screen for an hexapod using DYNAMIXEL MX-28AR servos, one panel should display servo information, Another panel should show robot IMU data, virtual horizon, yaw, pitch, row, and any other information that is relevant for an hexapod. 
4) Change the setup screen title to a Config the Config screen will have a lto of buttons that will display basic settings for the radio. All configuration should be saved on ESP memory and survive reboots.
5) Implement A screen calibration on the config, where the user should click 4 parts of the screen and display map should use that.
6) Implement a robot selection screen, add the hexapod and a generic robot profile.
7) Implement a gimbal calibration screen.

9) Implement a debug flag at compilation time, if that is true, add some metrics on the code that we measure loop performance on how often the screen is updating (fps) how often the main loop is running and how fast is the input loop running, every 5 seconds print the average values and any other good performance metric.

11) make sure that the code can compile, and is working