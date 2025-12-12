# backend/ – Raspberry Pi Backend (Planning Only)

This folder is reserved for the **future backend server** that will run on the Raspberry Pi Zero 2 W.

The backend will be responsible for:

- Hosting a simple HTTP API used by the Web UI
- Managing USB serial connections to:
  - Pico A (motion controller)
  - Pico B (IO controller)
- Translating API calls into serial commands
- Tracking job state (IDLE / RUNNING / ERROR)
- Exposing logs and live status to the frontend

For now, this folder contains only documentation and planning files.
