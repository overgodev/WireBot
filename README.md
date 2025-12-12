# WireBot ⚙️
### My Web‑Controlled Automated Wire Cutting Machine

WireBot is my personal project to build a CNC‑style, web‑controlled wire cutting machine using:

- Raspberry Pi Zero 2 W (web UI + backend)
- Raspberry Pi Pico A (real‑time motion controller for 5 steppers)
- Raspberry Pi Pico B (IO controller for servos, fans, temperature)
- TMC2209 stepper drivers
- 24 V + 5 V dual‑rail power

This repository currently contains **only documentation and structure** — no real firmware or backend code yet.
I’m using it as a clean starting point for design, wiring, and planning before I write the actual code.

## Repository Layout

- `backend/` – notes about the future API server on the Raspberry Pi
- `firmware/` – notes and pin maps for Pico A & Pico B firmware
- `webui/` – user flow and interface planning
- `hardware/` – power architecture, wiring notes, pinout tables
- `docs/` – overall system architecture, serial protocol, roadmap

## Status

- [x] Name + concept
- [x] Hardware architecture (24 V + 5 V, Pi + 2× Pico)
- [x] Pin planning for all controllers
- [x] Documentation skeleton
- [ ] Firmware implementation
- [ ] Backend implementation
- [ ] Web UI implementation
- [ ] First real test on hardware

This is a **work in progress** and intentionally documentation‑first.
