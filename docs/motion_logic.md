# Motion Logic – Planning

Motion logic will live on **Pico A**.

Ideas:

- Each axis has:
  - Current position (in steps)
  - Target position (in steps)
  - Max speed and acceleration
- A simple motion scheduler:
  - Moves one or more axes at a time depending on command
  - Handles homing sequences using endstops
- At first, motion can be:
  - "Move axis X by N steps" (relative)
  - "Home axis X until endstop" (homing)
  - "Execute CUT job: feed, then cut, then retract"

Detailed implementation is left for later,
but this file will keep track of design decisions and algorithms used.
