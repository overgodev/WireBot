#include <Arduino.h>

// =====================================================
// WireBot – Pico A Motion Firmware (v0.3)
// Adds: steps/mm, MOVE_MM, GOTO_MM, SETCH_MM, blade positions in mm.
//
// ASSUMPTIONS (edit if needed):
// - Motors: 1.8° (200 steps/rev)
// - Microstepping: 16×
// - MMU belt: GT2 20T => 40 mm/rev => 80 steps/mm
// - Blade leadscrew: 8 mm lead/rev => 400 steps/mm
//
// SWITCH WIRING:
// - NC to GND + INPUT_PULLUP
// - Triggered/HIT => pin reads HIGH
// =====================================================

// -----------------------------
// Pin Map (Pico A)
// -----------------------------
#define M1_STEP_PIN   2
#define M1_DIR_PIN    3
#define M1_EN_PIN     4

#define M2_STEP_PIN   5
#define M2_DIR_PIN    6
#define M2_EN_PIN     7

#define M3_STEP_PIN   8
#define M3_DIR_PIN    9
#define M3_EN_PIN     10

#define M4_STEP_PIN   11
#define M4_DIR_PIN    12
#define M4_EN_PIN     13

#define M5_STEP_PIN   14
#define M5_DIR_PIN    15
#define M5_EN_PIN     16

#define SW1_PIN  17  // PRELOAD_HIT
#define SW2_PIN  18  // FEEDER_ENTRY
#define SW3_PIN  19  // CUT_HOME (CLOSED / FULL CUT)
#define SW4_PIN  20  // MMU_HOME (belt selector)

#define STATUS_LED_PIN 21
#define ONBOARD_LED    25

static const uint32_t STEP_PULSE_US = 3;

static const float DEFAULT_VMAX  = 2000.0f;   // steps/s
static const float DEFAULT_ACCEL = 4000.0f;   // steps/s^2

static const float HOME_V     = 700.0f;
static const float HOME_ACCEL = 2500.0f;
static const uint32_t HOME_TIMEOUT_MS = 20000;

static const bool EN_ACTIVE_LOW = true;
static const bool SWITCH_TRIGGERED_HIGH = true;

// -----------------------------
// SPMM defaults (editable at runtime)
// -----------------------------
static float spmm[5] = {
  80.0f,   // M1 MMU_BELT (20T GT2)
  0.0f,    // M2 PRELOAD_PUSH (unknown yet, leave 0 until you measure)
  0.0f,    // M3 FEEDER (unknown yet)
  400.0f,  // M4 BLADE leadscrew 8mm lead
  0.0f     // M5 reserved
};

// -----------------------------
// Channel + blade calibration (steps, RAM only)
// -----------------------------
static const int MAX_CH = 16;
static int32_t chPos[MAX_CH + 1];   // channel positions in steps (M1)
static int currentCh = 0;

// Blade axis positions relative to CUT_HOME (closed) = 0
static int32_t bladeOpenPos  = 2000;
static int32_t bladeCutPos   = 0;
static int32_t bladeStripPos = 400;

// =====================================================
// Axis model
// =====================================================
struct Axis {
  const char* name;
  uint8_t stepPin, dirPin, enPin;

  int32_t posSteps = 0;

  bool enabled = false;
  bool moving = false;
  bool jogging = false;
  bool homing = false;

  int8_t dir = +1;
  int32_t targetSteps = 0;
  float vmax = DEFAULT_VMAX;
  float accel = DEFAULT_ACCEL;

  float v = 0.0f;
  uint32_t lastUpdateUs = 0;
  uint32_t lastStepUs = 0;
  uint32_t stepIntervalUs = 0;

  float jogV = 0.0f;

  int swPin = -1;
  bool hasSwitch = false;
  uint32_t homeStartMs = 0;
  int8_t homeDir = -1;
};

Axis ax[5] = {
  { "M1", M1_STEP_PIN, M1_DIR_PIN, M1_EN_PIN },
  { "M2", M2_STEP_PIN, M2_DIR_PIN, M2_EN_PIN },
  { "M3", M3_STEP_PIN, M3_DIR_PIN, M3_EN_PIN },
  { "M4", M4_STEP_PIN, M4_DIR_PIN, M4_EN_PIN },
  { "M5", M5_STEP_PIN, M5_DIR_PIN, M5_EN_PIN },
};

static inline bool swTriggered(int pin) {
  int v = digitalRead(pin);
  return SWITCH_TRIGGERED_HIGH ? (v == HIGH) : (v == LOW);
}

static inline bool axisSwitchTriggered(const Axis& a) {
  if (!a.hasSwitch) return false;
  return swTriggered(a.swPin);
}

static void axisEnable(Axis& a, bool en) {
  a.enabled = en;
  if (EN_ACTIVE_LOW) digitalWrite(a.enPin, en ? LOW : HIGH);
  else              digitalWrite(a.enPin, en ? HIGH : LOW);
}

static void axisSetDir(Axis& a, int8_t dir) {
  a.dir = (dir >= 0) ? +1 : -1;
  digitalWrite(a.dirPin, (a.dir > 0) ? HIGH : LOW);
}

static void axisStop(Axis& a) {
  a.moving = false;
  a.jogging = false;
  a.homing = false;
  a.v = 0.0f;
  a.stepIntervalUs = 0;
}

static inline void axisStepOnce(Axis& a) {
  digitalWrite(a.stepPin, HIGH);
  delayMicroseconds(STEP_PULSE_US);
  digitalWrite(a.stepPin, LOW);
  a.posSteps += (a.dir > 0) ? +1 : -1;
}

static inline float decelDistanceSteps(float v, float a) {
  if (a <= 1e-6f) return 0.0f;
  return (v * v) / (2.0f * a);
}

static Axis* findAxisByName(const String& s) {
  for (auto &a : ax) if (s == a.name) return &a;
  return nullptr;
}

static int axisIndex(const String& s) {
  if (s == "M1") return 0;
  if (s == "M2") return 1;
  if (s == "M3") return 2;
  if (s == "M4") return 3;
  if (s == "M5") return 4;
  return -1;
}

static void startMoveAbs(Axis& a, int32_t targetAbs, float vmax, float accel) {
  if (!a.enabled) { Serial.println("ERR DRIVER_DISABLED"); return; }
  a.targetSteps = targetAbs;
  a.vmax = max(10.0f, vmax);
  a.accel = max(10.0f, accel);

  int32_t delta = a.targetSteps - a.posSteps;
  if (delta == 0) { Serial.println("OK ALREADY_THERE"); return; }

  axisSetDir(a, (delta > 0) ? +1 : -1);
  a.moving = true; a.jogging = false; a.homing = false;
  a.v = 0.0f;
  a.lastUpdateUs = micros();
  a.lastStepUs = micros();
  a.stepIntervalUs = 0;
}

static void startMoveRel(Axis& a, int32_t deltaSteps, float vmax, float accel) {
  startMoveAbs(a, a.posSteps + deltaSteps, vmax, accel);
}

static void startJog(Axis& a, float signedSpeed, float accel) {
  if (!a.enabled) { Serial.println("ERR DRIVER_DISABLED"); return; }
  if (fabs(signedSpeed) < 1.0f) { axisStop(a); Serial.println("OK JOG_STOP"); return; }

  axisSetDir(a, (signedSpeed >= 0.0f) ? +1 : -1);
  a.jogV = fabs(signedSpeed);
  a.accel = max(10.0f, accel);
  a.vmax = a.jogV;

  a.jogging = true; a.moving = false; a.homing = false;
  a.v = 0.0f;
  a.lastUpdateUs = micros();
  a.lastStepUs = micros();
  a.stepIntervalUs = 0;
}

static void startHome(Axis& a) {
  if (!a.hasSwitch) { Serial.println("ERR NO_SWITCH"); return; }
  if (!a.enabled)   { Serial.println("ERR DRIVER_DISABLED"); return; }

  axisSetDir(a, a.homeDir);
  a.homing = true; a.moving = false; a.jogging = false;
  a.vmax = HOME_V;
  a.accel = HOME_ACCEL;
  a.v = 0.0f;
  a.homeStartMs = millis();

  a.lastUpdateUs = micros();
  a.lastStepUs = micros();
  a.stepIntervalUs = 0;
}

static void updateAxis(Axis& a, uint32_t nowUs) {
  if (!a.enabled) return;
  if (!(a.moving || a.jogging || a.homing)) return;

  if (a.homing && (millis() - a.homeStartMs) > HOME_TIMEOUT_MS) {
    axisStop(a);
    Serial.print("ERR HOME_TIMEOUT "); Serial.println(a.name);
    return;
  }

  if (a.homing && axisSwitchTriggered(a)) {
    axisStop(a);
    a.posSteps = 0;
    Serial.print("OK HOME "); Serial.println(a.name);
    return;
  }

  uint32_t dtUs = nowUs - a.lastUpdateUs;
  if (dtUs == 0) dtUs = 1;
  a.lastUpdateUs = nowUs;
  float dt = (float)dtUs / 1000000.0f;

  float desiredV = a.vmax;

  if (a.moving) {
    int32_t remaining = a.targetSteps - a.posSteps;
    int32_t remAbs = abs(remaining);

    float dDecel = decelDistanceSteps(a.v, a.accel);
    desiredV = ((float)remAbs <= dDecel + 1.0f) ? 0.0f : a.vmax;

    int8_t wantDir = (remaining > 0) ? +1 : -1;
    if (wantDir != a.dir) axisSetDir(a, wantDir);

    if (remAbs == 0) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }
  }

  if (a.jogging) desiredV = a.jogV;
  if (a.homing)  desiredV = a.vmax;

  if (a.v < desiredV) a.v = min(desiredV, a.v + a.accel * dt);
  else if (a.v > desiredV) a.v = max(desiredV, a.v - a.accel * dt);

  if (a.moving) {
    int32_t remAbs = abs(a.targetSteps - a.posSteps);
    if (a.v < 5.0f && remAbs <= 2) {
      while (a.posSteps != a.targetSteps) axisStepOnce(a);
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }
  }

  if (!a.jogging && !a.homing && a.v <= 0.5f) {
    axisStop(a);
    Serial.print("OK DONE "); Serial.println(a.name);
    return;
  }

  float speed = max(1.0f, a.v);
  a.stepIntervalUs = (uint32_t)(1000000.0f / speed);

  while ((uint32_t)(nowUs - a.lastStepUs) >= a.stepIntervalUs) {
    a.lastStepUs += a.stepIntervalUs;

    if (a.moving && a.posSteps == a.targetSteps) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }

    axisStepOnce(a);

    if (a.homing && axisSwitchTriggered(a)) {
      axisStop(a);
      a.posSteps = 0;
      Serial.print("OK HOME "); Serial.println(a.name);
      return;
    }

    if (a.moving && a.posSteps == a.targetSteps) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }

    if (a.stepIntervalUs < 200) break;
  }
}

// =====================================================
// PRELOAD macro (M2 until SW1 triggers, then backoff)
// =====================================================
enum PreloadState : uint8_t { PL_IDLE=0, PL_PUSHING=1, PL_BACKOFF=2 };
static PreloadState plState = PL_IDLE;
static int32_t plMaxSteps = 0;
static int32_t plBackoff = 0;
static int32_t plStartPos = 0;
static float plV = 800.0f;
static float plA = 2500.0f;

static void preloadStart(int32_t maxSteps, int32_t backoffSteps, float vmax, float accel) {
  Axis &m2 = ax[1];
  if (!m2.enabled) { Serial.println("ERR DRIVER_DISABLED"); return; }

  plState = PL_PUSHING;
  plMaxSteps = maxSteps;
  plBackoff = backoffSteps;
  plStartPos = m2.posSteps;
  plV = vmax;
  plA = accel;

  startJog(m2, +plV, plA);
  Serial.println("OK PRELOAD START");
}

static void preloadUpdate() {
  if (plState == PL_IDLE) return;

  Axis &m2 = ax[1];
  int32_t pushed = abs(m2.posSteps - plStartPos);

  if (plState == PL_PUSHING) {
    if (swTriggered(SW1_PIN)) {
      axisStop(m2);
      plState = PL_BACKOFF;
      startMoveRel(m2, -plBackoff, plV, plA);
      Serial.println("OK PRELOAD HIT");
      return;
    }
    if (pushed >= plMaxSteps) {
      axisStop(m2);
      plState = PL_IDLE;
      Serial.println("ERR PRELOAD_TIMEOUT");
      return;
    }
  } else if (plState == PL_BACKOFF) {
    if (!(m2.moving || m2.jogging || m2.homing)) {
      plState = PL_IDLE;
      Serial.println("OK PRELOAD DONE");
    }
  }
}

// =====================================================
// Helpers: mm ↔ steps
// =====================================================
static bool hasSpmm(int idx) {
  return idx >= 0 && idx < 5 && spmm[idx] > 0.0001f;
}

static int32_t mmToSteps(int idx, float mm) {
  return (int32_t)lroundf(mm * spmm[idx]);
}

static float stepsToMm(int idx, int32_t steps) {
  if (!hasSpmm(idx)) return 0.0f;
  return (float)steps / spmm[idx];
}

// =====================================================
// Serial command parser
// =====================================================
static String inLine;

static void printHelp() {
  Serial.println("WireBot PicoA v0.3 commands:");
  Serial.println("  HELP | PING | STATUS? | SW?");
  Serial.println("  EN <0|1> | AXEN <Mx> <0|1> | STOP");
  Serial.println("  MOVE <Mx> <steps> [v] [a]");
  Serial.println("  GOTO <Mx> <pos> [v] [a]");
  Serial.println("  JOG  <Mx> <v> [a]");
  Serial.println("  HOME <M1|M4|ALL>");
  Serial.println("  SPMM? | SETSPMM <Mx> <steps_per_mm>");
  Serial.println("  MOVE_MM <Mx> <mm> [v_mm_s] [a_mm_s2]");
  Serial.println("  GOTO_MM <Mx> <mm> [v_mm_s] [a_mm_s2]");
  Serial.println("  SETCH <n> <posSteps> | SETCH_MM <n> <mm> | CH <n> | CH?");
  Serial.println("  PRELOAD <maxSteps> <backoff> [v] [a]");
  Serial.println("  SETOPEN <steps> | SETOPEN_MM <mm>");
  Serial.println("  SETSTRIP <steps> | SETSTRIP_MM <mm>");
  Serial.println("  OPEN | CUT | STRIP");
}

static void printSwitches() {
  Serial.print("SW1_PRELOAD="); Serial.print(swTriggered(SW1_PIN) ? 1 : 0);
  Serial.print(" SW2_FEEDER="); Serial.print(swTriggered(SW2_PIN) ? 1 : 0);
  Serial.print(" SW3_CUT_HOME="); Serial.print(swTriggered(SW3_PIN) ? 1 : 0);
  Serial.print(" SW4_MMU_HOME="); Serial.println(swTriggered(SW4_PIN) ? 1 : 0);
}

static void printSpmm() {
  Serial.print("SPMM M1="); Serial.print(spmm[0], 3);
  Serial.print(" M2="); Serial.print(spmm[1], 3);
  Serial.print(" M3="); Serial.print(spmm[2], 3);
  Serial.print(" M4="); Serial.print(spmm[3], 3);
  Serial.print(" M5="); Serial.println(spmm[4], 3);
}

static void printStatus() {
  bool busy = false;
  for (auto &a : ax) if (a.moving || a.jogging || a.homing) { busy = true; break; }

  Serial.print("STATUS "); Serial.println(busy ? "BUSY" : "IDLE");
  Serial.print("CH="); Serial.println(currentCh);
  printSpmm();

  for (int i = 0; i < 5; i++) {
    auto &a = ax[i];
    Serial.print(a.name);
    Serial.print(" pos="); Serial.print(a.posSteps);
    if (hasSpmm(i)) {
      Serial.print(" ("); Serial.print(stepsToMm(i, a.posSteps), 3); Serial.print("mm)");
    }
    Serial.print(" en="); Serial.print(a.enabled ? 1 : 0);
    Serial.print(" mode=");
    if (a.homing) Serial.print("HOME");
    else if (a.jogging) Serial.print("JOG");
    else if (a.moving) Serial.print("MOVE");
    else Serial.print("IDLE");
    Serial.println();
  }

  Serial.print("BLADE open="); Serial.print(bladeOpenPos);
  Serial.print(" strip="); Serial.print(bladeStripPos);
  Serial.print(" cut="); Serial.println(bladeCutPos);
}

static Axis* axisFromTok(const String& tok, int &idxOut) {
  String t = tok; t.toUpperCase();
  idxOut = -1;
  if (t == "M1") idxOut = 0;
  else if (t == "M2") idxOut = 1;
  else if (t == "M3") idxOut = 2;
  else if (t == "M4") idxOut = 3;
  else if (t == "M5") idxOut = 4;
  if (idxOut < 0) return nullptr;
  return &ax[idxOut];
}

static float parseFloatOr(const String& s, float fallback) {
  if (s.length() == 0) return fallback;
  return s.toFloat();
}

static void handleCommand(const String& line) {
  String t[10];
  int n = 0;
  int start = 0;
  for (int i = 0; i <= (int)line.length(); i++) {
    if (i == (int)line.length() || line[i] == ' ') {
      if (i > start) {
        if (n < 10) t[n++] = line.substring(start, i);
      }
      start = i + 1;
    }
  }
  if (n == 0) return;

  String cmd = t[0];
  cmd.toUpperCase();

  if (cmd == "HELP") { printHelp(); return; }
  if (cmd == "PING") { Serial.println("OK PONG"); return; }
  if (cmd == "STATUS?" || cmd == "STATUS") { printStatus(); return; }
  if (cmd == "SW?") { printSwitches(); return; }
  if (cmd == "SPMM?") { printSpmm(); return; }

  if (cmd == "STOP") {
    for (auto &a : ax) axisStop(a);
    plState = PL_IDLE;
    Serial.println("OK STOP");
    return;
  }

  if (cmd == "EN") {
    int en = (n >= 2) ? t[1].toInt() : 0;
    for (auto &a : ax) axisEnable(a, en != 0);
    digitalWrite(STATUS_LED_PIN, (en != 0) ? HIGH : LOW);
    Serial.print("OK EN "); Serial.println(en != 0 ? 1 : 0);
    return;
  }

  if (cmd == "AXEN") {
    if (n < 3) { Serial.println("ERR AXEN Mx 0|1"); return; }
    int idx; Axis* a = axisFromTok(t[1], idx);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    int en = t[2].toInt();
    axisEnable(*a, en != 0);
    Serial.print("OK AXEN "); Serial.print(a->name); Serial.print(" "); Serial.println(en != 0 ? 1 : 0);
    return;
  }

  if (cmd == "SETSPMM") {
    if (n < 3) { Serial.println("ERR SETSPMM Mx steps_per_mm"); return; }
    int idx; Axis* a = axisFromTok(t[1], idx);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    float v = t[2].toFloat();
    if (v <= 0.0f) { Serial.println("ERR BAD_SPMM"); return; }
    spmm[idx] = v;
    Serial.print("OK SETSPMM "); Serial.print(a->name); Serial.print(" "); Serial.println(spmm[idx], 3);
    return;
  }

  if (cmd == "MOVE" || cmd == "GOTO") {
    if (n < 3) { Serial.println("ERR MOVE/GOTO usage"); return; }
    int idx; Axis* a = axisFromTok(t[1], idx);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }

    int32_t val = (int32_t)t[2].toInt();
    float vmax = (n >= 4) ? parseFloatOr(t[3], DEFAULT_VMAX) : DEFAULT_VMAX;
    float acc  = (n >= 5) ? parseFloatOr(t[4], DEFAULT_ACCEL) : DEFAULT_ACCEL;

    if (cmd == "MOVE") startMoveRel(*a, val, vmax, acc);
    else              startMoveAbs(*a, val, vmax, acc);

    Serial.print("OK "); Serial.print(cmd); Serial.print(" "); Serial.print(a->name); Serial.print(" "); Serial.println(val);
    return;
  }

  if (cmd == "MOVE_MM" || cmd == "GOTO_MM") {
    if (n < 3) { Serial.println("ERR MOVE_MM/GOTO_MM Mx mm [v_mm_s] [a_mm_s2]"); return; }
    int idx; Axis* a = axisFromTok(t[1], idx);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    if (!hasSpmm(idx)) { Serial.println("ERR NO_SPMM"); return; }

    float mm = t[2].toFloat();
    float vmm = (n >= 4) ? parseFloatOr(t[3], 50.0f) : 50.0f;     // mm/s default
    float amm = (n >= 5) ? parseFloatOr(t[4], 200.0f) : 200.0f;   // mm/s^2 default

    int32_t steps = mmToSteps(idx, mm);
    float vmax = fabs(vmm) * spmm[idx];  // steps/s
    float acc  = fabs(amm) * spmm[idx];  // steps/s^2

    if (cmd == "MOVE_MM") startMoveRel(*a, steps, vmax, acc);
    else                 startMoveAbs(*a, steps, vmax, acc);

    Serial.print("OK "); Serial.print(cmd); Serial.print(" "); Serial.print(a->name);
    Serial.print(" mm="); Serial.print(mm, 3);
    Serial.print(" steps="); Serial.println(steps);
    return;
  }

  if (cmd == "JOG") {
    if (n < 3) { Serial.println("ERR JOG Mx v [a]"); return; }
    int idx; Axis* a = axisFromTok(t[1], idx);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    float v = t[2].toFloat();
    float acc = (n >= 4) ? parseFloatOr(t[3], DEFAULT_ACCEL) : DEFAULT_ACCEL;
    startJog(*a, v, acc);
    Serial.print("OK JOG "); Serial.print(a->name); Serial.print(" "); Serial.println(v, 1);
    return;
  }

  if (cmd == "HOME") {
    if (n < 2) { Serial.println("ERR HOME M1|M4|ALL"); return; }
    String who = t[1]; who.toUpperCase();
    if (who == "ALL") {
      startHome(ax[0]);
      startHome(ax[3]);
      Serial.println("OK HOME ALL");
      return;
    }
    if (who != "M1" && who != "M4") { Serial.println("ERR HOME_ONLY_M1_M4"); return; }
    int idx; Axis* a = axisFromTok(who, idx);
    startHome(*a);
    Serial.print("OK HOME "); Serial.println(a->name);
    return;
  }

  if (cmd == "SETCH") {
    if (n < 3) { Serial.println("ERR SETCH n posSteps"); return; }
    int ch = t[1].toInt();
    int32_t pos = (int32_t)t[2].toInt();
    if (ch < 1 || ch > MAX_CH) { Serial.println("ERR BAD_CH"); return; }
    chPos[ch] = pos;
    Serial.print("OK SETCH "); Serial.print(ch); Serial.print(" "); Serial.println(pos);
    return;
  }

  if (cmd == "SETCH_MM") {
    if (n < 3) { Serial.println("ERR SETCH_MM n mm"); return; }
    if (!hasSpmm(0)) { Serial.println("ERR NO_SPMM_M1"); return; }
    int ch = t[1].toInt();
    float mm = t[2].toFloat();
    if (ch < 1 || ch > MAX_CH) { Serial.println("ERR BAD_CH"); return; }
    chPos[ch] = mmToSteps(0, mm);
    Serial.print("OK SETCH_MM "); Serial.print(ch); Serial.print(" mm="); Serial.print(mm, 3);
    Serial.print(" steps="); Serial.println(chPos[ch]);
    return;
  }

  if (cmd == "CH?") {
    Serial.print("CH="); Serial.println(currentCh);
    for (int i = 1; i <= MAX_CH; i++) {
      if (chPos[i] != 0 || i == 1) {
        Serial.print("CH"); Serial.print(i); Serial.print("=");
        Serial.print(chPos[i]);
        if (hasSpmm(0)) { Serial.print(" ("); Serial.print(stepsToMm(0, chPos[i]), 3); Serial.print("mm)"); }
        Serial.println();
      }
    }
    return;
  }

  if (cmd == "CH") {
    if (n < 2) { Serial.println("ERR CH n [v] [a]"); return; }
    int ch = t[1].toInt();
    if (ch < 1 || ch > MAX_CH) { Serial.println("ERR BAD_CH"); return; }

    float vmax = (n >= 3) ? parseFloatOr(t[2], DEFAULT_VMAX) : DEFAULT_VMAX;
    float acc  = (n >= 4) ? parseFloatOr(t[3], DEFAULT_ACCEL) : DEFAULT_ACCEL;

    startMoveAbs(ax[0], chPos[ch], vmax, acc);
    currentCh = ch;
    Serial.print("OK CH "); Serial.println(ch);
    return;
  }

  if (cmd == "PRELOAD") {
    if (n < 3) { Serial.println("ERR PRELOAD maxSteps backoff [v] [a]"); return; }
    int32_t maxS = (int32_t)t[1].toInt();
    int32_t back = (int32_t)t[2].toInt();
    float vmax = (n >= 4) ? parseFloatOr(t[3], 800.0f) : 800.0f;
    float acc  = (n >= 5) ? parseFloatOr(t[4], 2500.0f) : 2500.0f;
    preloadStart(maxS, back, vmax, acc);
    return;
  }

  if (cmd == "SETOPEN") {
    if (n < 2) { Serial.println("ERR SETOPEN steps"); return; }
    bladeOpenPos = (int32_t)t[1].toInt();
    Serial.print("OK SETOPEN "); Serial.println(bladeOpenPos);
    return;
  }
  if (cmd == "SETOPEN_MM") {
    if (n < 2) { Serial.println("ERR SETOPEN_MM mm"); return; }
    if (!hasSpmm(3)) { Serial.println("ERR NO_SPMM_M4"); return; }
    float mm = t[1].toFloat();
    bladeOpenPos = mmToSteps(3, mm);
    Serial.print("OK SETOPEN_MM mm="); Serial.print(mm, 3);
    Serial.print(" steps="); Serial.println(bladeOpenPos);
    return;
  }

  if (cmd == "SETSTRIP") {
    if (n < 2) { Serial.println("ERR SETSTRIP steps"); return; }
    bladeStripPos = (int32_t)t[1].toInt();
    Serial.print("OK SETSTRIP "); Serial.println(bladeStripPos);
    return;
  }
  if (cmd == "SETSTRIP_MM") {
    if (n < 2) { Serial.println("ERR SETSTRIP_MM mm"); return; }
    if (!hasSpmm(3)) { Serial.println("ERR NO_SPMM_M4"); return; }
    float mm = t[1].toFloat();
    bladeStripPos = mmToSteps(3, mm);
    Serial.print("OK SETSTRIP_MM mm="); Serial.print(mm, 3);
    Serial.print(" steps="); Serial.println(bladeStripPos);
    return;
  }

  if (cmd == "OPEN") {
    startMoveAbs(ax[3], bladeOpenPos, 1200, 3000);
    Serial.println("OK OPEN");
    return;
  }
  if (cmd == "CUT") {
    startMoveAbs(ax[3], bladeCutPos, 1200, 3000);
    Serial.println("OK CUT (at 0). send OPEN after if needed");
    return;
  }
  if (cmd == "STRIP") {
    startMoveAbs(ax[3], bladeStripPos, 1200, 3000);
    Serial.println("OK STRIP (at depth). send OPEN after if needed");
    return;
  }

  Serial.println("ERR UNKNOWN_CMD");
}

static void initAxisPins(Axis& a) {
  pinMode(a.stepPin, OUTPUT);
  pinMode(a.dirPin, OUTPUT);
  pinMode(a.enPin, OUTPUT);

  digitalWrite(a.stepPin, LOW);
  digitalWrite(a.dirPin, LOW);

  if (EN_ACTIVE_LOW) digitalWrite(a.enPin, HIGH);
  else              digitalWrite(a.enPin, LOW);

  a.enabled = false;
  axisStop(a);
  a.posSteps = 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {}

  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  digitalWrite(ONBOARD_LED, LOW);

  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);
  pinMode(SW4_PIN, INPUT_PULLUP);

  for (auto &a : ax) initAxisPins(a);

  // Only M1 and M4 have home switches:
  ax[0].hasSwitch = true; ax[0].swPin = SW4_PIN; ax[0].homeDir = -1; // MMU_HOME
  ax[3].hasSwitch = true; ax[3].swPin = SW3_PIN; ax[3].homeDir = -1; // CUT_HOME (closed)

  for (int i = 1; i <= MAX_CH; i++) chPos[i] = 0;

  Serial.println("OK WireBot PicoA v0.3 READY");
  Serial.println("NC switches: TRIGGERED=HIGH");
  Serial.println("Defaults: M1 SPMM=80, M4 SPMM=400");
  Serial.println("Type HELP for commands");
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool led = false;
  uint32_t nowMs = millis();
  if (nowMs - lastBlink >= 500) {
    lastBlink = nowMs;
    led = !led;
    digitalWrite(ONBOARD_LED, led ? HIGH : LOW);
  }

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = inLine;
      inLine = "";
      line.trim();
      if (line.length() > 0) handleCommand(line);
    } else {
      if (inLine.length() < 200) inLine += c;
    }
  }

  uint32_t nowUs = micros();
  for (auto &a : ax) updateAxis(a, nowUs);

  preloadUpdate();
}
