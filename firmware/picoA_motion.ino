#include <Arduino.h>

// =====================================================
// WireBot – Pico A Motion Firmware (v0.1)
// 5× TMC2209 (STEP/DIR/EN) + 4× Endstops
// Control via USB Serial text commands
//
// IMPORTANT WIRING NOTES (TMC2209 modules):
// - VMOT: 24V PSU (motor power)
// - GND: common ground (24V GND tied to 5V GND tied to Pico GND)
// - VIO: 3.3V from Pico (logic)
// - STEP/DIR/EN: from Pico GPIO
// - EN is usually ACTIVE-LOW on most breakouts
//
// ENDSTOPS:
// - Recommended: normally-closed (NC) to GND, use INPUT_PULLUP
// - Triggered => pin reads LOW
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

#define ENDSTOP1_PIN  17
#define ENDSTOP2_PIN  18
#define ENDSTOP3_PIN  19
#define ENDSTOP4_PIN  20

#define STATUS_LED_PIN 21
#define ONBOARD_LED    25

// Step pulse width (TMC2209 is fine with small pulses; keep safe)
static const uint32_t STEP_PULSE_US = 3;

// Defaults (units = steps/s, steps/s^2)
static const float DEFAULT_VMAX = 2000.0f;
static const float DEFAULT_ACCEL = 4000.0f;

// Homing defaults
static const float HOME_V = 600.0f;        // steps/s
static const float HOME_ACCEL = 2500.0f;   // steps/s^2
static const uint32_t HOME_TIMEOUT_MS = 15000;

// Enable polarity
static const bool EN_ACTIVE_LOW = true;

// Endstop polarity: NC-to-GND with pullup => triggered is LOW
static const bool ENDSTOP_TRIGGERED_LOW = true;

// =====================================================
// Axis model
// =====================================================
struct Axis {
  const char* name;

  uint8_t stepPin, dirPin, enPin;
  int32_t posSteps = 0;

  // motion command state
  bool enabled = false;
  bool moving = false;
  bool jogging = false;
  bool homing = false;

  int8_t dir = +1;                 // +1 or -1
  int32_t targetSteps = 0;         // absolute target (pos)
  float vmax = DEFAULT_VMAX;       // max speed steps/s
  float accel = DEFAULT_ACCEL;     // accel steps/s^2

  float v = 0.0f;                  // current speed (>=0)
  uint32_t lastUpdateUs = 0;        // for speed integration
  uint32_t lastStepUs = 0;          // for step timing
  uint32_t stepIntervalUs = 0;      // computed from speed

  // Jog target speed (signed)
  float jogV = 0.0f;

  // optional endstop
  int endstopPin = -1;
  bool hasEndstop = false;
  bool homed = false;
  uint32_t homeStartMs = 0;

  // internal accumulator for step scheduling
  // (simple: step at interval based on current speed)
};

// 5 axes
Axis ax[5] = {
  { "M1", M1_STEP_PIN, M1_DIR_PIN, M1_EN_PIN },
  { "M2", M2_STEP_PIN, M2_DIR_PIN, M2_EN_PIN },
  { "M3", M3_STEP_PIN, M3_DIR_PIN, M3_EN_PIN },
  { "M4", M4_STEP_PIN, M4_DIR_PIN, M4_EN_PIN },
  { "M5", M5_STEP_PIN, M5_DIR_PIN, M5_EN_PIN },
};

// Map endstops to axes (M1..M4). M5 has none by default.
static void attachEndstops() {
  ax[0].endstopPin = ENDSTOP1_PIN; ax[0].hasEndstop = true;
  ax[1].endstopPin = ENDSTOP2_PIN; ax[1].hasEndstop = true;
  ax[2].endstopPin = ENDSTOP3_PIN; ax[2].hasEndstop = true;
  ax[3].endstopPin = ENDSTOP4_PIN; ax[3].hasEndstop = true;
  ax[4].hasEndstop = false;
}

static inline bool endstopTriggered(const Axis& a) {
  if (!a.hasEndstop) return false;
  int v = digitalRead(a.endstopPin);
  return ENDSTOP_TRIGGERED_LOW ? (v == LOW) : (v == HIGH);
}

static inline void setEnablePin(bool en) {
  // global indicator LED
  digitalWrite(STATUS_LED_PIN, en ? HIGH : LOW);
}

// Enable/disable an axis driver
static void axisEnable(Axis& a, bool en) {
  a.enabled = en;
  if (EN_ACTIVE_LOW) {
    digitalWrite(a.enPin, en ? LOW : HIGH);
  } else {
    digitalWrite(a.enPin, en ? HIGH : LOW);
  }
}

static void axisStop(Axis& a) {
  a.moving = false;
  a.jogging = false;
  a.homing = false;
  a.v = 0.0f;
  a.stepIntervalUs = 0;
}

// Emit one step pulse and update position
static inline void axisStepOnce(Axis& a) {
  // Direction pin already set
  digitalWrite(a.stepPin, HIGH);
  delayMicroseconds(STEP_PULSE_US);
  digitalWrite(a.stepPin, LOW);

  a.posSteps += (a.dir > 0 ? +1 : -1);
}

static inline void axisSetDir(Axis& a, int8_t dir) {
  a.dir = (dir >= 0) ? +1 : -1;
  digitalWrite(a.dirPin, (a.dir > 0) ? HIGH : LOW);
}

static Axis* findAxisByName(const String& s) {
  for (auto &a : ax) {
    if (s == a.name) return &a;
  }
  return nullptr;
}

// =====================================================
// Motion logic
// =====================================================

// Compute a simple decel distance in steps: d = v^2 / (2a)
static inline float decelDistanceSteps(float v, float a) {
  if (a <= 1e-6f) return 0.0f;
  return (v * v) / (2.0f * a);
}

static void startMoveAbs(Axis& a, int32_t targetAbs, float vmax, float accel) {
  if (!a.enabled) {
    Serial.println("ERR DRIVER_DISABLED");
    return;
  }

  a.targetSteps = targetAbs;
  a.vmax = max(10.0f, vmax);
  a.accel = max(10.0f, accel);

  int32_t delta = a.targetSteps - a.posSteps;
  if (delta == 0) {
    Serial.println("OK ALREADY_THERE");
    return;
  }

  axisSetDir(a, (delta > 0) ? +1 : -1);
  a.moving = true;
  a.jogging = false;
  a.homing = false;
  a.v = 0.0f;
  a.lastUpdateUs = micros();
  a.lastStepUs = micros();
  a.stepIntervalUs = 0;
}

static void startMoveRel(Axis& a, int32_t deltaSteps, float vmax, float accel) {
  startMoveAbs(a, a.posSteps + deltaSteps, vmax, accel);
}

static void startJog(Axis& a, float signedSpeed, float accel) {
  if (!a.enabled) {
    Serial.println("ERR DRIVER_DISABLED");
    return;
  }
  float sp = signedSpeed;
  if (fabs(sp) < 1.0f) {
    axisStop(a);
    Serial.println("OK JOG_STOP");
    return;
  }

  axisSetDir(a, (sp >= 0.0f) ? +1 : -1);
  a.jogV = fabs(sp);
  a.accel = max(10.0f, accel);
  a.vmax = a.jogV;

  a.jogging = true;
  a.moving = false;
  a.homing = false;
  a.v = 0.0f;
  a.lastUpdateUs = micros();
  a.lastStepUs = micros();
  a.stepIntervalUs = 0;
}

static void startHome(Axis& a) {
  if (!a.hasEndstop) {
    Serial.println("ERR NO_ENDSTOP");
    return;
  }
  if (!a.enabled) {
    Serial.println("ERR DRIVER_DISABLED");
    return;
  }

  // Home direction: by default, negative direction towards endstop
  axisSetDir(a, -1);
  a.homing = true;
  a.moving = false;
  a.jogging = false;

  a.vmax = HOME_V;
  a.accel = HOME_ACCEL;
  a.v = 0.0f;
  a.homed = false;
  a.homeStartMs = millis();

  a.lastUpdateUs = micros();
  a.lastStepUs = micros();
  a.stepIntervalUs = 0;
}

static void updateAxis(Axis& a, uint32_t nowUs) {
  if (!a.enabled) return;
  if (!(a.moving || a.jogging || a.homing)) return;

  // Endstop safety during homing or if moving negative and you want hard-stop
  if (a.hasEndstop && endstopTriggered(a)) {
    if (a.homing) {
      // Homed!
      axisStop(a);
      a.posSteps = 0;
      a.homed = true;
      Serial.print("OK HOME "); Serial.println(a.name);
      return;
    } else {
      // If not homing, still stop if we hit endstop moving toward it
      if (a.dir < 0) {
        axisStop(a);
        Serial.print("ERR ENDSTOP_HIT "); Serial.println(a.name);
        return;
      }
    }
  }

  // Home timeout protection
  if (a.homing) {
    if ((millis() - a.homeStartMs) > HOME_TIMEOUT_MS) {
      axisStop(a);
      Serial.print("ERR HOME_TIMEOUT "); Serial.println(a.name);
      return;
    }
  }

  // Integrate speed at ~continuous time
  uint32_t dtUs = nowUs - a.lastUpdateUs;
  if (dtUs == 0) dtUs = 1;
  a.lastUpdateUs = nowUs;

  float dt = (float)dtUs / 1000000.0f;

  // Decide target speed based on mode
  float desiredV = a.vmax;

  if (a.moving) {
    int32_t remaining = a.targetSteps - a.posSteps;
    int32_t remAbs = abs(remaining);

    // If overshot (shouldn't happen), stop
    if ((remaining == 0) || (remAbs == 0)) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }

    // Decel planning
    float dDecel = decelDistanceSteps(a.v, a.accel);
    if ((float)remAbs <= dDecel + 1.0f) {
      // begin decelerating towards 0 as we approach target
      desiredV = 0.0f;
    } else {
      desiredV = a.vmax;
    }

    // Ensure direction still correct
    int8_t wantDir = (remaining > 0) ? +1 : -1;
    if (wantDir != a.dir) {
      // This can happen if someone edits pos externally; just fix direction
      axisSetDir(a, wantDir);
    }
  }

  if (a.jogging) {
    desiredV = a.jogV;
  }

  if (a.homing) {
    desiredV = a.vmax;
  }

  // Accel/decel to desiredV
  if (a.v < desiredV) {
    a.v = min(desiredV, a.v + a.accel * dt);
  } else if (a.v > desiredV) {
    a.v = max(desiredV, a.v - a.accel * dt);
  }

  // If we are moving and speed decelerated to ~0 and remaining is small, snap stop
  if (a.moving) {
    int32_t remAbs = abs(a.targetSteps - a.posSteps);
    if (remAbs <= 0) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }
    if (a.v < 5.0f && remAbs <= 2) {
      // finish with a couple steps max
      while (a.posSteps != a.targetSteps) {
        axisStepOnce(a);
      }
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }
  }

  // If speed is ~0 and not jogging/homing, stop
  if (!a.jogging && !a.homing && a.v <= 0.5f) {
    axisStop(a);
    Serial.print("OK DONE "); Serial.println(a.name);
    return;
  }

  // Compute step interval from speed
  float speed = max(1.0f, a.v);
  a.stepIntervalUs = (uint32_t)(1000000.0f / speed);

  // Emit steps according to timing
  while ((uint32_t)(nowUs - a.lastStepUs) >= a.stepIntervalUs) {
    a.lastStepUs += a.stepIntervalUs;

    // If moving: stop exactly at target
    if (a.moving) {
      if (a.posSteps == a.targetSteps) {
        axisStop(a);
        Serial.print("OK DONE "); Serial.println(a.name);
        return;
      }
    }

    axisStepOnce(a);

    // Endstop check after step (extra safety)
    if (a.hasEndstop && endstopTriggered(a)) {
      if (a.homing) {
        axisStop(a);
        a.posSteps = 0;
        a.homed = true;
        Serial.print("OK HOME "); Serial.println(a.name);
        return;
      } else if (a.dir < 0) {
        axisStop(a);
        Serial.print("ERR ENDSTOP_HIT "); Serial.println(a.name);
        return;
      }
    }

    // If moving, check if we reached target
    if (a.moving && a.posSteps == a.targetSteps) {
      axisStop(a);
      Serial.print("OK DONE "); Serial.println(a.name);
      return;
    }

    // If too many loops (avoid starving USB serial), break
    // This prevents lockup at extreme speeds.
    if (a.stepIntervalUs < 200) break; // ~>5000 steps/s (guard)
  }
}

// =====================================================
// Serial command parser
// =====================================================

static String inLine;

static void printHelp() {
  Serial.println("WireBot PicoA v0.1 commands:");
  Serial.println("  HELP");
  Serial.println("  PING");
  Serial.println("  STATUS?");
  Serial.println("  POS?                     (prints positions)");
  Serial.println("  EN <0|1>                  (global enable/disable drivers)");
  Serial.println("  AXEN <Mx> <0|1>           (enable one axis)");
  Serial.println("  STOP                      (stop all motion)");
  Serial.println("  MOVE <Mx> <steps> [v] [a] (relative move, steps can be negative)");
  Serial.println("  GOTO <Mx> <pos> [v] [a]   (absolute move to position)");
  Serial.println("  JOG  <Mx> <v> [a]         (continuous, v signed steps/s; 0 stops)");
  Serial.println("  HOME <Mx|ALL>             (home axis or all with endstops)");
  Serial.println("Notes:");
  Serial.println("  v=steps/s, a=steps/s^2");
  Serial.println("  Endstops stop motion if hit while moving negative.");
}

static void printStatus() {
  // Overall machine status
  bool any = false;
  for (auto &a : ax) {
    if (a.moving || a.jogging || a.homing) { any = true; break; }
  }
  Serial.print("STATUS ");
  Serial.println(any ? "BUSY" : "IDLE");
  for (auto &a : ax) {
    Serial.print(a.name);
    Serial.print(" pos=");
    Serial.print(a.posSteps);
    Serial.print(" en=");
    Serial.print(a.enabled ? 1 : 0);
    Serial.print(" mode=");
    if (a.homing) Serial.print("HOME");
    else if (a.jogging) Serial.print("JOG");
    else if (a.moving) Serial.print("MOVE");
    else Serial.print("IDLE");
    Serial.print(" v=");
    Serial.print(a.v, 1);
    Serial.print(" homed=");
    Serial.print(a.homed ? 1 : 0);
    if (a.hasEndstop) {
      Serial.print(" endstop=");
      Serial.print(endstopTriggered(a) ? 1 : 0);
    }
    Serial.println();
  }
}

static float parseFloatOr(const String& s, float fallback) {
  if (s.length() == 0) return fallback;
  return s.toFloat();
}

static long parseLongOr(const String& s, long fallback) {
  if (s.length() == 0) return fallback;
  return s.toInt();
}

static void handleCommand(const String& line) {
  // Tokenize by spaces
  String t[6];
  int n = 0;

  int start = 0;
  for (int i = 0; i <= (int)line.length(); i++) {
    if (i == (int)line.length() || line[i] == ' ') {
      if (i > start) {
        if (n < 6) t[n++] = line.substring(start, i);
      }
      start = i + 1;
    }
  }
  if (n == 0) return;

  String cmd = t[0];
  cmd.toUpperCase();

  if (cmd == "HELP") {
    printHelp();
    return;
  }
  if (cmd == "PING") {
    Serial.println("OK PONG");
    return;
  }
  if (cmd == "STATUS?" || cmd == "STATUS") {
    printStatus();
    return;
  }
  if (cmd == "POS?") {
    for (auto &a : ax) {
      Serial.print(a.name); Serial.print(" "); Serial.println(a.posSteps);
    }
    return;
  }

  if (cmd == "STOP") {
    for (auto &a : ax) axisStop(a);
    Serial.println("OK STOP");
    return;
  }

  if (cmd == "EN") {
    int en = (n >= 2) ? (int)t[1].toInt() : 0;
    for (auto &a : ax) axisEnable(a, en != 0);
    setEnablePin(en != 0);
    Serial.print("OK EN "); Serial.println(en != 0 ? 1 : 0);
    return;
  }

  if (cmd == "AXEN") {
    if (n < 3) { Serial.println("ERR AXEN usage: AXEN Mx 0|1"); return; }
    Axis* a = findAxisByName(t[1]);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    int en = (int)t[2].toInt();
    axisEnable(*a, en != 0);
    Serial.print("OK AXEN "); Serial.print(a->name); Serial.print(" "); Serial.println(en != 0 ? 1 : 0);
    return;
  }

  if (cmd == "HOME") {
    if (n < 2) { Serial.println("ERR HOME usage: HOME Mx|ALL"); return; }
    String who = t[1]; who.toUpperCase();
    if (who == "ALL") {
      // Start homing all axes that have endstops
      bool any = false;
      for (auto &a : ax) {
        if (a.hasEndstop) { startHome(a); any = true; }
      }
      Serial.println(any ? "OK HOME ALL" : "ERR NO_ENDSTOPS");
    } else {
      Axis* a = findAxisByName(t[1]);
      if (!a) { Serial.println("ERR BAD_AXIS"); return; }
      startHome(*a);
      Serial.print("OK HOME "); Serial.println(a->name);
    }
    return;
  }

  if (cmd == "MOVE" || cmd == "GOTO") {
    if (n < 3) {
      Serial.println("ERR MOVE usage: MOVE Mx steps [v] [a]  |  GOTO Mx pos [v] [a]");
      return;
    }
    Axis* a = findAxisByName(t[1]);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }

    long val = t[2].toInt();
    float vmax = (n >= 4) ? parseFloatOr(t[3], DEFAULT_VMAX) : DEFAULT_VMAX;
    float acc  = (n >= 5) ? parseFloatOr(t[4], DEFAULT_ACCEL) : DEFAULT_ACCEL;

    if (cmd == "MOVE") {
      startMoveRel(*a, (int32_t)val, vmax, acc);
      Serial.print("OK MOVE "); Serial.print(a->name); Serial.print(" "); Serial.println(val);
    } else {
      startMoveAbs(*a, (int32_t)val, vmax, acc);
      Serial.print("OK GOTO "); Serial.print(a->name); Serial.print(" "); Serial.println(val);
    }
    return;
  }

  if (cmd == "JOG") {
    if (n < 3) { Serial.println("ERR JOG usage: JOG Mx v [a]"); return; }
    Axis* a = findAxisByName(t[1]);
    if (!a) { Serial.println("ERR BAD_AXIS"); return; }
    float v = t[2].toFloat();
    float acc = (n >= 4) ? parseFloatOr(t[3], DEFAULT_ACCEL) : DEFAULT_ACCEL;
    startJog(*a, v, acc);
    Serial.print("OK JOG "); Serial.print(a->name); Serial.print(" "); Serial.println(v, 1);
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

  // Default disabled
  if (EN_ACTIVE_LOW) digitalWrite(a.enPin, HIGH);
  else digitalWrite(a.enPin, LOW);

  a.enabled = false;
  a.moving = a.jogging = a.homing = false;
  a.v = 0.0f;
  a.posSteps = 0;
  a.homed = false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) { /* wait a bit for USB */ }

  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  digitalWrite(ONBOARD_LED, LOW);

  attachEndstops();

  // Init axis pins
  for (auto &a : ax) initAxisPins(a);

  // Init endstops
  pinMode(ENDSTOP1_PIN, INPUT_PULLUP);
  pinMode(ENDSTOP2_PIN, INPUT_PULLUP);
  pinMode(ENDSTOP3_PIN, INPUT_PULLUP);
  pinMode(ENDSTOP4_PIN, INPUT_PULLUP);

  Serial.println("OK WireBot PicoA v0.1 READY");
  Serial.println("Type HELP for commands");
}

void loop() {
  // Blink onboard LED slowly when idle, fast when busy
  static uint32_t lastBlink = 0;
  static bool led = false;
  uint32_t nowMs = millis();
  bool busy = false;
  for (auto &a : ax) if (a.moving || a.jogging || a.homing) { busy = true; break; }
  uint32_t blinkPeriod = busy ? 150 : 600;
  if (nowMs - lastBlink >= blinkPeriod) {
    lastBlink = nowMs;
    led = !led;
    digitalWrite(ONBOARD_LED, led ? HIGH : LOW);
  }

  // Read serial lines
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = inLine;
      inLine = "";
      line.trim();
      if (line.length() > 0) handleCommand(line);
    } else {
      if (inLine.length() < 160) inLine += c;
    }
  }

  // Update motion
  uint32_t nowUs = micros();
  for (auto &a : ax) updateAxis(a, nowUs);
}
