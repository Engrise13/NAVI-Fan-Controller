# NAVI Fan Controller - Rewrite pending

<p align="left">
  <img src="https://img.shields.io/badge/C++-11+-00599C?style=flat-square&logo=c%2B%2B&logoColor=white" alt="C++ Standard" />
  <img src="https://img.shields.io/badge/Platform-Arduino-00979D?style=flat-square&logo=arduino&logoColor=white" alt="Platform Arduino" />
  <img src="https://img.shields.io/badge/Sensor-DHT22-informational?style=flat-square" alt="Sensor DHT22" />
  <img src="https://img.shields.io/badge/Display-LCD_1602_I2C-2ea44f?style=flat-square" alt="Display LCD 1602 I2C" />
  <img src="https://img.shields.io/badge/License-MIT-blue?style=flat-square" alt="License" />
</p>

Microcontroller-based environmental cooling and climate automation system. Ingests ambient telemetry via a calibrated DHT22 sensor, executes closed-loop hysteresis control or manual relay override, and provides non-blocking status rendering across an I2C-driven 1602 character display.

---

## Key Features

- **Decoupled Telemetry Pipeline:** Non-blocking 2000 ms periodic sampling loop managed via `millis()` tracking to prevent timing jitter.
- **Dual Operating Profiles:** Supports raw power toggle (manual bypass) and automated closed-loop thermostatic control with integrated threshold hysteresis.
- **Edge-Filtered IR Demodulation:** Decodes single-pulse NEC/extended IR trigger frames while discarding hardware-level repeat flags (`IRDATA_FLAGS_IS_REPEAT`).
- **Flicker-Free Display Driver:** Uses an in-memory space-padded row buffer pipeline (`printPadded`) instead of destructive bus-clearing (`lcd.clear()`).
- **Deterministic Fallback Handling:** Explicit hardware shutoff and telemetry cache invalidation (`NAN`) upon standby state entry.

---

## Technical Architecture

```text
       [IR Remote (NEC/Raw)]
                 │
                 ▼
            handleIR()           --> Validates frame, drops repeat flags, toggles fanMode
                 │
                 ├── [fanMode == false] ──────────────┐
                 │                                    │
                 ▼                                    ▼
             readDHT()            --> 2000 ms polling, updates temp & humid
                 │                                    │
                 ▼                                    │
        handleTempControl()       --> Hysteresis (T >= 30.0°C ON, T <= 29.5°C OFF)
                 │                                    │
                 ▼                                    │
            updateLCD()          <────────────────────┘
                 │
                 ▼
       [LCD 1602 I2C Driver]     --> Padded line rendering: "NAVI FAN", "TC", telemetry
```

### State Control Logic

Relay output behavior depends on active power mode and compile-time thermostat parameters:

| Operational State | Thermostat Mode (`TC`) | Condition | Relay Line |
| :--- | :--- | :--- | :--- |
| **Standby** | Any | System offline (`fanMode == false`) | `LOW` (Forced Off) |
| **Manual Active** | Disabled (`TC0`) | `fanMode == true` | `HIGH` (Direct Drive) |
| **Thermostat Active** | Enabled (`TC1`) | $T \ge 30.0^\circ\text{C}$ | `HIGH` (Active Cooling) |
| **Thermostat Active** | Enabled (`TC1`) | $T \le 29.5^\circ\text{C}$ ($T_{thr} - T_{hyst}$) | `LOW` (Cooling Cutoff) |
| **Thermostat Active** | Enabled (`TC1`) | $29.5^\circ\text{C} < T < 30.0^\circ\text{C}$ | Maintained Previous State |

---

## Hardware Specifications & Pinout

| Peripheral | Interface / Pin | Operational Characteristics |
| :--- | :--- | :--- |
| **IR Receiver** | `D2` | Active demodulated input line to `IRremote` engine |
| **Relay Module** | `D3` | Logic-level digital output driving power relay switch |
| **DHT22** | `D4` | Single-bus bidir digital input (AM2302 temp & humidity) |
| **LCD 1602** | `SDA` / `SCL` (A4/A5) | PCF8574 I2C expander bus interface at `0x27` |

---

## Requirements

- Arduino IDE `2.x` or PlatformIO Core
- Toolchain: AVR-GCC (C++11 standard support)

### Library Dependencies

Install required libraries via the Arduino Library Manager or `platformio.ini`:

- `DHT sensor library` (Adafruit) + `Adafruit Unified Sensor`
- `IRremote` (v4.x+)
- `LiquidCrystal_I2C`
- `Wire` (Arduino native core dependency)

---

## Configuration

Core firmware thresholds and pins are set statically through `constexpr` definitions:

```cpp
// pin definitions
constexpr uint8_t kIrPin = 2;
constexpr uint8_t kRelayPin = 3;
constexpr uint8_t kDhtPin = 4;
constexpr uint8_t kDhtType = DHT22;

// thermostat settings
constexpr bool kTempControlEnabled = false; // toggle between TC0 (manual) and TC1 (auto)
constexpr float kTempThreshold = 30.0f;     // activation setpoint in degrees Celsius
constexpr float kTempHysteresis = 0.5f;     // hysteresis cutoff offset
```

---

## Telemetry & Display Layout

The 16x2 character display formats output into dual status lines:

```text
Line 0: [NAVI FAN ON  TC0]   <-- Global power state & thermostat active flag
Line 1: [T: 28.4 H: 55.2 ]   <-- Live metrics, "PRESS ANY KEY" (idle) or "NO DATA"
```

---

## License

MIT
