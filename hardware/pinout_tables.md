# Pinout Tables – WireBot

This file summarizes the key pin mappings for all main boards.

## Pico A – Motion Controller

See also: `../firmware/pins_picoA.md`

- 5× TMC2209 drivers (STEP/DIR/EN)
- 4× endstops
- Status LED

## Pico B – IO Controller

See also: `../firmware/pins_picoB.md`

- 2× servos
- 1–2× fan outputs (SSR/MOSFET)
- 1× temperature sensor input
- Status LEDs

## Raspberry Pi Zero 2 W

Currently minimal GPIO usage (everything is web controlled and serial-based):

- 5V, GND – from 5 V PSU
- USB – connection to Pico A & Pico B
- (Optional) status LEDs or I²C display can be added later.
