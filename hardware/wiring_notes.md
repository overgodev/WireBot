# Wiring Notes – WireBot

High-level wiring notes for later when I build or re-build the machine.

## Stepper Drivers (TMC2209)

For each driver:

- VMOT → 24 V PSU +
- GND  → Common ground
- VIO  → 3.3 V from Pico A
- STEP → Pico A STEP pin for that axis
- DIR  → Pico A DIR pin for that axis
- EN   → Pico A EN pin for that axis (likely active LOW)
- Motor coils → A1/A2/B1/B2

## Servos

- Power: 5 V PSU (do NOT power directly from Pi 5 V rail alone)
- Ground: common ground (tied to Pico B and Pi)
- Signal: from Pico B GPIO (GP2 / GP3)

## Fans and SSR

- DC fans:
  - Either 5 V or 24 V depending on choice
  - Switched on the low side via MOSFET or via SSR
- AC loads:
  - Only through a proper AC-rated SSR
  - Pico only drives the SSR input; never handle mains directly

## Endstops

- Use normally-closed wiring if possible
- Connect between GPIO and GND
- Enable INPUT_PULLUP in firmware
