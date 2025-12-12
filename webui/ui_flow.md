# Web UI Flow – WireBot

This document describes how a user (me) will interact with the WireBot web interface.

## Main Screens

### 1. Top Bar

- Title: "WireBot"
- Status pill:
  - Green: IDLE
  - Blue: RUNNING / HOMING
  - Red: ERROR / E-STOP

### 2. Step Cards (Left-to-right or top-to-bottom)

1. **Connect & Check**
   - Actions:
     - Scan for devices
     - Connect to Pico A (motion)
     - Connect to Pico B (IO)
   - Status tags:
     - "Pico A: connected / not connected"
     - "Pico B: connected / not connected"

2. **Home Axes**
   - Actions:
     - Home All
     - Home Feed only
     - Home Cutter only
   - Status:
     - Endstop state OK / triggered

3. **Configure Job**
   - Inputs:
     - Wire length (mm)
     - Quantity (pcs)
     - Feed speed (mm/s)
     - Cut profile (normal / slow / soft)
   - Auto-updates a "Job Summary" panel.

4. **Advanced Settings (Optional)**
   - Inputs:
     - Acceleration (mm/s²)
     - Servo mode (off / clamp / sort)
     - Fan mode (auto / on / off)

5. **Preview & Run**
   - Shows:
     - Summary of configured job
     - Buttons:
       - Dry Run (no cutting)
       - Start Job
       - Emergency Stop

6. **Live Status & Logs**
   - Shows:
     - Pieces done / target
     - Current state (IDLE / RUNNING / HOMING)
     - Current temperature and fan state
   - Log panel:
     - Lines like:
       - "[INFO] Connected to Pico A"
       - "[CMD] HOME ALL"
       - "[INFO] Job completed."

This flow is already reflected in the planned `index.html` layout (to be implemented later).
