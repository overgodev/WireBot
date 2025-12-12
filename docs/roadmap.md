# Roadmap – WireBot

Rough plan for how I want to build this project.

## Phase 1 – Planning (current)

- [x] Pick name: WireBot
- [x] Define hardware topology (Pi + 2× Pico)
- [x] Define power architecture (24 V + 5 V)
- [x] Define pinout for Pico A and Pico B
- [x] Create repo skeleton and documentation

## Phase 2 – Basic Firmware + Backend

- [ ] Implement minimal Pico A firmware:
  - Single axis move in steps
  - Simple homing on one endstop
- [ ] Implement minimal Pico B firmware:
  - Toggle fan output
  - Read temperature and print it
- [ ] Implement Python backend:
  - One route to send a test command
  - Basic serial connection management

## Phase 3 – Full Motion + Web UI

- [ ] Expand Pico A firmware to handle:
  - All 5 axes
  - Homing for all axes
  - Simple motion profiles
  - Basic job sequence (feed + cut)
- [ ] Implement full HTTP API on Pi
- [ ] Implement Web UI:
  - Job configuration
  - Live progress display
  - Logs

## Phase 4 – Tuning and Safety

- [ ] Tune speeds and accelerations
- [ ] Add better error handling (endstop errors, overtemp, etc.)
- [ ] Add E-stop behaviour
- [ ] Document everything with real photos and diagrams
