# Pico A Pin Map – Motion Controller (Planning)

Pico A is dedicated to **motion**. It will control 5x TMC2209 stepper drivers and read 4 endstop switches.

## Stepper Drivers (TMC2209 in STEP/DIR mode)

| Axis | Purpose       | STEP (GPIO) | DIR (GPIO) | EN (GPIO) |
|------|---------------|------------:|-----------:|----------:|
| M1   | Feed          | GP2         | GP3        | GP4       |
| M2   | Cutter        | GP5         | GP6        | GP7       |
| M3   | Strip / Z     | GP8         | GP9        | GP10      |
| M4   | Puller/Roller | GP11        | GP12       | GP13      |
| M5   | Aux axis      | GP14        | GP15       | GP16      |

Notes:
- All EN pins can be individually controlled.
- In the future, I might tie all EN pins together for a global enable.

## Endstops

4 × micro switches, ideally wired **normally closed** to GND, read using INPUT_PULLUP:

| Endstop | GPIO | Notes                           |
|---------|------|---------------------------------|
| E1      | GP17 | Home switch for feed            |
| E2      | GP18 | Home switch for cutter          |
| E3      | GP19 | Home switch for strip/Z         |
| E4      | GP20 | Shared or extra axis home       |

## Status / Debug Pins

| Pin  | Function            |
|------|---------------------|
| GP21 | Motion status LED   |
| GP22 | Spare IO            |
| GP25 | Onboard LED         |

## Reserved ADC Pins

For future sensors (tension, current, etc.):

- GP26 – ADC0
- GP27 – ADC1
- GP28 – ADC2
