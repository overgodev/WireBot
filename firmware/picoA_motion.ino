#include <Arduino.h>

// =====================================================
// WireBot – Pico A Motion Firmware (v0.2)
// 5× TMC2209 (STEP/DIR/EN) + 4× NC microswitch inputs
// USB Serial text commands
//
// SWITCH WIRING (IMPORTANT):
// - You said: NC to GND + INPUT_PULLUP
//   => NOT HIT: pin reads LOW (closed to GND)
//   => HIT/TRIGGERED: pin reads HIGH (opens, pulled up)
// So: TRIGGERED = HIGH
//
// Motor roles (current plan):
// M1 = MMU_BELT (channel selector belt, has home switch)
// M2 = PRELOAD_PUSH (NEMA14 pancake, pushes wire to preload switch E1)
// M3 = FEEDER (NEMA17 feed)
// M4 = BLADE (cut/strip depth motor, home switch at FULL CUT / CLOSED)
// M5 = RESERVED (2nd blade axis later / future expansion)
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

// Switch inputs (NC to GND => triggered HIGH)
#define SW1_PIN  17  // E1: PRELOAD_HIT
#define SW2_PIN  18  // E2: FEEDER_ENTRY
#define SW3_PIN  19  // E3: CUT_HOME (CLOSED / FULL CUT)
#define SW4_PIN  20  // E4: MMU_HOME (belt selector home)

#define STATUS_LED_PIN 21
#define ONBOARD_LED    25

static const uint32_t STEP_PULSE_US = 3;

// Defaults (steps/s, steps/s^2)
static const float DEFAULT_VMAX  = 2000.0f;
static const float DEFAULT_ACCEL = 4000.0f;

// Homing defaults
static const float HOME_V     = 700.0f;
static const float HOME_ACCEL = 2500.0f;
static const uint32_t HOME_TIMEOUT_MS = 20000;

// Enable polarity
static const bool EN_ACTIVE_LOW = true;

// Switch polarity (NC to GND with pullup => triggered HIGH)
static const bool SWITCH_TRIGGERED_HIGH = true;

// -----------------------------
// Channel + blade calibration storage (RAM only for now)
// -----------------------------
static const int MAX_CH = 16;
static int32_t chPos[MAX_CH + 1];   // 1..MAX_CH
static int currentCh = 0;

// Blade axis positions relative to CUT_HOME (closed/full cut) = 0
static int32_t bladeOpenPos  = 2000;  // steps away from closed (safe open)
static int32_t bladeCutPos   = 0;     // full cut (home)
static int32_t bladeStripPos = 400;   // shallow depth (example)

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

  // homing
  int swPin = -1;
  bool hasSwitch = false;
  uint32_t homeStartMs = 0;

  // homing direction (toward switch)
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

// d = v^2/(2a)
static inline float decelDistanceSteps(float v, float a) {
  if (a <= 1e-6f) return 0.0f;
  return (v * v) / (2.0f * a);
}

static Axis* findAxisByName(const String& s) {
  for (auto &a : ax) if (s == a.name) return &a;
  return nullptr;
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

  // Home timeout
  if (a.homing && (millis() - a.homeStartMs) > HOME_TIMEOUT_MS) {
    axisStop(a);
    Serial.print("ERR HOME_TIMEOUT "); Serial.println(a.name);
    return;
  }

  // If homing and already triggered, stop immediately
  if (a.homing && axisSwitchTriggered(a)) {
    axisStop(a);
    a.posSteps = 0; // home is zero
    Serial.print("OK HOME "); Serial.println(a.name);
    return;
  }

  // Speed integration
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

  // accel/decel
  if (a.v < desiredV) a.v = min(desiredV, a.v + a.accel * dt);
  else if (a.v > desiredV) a.v = max(desiredV, a.v - a.accel * dt);

  // finish snap
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

    // If moving and reached
    if (a.moving && a.posSteps == a.targetSteps) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }

    // If homing and switch triggers after a step
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

    if (a.stepIntervalUs < 200) break; // guard
  }
}

// =====================================================
// PRELOAD macro state machine (non-blocking)
// Moves M2 forward until SW1 triggers, then backoff.
// =====================================================
enum PreloadState : uint8_t { PL_IDLE=0, PL_PUSHING=1, PL_BACKOFF=2 };
static PreloadState plState = PL_IDLE;
static int32_t plMaxSteps = 0;
static int32_t plBackoff = 0;
static int32_t plStartPos = 0;
static float plV = 800.0f;
static float plA = 2500.0f;

static void preloadStart(int32_t maxSteps, int32_t backoffSteps, float vmax, float accel) {
  Axis &m2 = ax[1]; // M2
  if (!m2.enabled) { Serial.println("ERR DRIVER_DISABLED"); return; }

  plState = PL_PUSHING;
  plMaxSteps = maxSteps;
  plBackoff = backoffSteps;
  plStartPos = m2.posSteps;
  plV = vmax;
  plA = accel;

  // Push direction = + (you can flip later if needed)
  startJog(m2, +plV, plA);
  Serial.println("OK PRELOAD START");
}

static void preloadUpdate() {
  if (plState == PL_IDLE) return;

  Axis &m2 = ax[1];
  int32_t pushed = abs(m2.posSteps - plStartPos);

  if (plState == PL_PUSHING) {
    // Stop condition: SW1 triggered (HIGH) OR maxSteps reached
    if (swTriggered(SW1_PIN)) {
      axisStop(m2);
      plState = PL_BACKOFF;
      // backoff is negative direction
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
    // Wait for M2 to finish its move (handled by updateAxis)
    if (!(m2.moving || m2.jogging || m2.homing)) {
      plState = PL_IDLE;
      Serial.println("OK PRELOAD DONE");
    }
  }
}

// =====================================================
// Serial parser
// =====================================================
static String inLine;

static void printHelp() {
  Serial.println("WireBot PicoA v0.2 commands:");
  Serial.println("  HELP");
  Serial.println("  PING");
  Serial.println("  STATUS?");
  Serial.println("  SW?                         (print switch states)");
  Serial.println("  EN <0|1>");
  Serial.println("  AXEN <Mx> <0|1>");
  Serial.println("  STOP");
  Serial.println("  MOVE <Mx> <steps> [v] [a]");
  Serial.println("  GOTO <Mx> <pos> [v] [a]");
  Serial.println("  JOG  <Mx> <v> [a]");
  Serial.println("  HOME <M1|M4|ALL>            (homes MMU and/or BLADE)");
  Serial.println("  SETCH <n> <posSteps>        (store channel belt position)");
  Serial.println("  CH <n> [v] [a]              (go to channel n)");
  Serial.println("  CH?                         (print current channel + table)");
  Serial.println("  PRELOAD <maxSteps> <backoff> [v] [a]  (M2 until SW1 then backoff)");
  Serial.println("  SETOPEN <steps>             (blade open position)");
  Serial.println("  SETSTRIP <steps>            (strip depth position)");
  Serial.println("  OPEN                        (go blade open)");
  Serial.println("  CUT                         (ensure full cut closed, then open)");
  Serial.println("  STRIP                       (go strip depth, then open)");
}

static void printSwitches() {
  Serial.print("SW1_PRELOAD="); Serial.print(swTriggered(SW1_PIN) ? 1 : 0);
  Serial.print(" SW2_FEEDER="); Serial.print(swTriggered(SW2_PIN) ? 1 : 0);
  Serial.print(" SW3_CUT_HOME="); Serial.print(swTriggered(SW3_PIN) ? 1 : 0);
  Serial.print(" SW4_MMU_HOME="); Serial.println(swTriggered(SW4_PIN) ? 1 : 0);
}

static void printStatus() {
  bool busy = false;
  for (auto &a : ax) if (a.moving || a.jogging || a.homing) { busy = true; break; }

  Serial.print("STATUS "); Serial.println(busy ? "BUSY" : "IDLE");
  Serial.print("CH="); Serial.println(currentCh);

  for (auto &a : ax) {
    Serial.print(a.name);
    Serial.print(" pos="); Serial.print(a.posSteps);
    Serial.print(" en="); Serial.print(a.enabled ? 1 : 0);
    Serial.print(" mode=");
    if (a.homing) Serial.print("HOME");
    else if (a.jogging) Serial.print("JOG");
    else if (a.moving) Serial.print("MOVE");
    else Serial.print("IDLE");
    Serial.print(" v="); Serial.print(a.v, 1);
    if (a.hasSwitch) { Serial.print(" sw="); Serial.print(axisSwitchTriggered(a) ? 1 : 0); }
    Serial.println();
  }

  Serial.print("BLADE open="); Serial.print(bladeOpenPos);
  Serial.print(" strip="); Serial.print(bladeStripPos);
  Serial.print(" cut="); Serial.println(bladeCutPos);
}

static float parseFloatOr(const String& s, float fallback) {
  if (s.length() == 0) return fallback;
  return s.toFloat();
}

static void handleCommand(const String& line) {
  // Tokenize
  String t[8];
  int n = 0;
  int start = 0;
  for (int i = 0; i <= (int)line.length(); i++) {
    if (i == (int)line.length() || line[i] == ' ') {
      if (i > start) {
        if (n < 8) t[n++] = line.substring(start, i);
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
    Axis* a = findAxisByName(t[1]);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    int en = t[2].toInt();
    axisEnable(*a, en != 0);
    Serial.print("OK AXEN "); Serial.print(a->name); Serial.print(" "); Serial.println(en != 0 ? 1 : 0);
    return;
  }

  if (cmd == "MOVE" || cmd == "GOTO") {
    if (n < 3) { Serial.println("ERR MOVE/GOTO usage"); return; }
    Axis* a = findAxisByName(t[1]);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }

    int32_t val = (int32_t)t[2].toInt();
    float vmax = (n >= 4) ? parseFloatOr(t[3], DEFAULT_VMAX) : DEFAULT_VMAX;
    float acc  = (n >= 5) ? parseFloatOr(t[4], DEFAULT_ACCEL) : DEFAULT_ACCEL;

    if (cmd == "MOVE") {
      startMoveRel(*a, val, vmax, acc);
      Serial.print("OK MOVE "); Serial.print(a->name); Serial.print(" "); Serial.println(val);
    } else {
      startMoveAbs(*a, val, vmax, acc);
      Serial.print("OK GOTO "); Serial.print(a->name); Serial.print(" "); Serial.println(val);
    }
    return;
  }

  if (cmd == "JOG") {
    if (n < 3) { Serial.println("ERR JOG Mx v [a]"); return; }
    Axis* a = findAxisByName(t[1]);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    float v = t[2].toFloat();
    float acc = (n >= 4) ? parseFloatOr(t[3], DEFAULT_ACCEL) : DEFAULT_ACCEL;
    startJog(*a, v, acc);
    Serial.print("OK JOG "); Serial.print(a->name); Serial.print(" "); Serial.println(v, 1);
    return;
  }

  // HOME only for M1 and M4 (and ALL = those two)
  if (cmd == "HOME") {
    if (n < 2) { Serial.println("ERR HOME M1|M4|ALL"); return; }
    String who = t[1]; who.toUpperCase();
    if (who == "ALL") {
      startHome(ax[0]); // M1
      startHome(ax[3]); // M4
      Serial.println("OK HOME ALL");
      return;
    }
    Axis* a = findAxisByName(who);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    if (who != "M1" && who != "M4") { Serial.println("ERR HOME_ONLY_M1_M4"); return; }
    startHome(*a);
    Serial.print("OK HOME "); Serial.println(a->name);
    return;
  }

  // Channel table
  if (cmd == "SETCH") {
    if (n < 3) { Serial.println("ERR SETCH n posSteps"); return; }
    int ch = t[1].toInt();
    int32_t pos = (int32_t)t[2].toInt();
    if (ch < 1 || ch > MAX_CH) { Serial.println("ERR BAD_CH"); return; }
    chPos[ch] = pos;
    Serial.print("OK SETCH "); Serial.print(ch); Serial.print(" "); Serial.println(pos);
    return;
  }

  if (cmd == "CH?") {
    Serial.print("CH="); Serial.println(currentCh);
    for (int i = 1; i <= MAX_CH; i++) {
      if (chPos[i] != 0 || i == 1) { // show at least CH1
        Serial.print("CH"); Serial.print(i); Serial.print("="); Serial.println(chPos[i]);
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

    // Require M1 homed logically (pos=0 at SW4)
    // We don't hard-enforce, but it's recommended.
    startMoveAbs(ax[0], chPos[ch], vmax, acc);
    currentCh = ch;
    Serial.print("OK CH "); Serial.println(ch);
    return;
  }

  // PRELOAD macro (M2 until SW1 then backoff)
  if (cmd == "PRELOAD") {
    if (n < 3) { Serial.println("ERR PRELOAD maxSteps backoff [v] [a]"); return; }
    int32_t maxS = (int32_t)t[1].toInt();
    int32_t back = (int32_t)t[2].toInt();
    float vmax = (n >= 4) ? parseFloatOr(t[3], 800.0f) : 800.0f;
    float acc  = (n >= 5) ? parseFloatOr(t[4], 2500.0f) : 2500.0f;
    preloadStart(maxS, back, vmax, acc);
    return;
  }

  // Blade calibration
  if (cmd == "SETOPEN") {
    if (n < 2) { Serial.println("ERR SETOPEN steps"); return; }
    bladeOpenPos = (int32_t)t[1].toInt();
    Serial.print("OK SETOPEN "); Serial.println(bladeOpenPos);
    return;
  }
  if (cmd == "SETSTRIP") {
    if (n < 2) { Serial.println("ERR SETSTRIP steps"); return; }
    bladeStripPos = (int32_t)t[1].toInt();
    Serial.print("OK SETSTRIP "); Serial.println(bladeStripPos);
    return;
  }
  if (cmd == "SETCUT") {
    bladeCutPos = 0; // home position is full cut by definition
    Serial.println("OK SETCUT 0");
    return;
  }

  // Blade actions (M4)
  if (cmd == "OPEN") {
    startMoveAbs(ax[3], bladeOpenPos, 1200, 3000);
    Serial.println("OK OPEN");
    return;
  }
  if (cmd == "CUT") {
    // Ensure full cut (0), then open
    startMoveAbs(ax[3], bladeCutPos, 1200, 3000);
    // Opening after will be done by host or you can manually send OPEN;
    Serial.println("OK CUT (go to 0 then send OPEN)");
    return;
  }
  if (cmd == "STRIP") {
    startMoveAbs(ax[3], bladeStripPos, 1200, 3000);
    Serial.println("OK STRIP (go to depth then send OPEN)");
    return;
  }

  Serial.println("ERR UNKNOWN_CMD");
}

// =====================================================
// Setup / Loop
// =====================================================
static void initAxisPins(Axis& a) {
  pinMode(a.stepPin, OUTPUT);
  pinMode(a.dirPin, OUTPUT);
  pinMode(a.enPin, OUTPUT);

  digitalWrite(a.stepPin, LOW);
  digitalWrite(a.dirPin, LOW);

  // default disabled
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

  // Switch inputs
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);
  pinMode(SW4_PIN, INPUT_PULLUP);

  // Init axes
  for (auto &a : ax) initAxisPins(a);

  // Attach switches to only the axes that home:
  // M1 homes to SW4, M4 homes to SW3.
  ax[0].hasSwitch = true; ax[0].swPin = SW4_PIN; ax[0].homeDir = -1; // MMU_HOME
  ax[3].hasSwitch = true; ax[3].swPin = SW3_PIN; ax[3].homeDir = -1; // CUT_HOME (closed)

  // Others do not have home switches in this version
  ax[1].hasSwitch = false; // M2 uses SW1 in PRELOAD macro
  ax[2].hasSwitch = false;
  ax[4].hasSwitch = false;

  // init channel table (safe defaults)
  for (int i = 1; i <= MAX_CH; i++) chPos[i] = 0;

  Serial.println("OK WireBot PicoA v0.2 READY");
  Serial.println("NC switches: TRIGGERED=HIGH");
  Serial.println("Type HELP for commands");
}

void loop() {
  // Blink LED
  static uint32_t lastBlink = 0;
  static bool led = false;
  uint32_t nowMs = millis();
  if (nowMs - lastBlink >= 500) {
    lastBlink = nowMs;
    led = !led;
    digitalWrite(ONBOARD_LED, led ? HIGH : LOW);
  }

  // Serial lines
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = inLine;
      inLine = "";
      line.trim();
      if (line.length() > 0) handleCommand(line);
    } else {
      if (inLine.length() < 180) inLine += c;
    }
  }

  // Motion update
  uint32_t nowUs = micros();
  for (auto &a : ax) updateAxis(a, nowUs);

  // Macro update
  preloadUpdate();
}
