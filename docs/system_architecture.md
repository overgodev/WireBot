# System Architecture – WireBot

WireBot is built around three main compute nodes:

- 1× Raspberry Pi Zero 2 W (web + backend)
- 2× Raspberry Pi Pico (motion + IO)

## High-Level View

- User's browser → talks HTTP to → Pi backend
- Pi backend → talks USB serial to:
  - Pico A (motion)
  - Pico B (IO)
- Pico A:
  - Controls 5 stepper axes via TMC2209 drivers
  - Reads endstops and handles homing
- Pico B:
  - Controls servos and fans
  - Reads temperature sensor

The serial protocol between Pi and Picos is text-based and command-oriented
(planned, but not implemented yet).
