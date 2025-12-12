# WireBot Power Architecture

WireBot uses a **dual-rail power system**:

- **24 V PSU** for motors and other high-power loads
- **5 V PSU** for control electronics and servos

All grounds are tied together at a single common reference.

## 1. Power Rails

### 24 V Rail

**Source:** 24 V DC PSU (e.g. 24 V, 8–10 A)

**Used for:**

- TMC2209 **VMOT** pins (motor power)
- 24 V fans (if used), switched through SSR or MOSFET
- Any future 24 V actuators

### 5 V Rail

**Source:** 5 V DC PSU (e.g. 5 V, 5–10 A depending on servos/fans)

**Used for:**

- Raspberry Pi Zero 2 W
- Pico A (motion controller)
- Pico B (IO controller)
- Servos
- 5 V fans (if used)

The Pi and both Picos should be fed from the same stable 5 V PSU,
not random USB sources, in the final build.

## 2. Common Ground

**All grounds must be common**, otherwise step/dir signals will not be referenced correctly.

Recommended wiring:

- Tie 24 V PSU GND and 5 V PSU GND together at a single common point.
- Connect Pi GND, Pico A GND, Pico B GND, and driver GND to this point.

## 3. Device Wiring Overview

- Pi Zero:
  - Powered from 5 V PSU
  - Communicates via USB to Pico A & Pico B

- Pico A:
  - Powered from 5 V PSU
  - Drives TMC2209 STEP/DIR/EN
  - Reads endstops

- Pico B:
  - Powered from 5 V PSU
  - Drives servos and fan SSR/MOSFET
  - Reads temperature sensor via ADC
