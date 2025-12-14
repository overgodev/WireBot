# WireBot (docs-only snapshot)

WireBot is a Raspberry Pi Zero 2 W + dual-RP2040 (2× Pico) control architecture for a modular **wire handling machine** with a **web UI**. The Pi Zero hosts the UI + job logic, while the Picos handle real-time motion and IO.

---

## High-level architecture

### Roles
- **Raspberry Pi Zero 2 W (Brain + UI)**
  - Hosts **Web UI** (control panel, job setup, calibration pages)
  - Translates UI actions into high-level serial commands
  - Talks to both Picos over **USB serial**
  - Owns “job logic” (sequence of operations, safety checks, retries)

- **Pico A (Motion Controller)**
  - Drives **5× TMC2209** in **STEP/DIR/EN** mode
  - Reads **4× microswitches** (NC to GND)
  - Implements homing, channel positioning, blade depth positions, preload macro, etc.

- **Pico B (IO Controller)**
  - Controls **2 servos**
  - Controls **1–2 fan/SSR outputs**
  - Reads **1 analog temperature sensor (ADC)**
  - Provides simple serial commands for the Pi to call (servo angles, fan modes)

### Power (important)
- System has a **5V PSU** (explicit requirement).
- Stepper drivers typically use their own motor supply (often 12–24V). Regardless:
  - **All grounds must be common**: Pi GND ↔ Pico A GND ↔ Pico B GND ↔ driver logic GND ↔ motor PSU GND ↔ 5V PSU GND.
  - **TMC2209 VIO must be 3.3V** (from Pico) unless the breakout explicitly supports 5V logic.

---

## Mechanical system: 3-part design

WireBot is split into 3 functional modules:

### 1) MMU / Wire Selector (preload channel system)
Purpose: select a wire channel, preload it so engagement is consistent.

- **M1 (NEMA17, belt + 20T pulley)**: moves selector carriage to “channel positions”
- **Servo 1** (Pico B): lifts/lowers selector (engage/disengage)
- **M2 (NEMA14 pancake)**: pushes wire into a preload chamber
- **SW1 (PRELOAD_HIT)**: microswitch detects wire reached chamber
- Preload behavior:
  1. select channel via M1
  2. drop selector via servo
  3. push wire with M2 until SW1 triggers
  4. backoff by configurable amount to create “preload” so next engagement always grabs

### 2) Feeder module (clamp + feed length)
Purpose: once MMU delivers wire, clamp and feed exact length.

- **SW2 (FEEDER_ENTRY)**: detects wire has arrived at feeder
- **Servo 2** (Pico B): clamps wire into feeder rollers/mechanism
- **M3 (NEMA17)**: feeds wire to length (steps or mm once calibrated)

Sequence (typical):
1. wait for SW2 triggered
2. optional: MMU nudges wire a bit more
3. clamp servo closes
4. M3 feeds required length

### 3) Cutter / Stripper module (single blade axis depth)
Purpose: one blade axis does both strip + cut by moving to different depths.

- **M4 (NEMA17)**: blade depth axis
- **SW3 (CUT_HOME)**: home switch is at **FULL CUT / CLOSED** position
- “Strip depth” is a calibrated position slightly away from closed (shallower)
- “Open” is a safe open position further away

**Leadscrew detail (confirmed):**
- T8 leadscrew, **2mm pitch / 2mm lead** (T8×2)
- High resolution for depth control.

---

## Switch wiring & polarity (very important)

All switches are **NC to GND**, with Pico pins using `INPUT_PULLUP`.

Meaning:
- Not pressed (NC closed to GND) => reads **LOW**
- Pressed/triggered (contact opens, pulled up) => reads **HIGH**

So firmware should treat **TRIGGERED = HIGH**.

---

## Motion axes mapping (Pico A)

Pico A controls 5 stepper channels (TMC2209 STEP/DIR/EN):

| Axis | Name | Motor | Function |
|---|---|---|---|
| M1 | MMU_BELT | NEMA17 | Belt selector carriage (20T pulley) |
| M2 | PRELOAD_PUSH | NEMA14 pancake | Push wire to preload switch then backoff |
| M3 | FEEDER | NEMA17 | Feed wire to length |
| M4 | BLADE | NEMA17 | Cut/strip depth axis on leadscrew |
| M5 | Reserved | (future) | spare axis / expansion |

Switches on Pico A:

| Switch | Name | Meaning |
|---|---|---|
| SW1 | PRELOAD_HIT | Wire reached preload chamber |
| SW2 | FEEDER_ENTRY | Wire arrived at feeder |
| SW3 | CUT_HOME | Blade fully closed/full cut (home position 0) |
| SW4 | MMU_HOME | MMU belt home reference |

---

## Steps/mm (confirmed / computed)

Assumptions used:
- motor: 1.8° (200 steps/rev)
- microstepping: 16×

### MMU belt (20T GT2)
- GT2 pitch = 2mm/tooth
- 20T => 40mm travel/rev
- steps/mm = (200×16)/40 = **80 steps/mm**

### Blade leadscrew (T8×2)
- lead = 2mm travel/rev
- steps/mm = (200×16)/2 = **1600 steps/mm**

Notes:
- M2 preload push steps/mm unknown until mechanism finalized
- M3 feeder steps/mm depends on roller diameter / gearing

---

## Pico B IO mapping

Pico B controls:
- **Servo 1**: selector lift (up/down)
- **Servo 2**: feeder clamp (open/close)
- **Fan/SSR outputs**: 1 or 2 channels
- **Temp sensor**: analog ADC input (0–3.3V)

Servo power must come from external 5V PSU (not Pico 3.3V). Grounds common.

---

## Firmware responsibilities (what exists / planned)

### Pico A (motion)
Planned command set includes:
- Driver enable/disable (global and per-axis)
- Homing:
  - `HOME M1` (MMU belt home via SW4)
  - `HOME M4` (blade home via SW3, closed/full cut)
- Channel system:
  - Pi should send `CH <n>` (Pico A maps to stored positions)
  - store via `SETCH` or `SETCH_MM`
- Preload macro:
  - push M2 until SW1 triggers, then backoff by variable amount
- Blade positions:
  - open position, strip depth, cut position (0 at home)
  - commands like `OPEN`, `STRIP`, `CUT`

### Pico B (IO)
Planned command set includes:
- `SERVO <1|2> <deg>` (or presets later)
- `FAN <1|2> ON|OFF|AUTO`
- `TEMP?` returning raw+voltage
- optional overtemp latch

---

## Typical job flow (Pi-driven)

Example “produce one wire” flow (conceptual):
1. **Select channel**
   - Pico A: `CH n` (moves MMU belt)
   - Pico B: selector servo down
2. **Preload**
   - Pico A: `PRELOAD maxSteps backoff`
3. **Deliver to feeder**
   - Pi waits/polls Pico A: SW2 becomes triggered
   - Pico B: clamp servo closes
4. **Feed length**
   - Pico A: feeder motor M3 steps/mm once calibrated
5. **Strip**
   - Pico A: blade to strip depth (M4)
   - Pico A: blade open
6. **Cut**
   - Pico A: blade closed/full cut (home position 0)
   - Pico A: blade open
7. Release clamp, retract/advance as needed

---

## Repo naming

Project name: **WireBot**

Suggested repo description:
> “WireBot: Pi Zero + dual-Pico modular wire selector / feeder / strip+cut machine with web UI.”

---

## Notes for future improvements

- Store calibration values (channels, blade open/strip depth, steps/mm) in non-volatile memory (EEPROM/flash) on Pico A.
- Add higher-level macros on Pico A for “one full cycle” to reduce latency and simplify Pi logic.
- Add servo “presets” on Pico B so Pi sends `SEL DOWN` / `CLAMP ON` rather than raw angles.
- Add safety: emergency stop input, motor current tuning, motion soft-limits, stall detection (optional).

---

## Quick reminder: switch polarity

Because switches are **NC to GND** with pullups:
- Triggered = **HIGH**
- Normal = **LOW**
