# LVGL Simulator for RC Remote Controller

Desktop simulator for the ESP32-S3 RC Remote Controller UI using SDL2.

## Features

- Runs the exact same LVGL UI code as the microcontroller
- Mouse input simulates touch screen
- Same 320x240 resolution (2x scaled for visibility)
- Fast iteration - no need to flash the microcontroller

## Requirements

- macOS (or Linux with minor modifications)
- [just](https://github.com/casey/just) command runner (`brew install just`)
- SDL2 library (automatically installed via Homebrew on macOS)
- CMake 3.14+
- C/C++ compiler

## Building & Running

```bash
# From the simulator directory:
just          # Build and run (default)
just build    # Build only
just run      # Build and run
just quick    # Quick rebuild and run (no cmake reconfigure)
just clean    # Remove build artifacts
just rebuild  # Clean rebuild from scratch
```

Run `just --list` to see all available commands.

## Controls

- **Mouse**: Simulates touch input
- **ESC** or **Close Window**: Exit simulator

## Project Structure

```
simulator/
├── justfile         # Just command runner recipes
├── CMakeLists.txt   # CMake configuration
├── lv_conf.h        # LVGL configuration for simulator
├── main.c           # Simulator main loop (SDL init, LVGL tick)
├── lv_drv_sdl.c     # SDL display and input drivers
├── hal_stubs.c      # Hardware abstraction stubs
├── Settings_stub.c  # Settings API stub (in-memory)
└── README.md        # This file
```

## How It Works

1. **Display**: SDL2 creates a window and texture. LVGL renders to a buffer, which is copied to the SDL texture and displayed.

2. **Input**: SDL2 mouse events are captured and converted to LVGL touch input (pointer device).

3. **Tick**: SDL2's `SDL_GetTicks()` provides the timing for LVGL's tick system.

4. **UI Code**: The actual UI source files from `src/ui/` are compiled directly into the simulator, so any changes to the UI are reflected in both the simulator and microcontroller builds.

## Rebuilding After UI Changes

After modifying any file in `src/ui/`:

```bash
cd simulator
just quick    # Fast incremental rebuild
```

Or for a full rebuild:

```bash
just rebuild
```

## Differences from Microcontroller

- No actual hardware (gimbals, switches, radio, etc.)
- Settings are in-memory only (reset on restart)
- Higher memory available
- No ESP32-specific features

## Troubleshooting

### SDL2 not found
```bash
brew install sdl2 pkg-config
```

### Black screen
Check that LVGL is properly initialized and `ui_init()` is called.

### Touch not working
Make sure the mouse is within the window bounds.
