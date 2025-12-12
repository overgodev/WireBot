# firmware/ – Pico Firmware Planning

This folder is for the **future firmware code** that will run on:

- **Pico A** – Motion controller (5× TMC2209 + 4 endstops)
- **Pico B** – IO controller (servos, fans, temperature sensor)

At the moment, there is **no actual firmware logic here yet**.
Only pin maps and empty placeholders exist so that later code can be dropped in cleanly.

## Responsibilities

### Pico A (Motion)

- Generate real-time STEP/DIR pulses for:
  - Feed axis
  - Cutter axis
  - Strip/Z axis
  - Puller/roller axis
  - Auxiliary axis
- Handle homing via 4 endstop switches
- Implement basic motion profiles (accel/decel)
- Receive simple text commands over USB serial from the Pi

### Pico B (IO)

- Drive 2 servo outputs
- Drive 1–2 SSR or MOSFET outputs for fans
- Read 1 analog temperature sensor (e.g. NTC) on an ADC pin
- Implement simple fan/servo behaviour commands

Pin maps are documented in `pins_picoA.md` and `pins_picoB.md`.
