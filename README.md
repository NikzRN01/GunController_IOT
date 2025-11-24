# GunController IoT Project

A simple ESP32 + sensor + Python bridge setup that turns a physical "gun" controller (joystick, trigger, reload button, vibration motor, and MPU6050 IMU) into PC game inputs. The ESP32 exposes real‑time state (joystick axes, gyroscope, trigger, reload, ammo) over a lightweight HTTP JSON endpoint on its own Wi‑Fi Access Point. A Python script (`bridge_code.py`) polls the ESP32 and converts sensor values into keyboard (WASD) and mouse actions for first‑person or similar games.

---
## Features
- ESP32 acts as a Wi‑Fi Access Point (`GunController`) and HTTP server (port 80).
- Joystick mapped to `W`, `A`, `S`, `D` with a configurable dead zone.
- Gyroscope (MPU6050) drives relative mouse movement for aiming.
- Trigger button fires (left mouse click) and activates a vibration motor for haptic feedback.
- Reload button sends keyboard `R` and refills ammo on the microcontroller.
- Simple JSON protocol for extensibility.

---
## Repository Contents
| File | Purpose |
|------|---------|
| `guncontroller.ino` | ESP32 firmware: Wi‑Fi AP, sensor read, JSON output, haptics, ammo logic. |
| `bridge_code.py` | Python PC bridge: polls JSON, generates keyboard/mouse events with `pyautogui`. |

---
## Hardware Components
- ESP32 development board (with native Wi‑Fi).
- MPU6050 IMU (I²C accelerometer + gyroscope).
- 2‑axis analog joystick (VRx/VRy to ADC pins; +5V or 3.3V depending on module, GND).
- Trigger button (momentary, wired to `GPIO18` with internal pull‑up).
- Reload button (momentary, wired to `GPIO19` with internal pull‑up).
- Vibration motor + suitable transistor/MOSFET + diode (or a driver board) on `GPIO25` (HIGH = on).
- Optional: small LiPo or USB power source.

---
## Pin Mapping (ESP32)
| Function | GPIO | Notes |
|----------|------|-------|
| Joystick X | 32 | Analog input (ADC). |
| Joystick Y | 33 | Analog input (ADC). |
| Trigger Button | 18 | `INPUT_PULLUP` (active LOW). |
| Reload Button | 19 | `INPUT_PULLUP` (active LOW). |
| Vibration Motor | 25 | Digital output (drive transistor). |
| I²C SDA (MPU6050) | 21 | Default ESP32 SDA. |
| I²C SCL (MPU6050) | 22 | Default ESP32 SCL. |

Verify joystick supply voltage matches board tolerance; if module uses 5V, confirm output levels are safe for ADC or use level shifting.

---
## Firmware Behavior (`guncontroller.ino`)
1. Initializes Serial, I²C, MPU6050, pin modes.
2. Starts a Wi‑Fi SoftAP with SSID `GunController` and password `12345678`. (Comment says "No password" but currently a password is set.)
3. Each loop (~20 Hz):
   - Reads joystick (`analogRead`), IMU motion (`getMotion6`).
   - Reads trigger and reload digital inputs.
   - If trigger pressed and `ammo > 0`: runs `fireGun()` (motor buzz for 200 ms, decrement ammo).
   - If reload pressed: `ammo = MAX_AMMO`.
   - Assembles JSON: `{joyX, joyY, gx, gy, gz, trigger, reload, ammo}`.
   - If an HTTP client is connected, responds with JSON (CORS open) then closes.
4. `fireGun()` drives `VIBRATION_PIN` HIGH briefly.

---
## Python Bridge Behavior (`bridge_code.py`)
1. Polls `http://192.168.4.1/` every loop (timeout 1 s). Adjust IP if AP IP differs.
2. Parses JSON and:
   - Maps joystick displacement from center to WASD key presses (`keyDown`/`keyUp`). Dead zone ±200 around center.
   - Converts gyroscope `gx`, `gy` to relative mouse movement (`pyautogui.moveRel`) scaled by `SENSITIVITY` (default 0.1).
   - Detects trigger edge (0→1) to send `pyautogui.click()`.
   - Detects reload edge (0→1) to send `pyautogui.press('r')`.
3. Prints current ammo and joystick raw values for debugging.

`CENTER_JOY` (2048) assumes a 12‑bit ADC mid-point on ESP32 (0–4095). Adjust if your joystick sits off‑center.

---
## Setup Instructions
### 1. Hardware Assembly
- Wire joystick X/Y outputs to `GPIO32`, `GPIO33`; VCC to 3.3V (preferred); GND to GND.
- Connect trigger button between `GPIO18` and GND (internal pull-up active).
- Connect reload button between `GPIO19` and GND.
- MPU6050: `VCC -> 3.3V`, `GND -> GND`, `SDA -> GPIO21`, `SCL -> GPIO22`.
- Vibration motor: Use a transistor (e.g., 2N2222 or MOSFET) with base/gate resistor; motor supply (5V or 3.3V), diode across leads; control pin from `GPIO25`.

### 2. Firmware Flash
- Open Arduino IDE (or PlatformIO).
- Install ESP32 board support via Boards Manager.
- Install `MPU6050` library if not already present.
- Load `guncontroller.ino`.
- Select correct COM port & board (e.g., ESP32 Dev Module).
- Upload sketch.
- Open Serial Monitor at 115200 baud: note printed AP IP (commonly `192.168.4.1`).

### 3. PC Environment
Optional virtual environment:
```powershell
python -m venv .venv
./.venv/Scripts/Activate.ps1
```
Install dependencies:
```powershell
pip install requests pyautogui
```
(You may need to allow screen control permissions on some OSes.)

### 4. Connect to Controller Wi‑Fi
- On your PC, join SSID `GunController`.
- Use password `12345678` (unless you remove it in firmware).
- Confirm you can ping `192.168.4.1`.

### 5. Adjust Python Script (If Needed)
- If AP IP differs, edit `ESP32_IP` in `bridge_code.py`.
- Tune `CENTER_JOY` after observing steady joystick values via Serial Monitor.
- Adjust `SENSITIVITY` for comfortable mouse aim speed.

### 6. Run Bridge Script
```powershell
python bridge_code.py
```
Keep the game window in focus; `pyautogui` sends inputs to the active window.

### 7. Play Interaction
- Aim: Physically rotate/move the controller (gyro drives cursor).
- Move: Push joystick; edges map to W/A/S/D; release returns to stop after dead zone.
- Fire: Pull trigger (button LOW) → vibration + left click (if ammo > 0).
- Reload: Press reload button → `R` key and ammo reset.
- Ammo Display: Console prints current ammo; consider overlaying later.

---
## Calibration & Tuning
| Parameter | Purpose | How to Tune |
|-----------|---------|------------|
| `CENTER_JOY` | Joystick neutral center | Read `joyX/joyY` at rest several times, average. |
| Dead zone (±200) | Prevent drift | Increase if small unintended movements occur. |
| `SENSITIVITY` | Mouse aim scaling | Raise for faster aim, lower for precision. |
| Vibration duration (200 ms) | Haptic feedback length | Edit `fireGun()` delay. |

---
## JSON Protocol (Example)
```json
{"joyX":2035,"joyY":1890,"gx":120,"gy":-45,"gz":300,"trigger":0,"reload":0,"ammo":7}
```
- `joyX`, `joyY`: Raw ADC (0–4095).
- `gx`, `gy`, `gz`: Raw gyro readings (LSB units from MPU6050).
- `trigger`, `reload`: 1 if pressed else 0.
- `ammo`: Remaining shots (0–10).

---
## Troubleshooting
| Issue | Possible Cause | Fix |
|-------|----------------|-----|
| No response / timeout | Wrong IP / not connected to AP | Check SSID, update `ESP32_IP`, ping. |
| Joystick drift | Center value inaccurate | Recalibrate `CENTER_JOY`, enlarge dead zone. |
| Mouse too fast/slow | Sensitivity mismatch | Adjust `SENSITIVITY`. |
| No vibration | Motor driver wiring | Verify transistor orientation, supply voltage. |
| Ammo never decreases | Trigger wiring | Ensure button pulls pin LOW when pressed. |
| Python errors about `pyautogui` | Missing permissions | Allow screen control / install lib properly. |

---
## Safety & Notes
- `pyautogui` can move your mouse unexpectedly; run with caution and close script (Ctrl+C) if behavior is erratic.
- Exposing an open AP can be a security risk; consider removing or strengthening the password.
- Use proper motor driving circuitry to avoid damaging the ESP32 GPIO.

---
## Possible Future Improvements
- Add web socket streaming for lower latency.
- Normalize gyro data to degrees/sec and apply smoothing filter.
- Implement in‑game overlay or ammo HUD via an additional Python window.
- Add burst fire / alternate fire modes.
- Provide calibration routine on startup.

---
## Quick Start Summary
1. Flash `guncontroller.ino` to ESP32.
2. Join Wi‑Fi `GunController` (password `12345678`).
3. Install Python deps: `pip install requests pyautogui`.
4. Run `python bridge_code.py`.
5. Focus game window and start playing.

Enjoy building and customizing your GunController!