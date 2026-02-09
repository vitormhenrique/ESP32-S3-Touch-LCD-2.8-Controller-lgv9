# RC Controller UI - Feature Documentation

---

## 🎛️ Input Screen

Displays real-time status of all physical controls on the RC controller.

### Main Components (Top Section)
Upper part
- **Two Gimbals** - Square panels with crosshair grid and cyan moving dot showing X/Y joystick position
- **Two 5-Way Navigation Switches** - Grid of 5 buttons (Up/Down/Left/Right/Center) that highlight blue when pressed
Scrollable Control Panel (2 pages with horizontal swipe)

Bottom part
**Page 1:**
- **4 Buttons (B1-B4)** - Rounded rectangles showing pressed/released state with blue glow effect when active
- **2 Three-Position Toggles (T1-T2)** - Vertical track with orange indicator that moves to up/center/down position
- **3 Switches (S1-S3)** - Pill-shaped indicators, green "ON" when active, gray "OFF" when inactive

**Page 2:**
- **3 Switches (S4-S6)** - Same style as S1-S3
- **2 Potentiometers (P1-P2)** - Horizontal progress bar with purple fill and percentage label (0-100%)
- **2 Encoders (E1-E2)** - Card with numeric value display showing current encoder count
Scrollable with snap behaviour

---

## 📊 Header Bar

Status bar showing system information (currently hidden to maximize display area).

### Components
- **Title Label** - Displays "RC Control" on the left
- **WiFi Icon** - Green WiFi symbol when connected, orange warning triangle when disconnected
- **Battery Icon** - Dynamic icon changing with charge level (full/3-4/half/1-4/empty)
- **Battery Percentage** - Numeric label showing exact charge (e.g., "78%")
- **Charging Indicator** - Blue lightning bolt icon when charging

### Battery Color Coding
- Green: Above 50%
- Orange: 10-50%
- Red: Below 10%

---

## 📡 Telemetry Screen

Multi-page display showing data from connected robot (4 pages with horizontal swipe).

### Page 1: Status
- **Signal Strength** - RSSI value with bar indicator
- **Latency** - Response time in milliseconds
- **Error Count** - Number of communication errors
- **Uptime** - Connection duration

### Page 2: Log
- **Terminal Display** - Scrollable dark panel with green monospace text showing system messages and events

### Page 3: Servo Status (Hexapod Only)
- **18 Servo Cards** - Grid organized by leg (L1-L3, R1-R3) with 3 joints each (Coxa/Femur/Tibia)
- **Per-Servo Info:**
  - Position value in degrees
  - Load percentage bar
  - Temperature with color warning (white normal, orange >45°C, red >60°C)

### Page 4: IMU/Orientation (Hexapod Only)
- **Virtual Horizon** - Circular display with crosshairs showing robot tilt
- **Attitude Readout** - Roll, Pitch, Yaw angles in degrees
- **Accelerometer Values** - X, Y, Z acceleration readings

---

## ⚙️ Config Screen

Scrollable settings menu with touch-friendly buttons.

### Menu Items
1. **WiFi** - Network settings with connection status sublabel
2. **Robot Profile** - Select robot type (Generic/Hexapod)
3. **Gimbal Calibration** - Calibrate joystick center and ranges
4. **Touch Calibration** - Calibrate touchscreen accuracy
5. **Display** - Screen brightness settings
6. **About** - Firmware version info

### Button Style
- Dark card background with icon on left
- White label text with gray sublabel
- Arrow indicator on right edge
- Full-width touch target

---

## 🧭 Navigation

### Bottom Tab Bar (3 tabs)
| Tab | Icon | Screen |
|-----|------|--------|
| 1 | ✏️ | Input Controls |
| 2 | 👁️ | Telemetry |
| 3 | ⚙️ | Config |

### Swipe Gestures
- **Input Screen** - Swipe left/right between control pages
- **Telemetry Screen** - Swipe left/right between data pages
- Pages snap into place when released

---

## 🤖 Robot Profiles

### Generic Robot
- Shows: Status page, Log page
- Hides: Servo page, IMU page

### Hexapod Robot
- Shows: All 4 telemetry pages
- Enables: 18-servo monitoring, orientation display

---

## 🎨 Visual Theme

### Colors
| Element | Color |
|---------|-------|
| Background | Very dark (#0D1117) |
| Panels | Dark gray (#161B22) |
| Cards | Medium gray (#21262D) |
| Primary/Active | Blue (#58A6FF) |
| Success/ON | Green (#3FB950) |
| Warning/Center | Orange (#D29922) |
| Error/OFF | Red (#F85149) |
| Gimbal Dot | Cyan (#39C5CF) |
| Potentiometer | Purple (#A371F7) |

### Typography
- White text on dark backgrounds
- Gray (#8B949E) for secondary labels
- 12-14px font sizes throughout
