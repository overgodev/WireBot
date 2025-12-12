#include <Arduino.h>

// =====================================================
// WireBot – Pico B IO Firmware (v0.1)
// 2× Servos + 1–2 Fan/SSR outputs + 1× Temp (ADC)
// Control via USB Serial text commands
//
// Notes:
// - Servos should be powered from your external 5V PSU (NOT from Pico 3.3V)
// - Grounds must be common: 5V PSU GND tied to Pico GND (and system common GND)
// - Fan outputs can drive SSR inputs or MOSFET gates (DC). For SSR input:
//   use an opto SSR input rating and keep current reasonable.
// - Temp sensor assumed analog divider into ADC (0..3.3V).
// =====================================================

// -----------------------------
// Pin Map (Pico B)
// -----------------------------
#define SERVO1_PIN      2   // GP2
#define SERVO2_PIN      3   // GP3

#define FAN1_PIN        4   // GP4
#define FAN2_PIN        5   // GP5 (optional)

#define TEMP_ADC_PIN    26  // GP26 / ADC0

#define IO_STATUS_LED   6   // GP6 (optional external LED)
#define OVERTEMP_LED    7   // GP7 (optional external LED)
#define ONBOARD_LED     25  // GP25

// -----------------------------
// Servo PWM settings
// -----------------------------
// Standard hobby servo: 50 Hz (20 ms period).
// Pulse width ~1000..2000 us typically maps to ~0..180 degrees.
// Adjust SERVO_MIN_US and SERVO_MAX_US to match your servo.
static const uint32_t SERVO_PERIOD_US = 20000;
static const uint16_t SERVO_MIN_US    = 500;   // safe wide range (tune if needed)
static const uint16_t SERVO_MAX_US    = 2500;

// If you want default centered:
static const int SERVO_DEFAULT_DEG = 90;

// -----------------------------
// Temperature settings (ADC interpretation)
// -----------------------------
// We will report:
// - raw ADC (0..4095)
// - voltage (0..3.3V)
// We do NOT compute real °C yet (because that depends on your sensor + divider).
//
// If later you tell me your NTC value + fixed resistor value, I can add real °C.
static const float ADC_REF_V = 3.3f;
static const int   ADC_MAX   = 4095;

// Overtemp logic (optional): compare ADC voltage to threshold.
// Disabled by default.
static bool overtempEnabled = false;
static float overtempVolt = 2.8f;      // placeholder; tune later
static bool overtempLatched = false;

// Fan control modes
enum FanMode : uint8_t { FAN_OFF=0, FAN_ON=1, FAN_AUTO=2 };

struct FanChannel {
  uint8_t pin;
  bool present;
  FanMode mode;
  bool state;          // actual output state
};

static FanChannel fan1 { FAN1_PIN, true, FAN_AUTO, false };
static FanChannel fan2 { FAN2_PIN, true, FAN_AUTO, false }; // set present=false if you only use 1 fan

// -----------------------------
// Servo state
// -----------------------------
struct ServoChan {
  uint8_t pin;
  bool present;
  int deg;             // 0..180 (logical)
  uint16_t pulseUs;    // mapped pulse
  uint32_t lastPulseStartUs;
  bool high;
};

static ServoChan s1 { SERVO1_PIN, true, SERVO_DEFAULT_DEG, 1500, 0, false };
static ServoChan s2 { SERVO2_PIN, true, SERVO_DEFAULT_DEG, 1500, 0, false };

// Serial input
static String inLine;

// =====================================================
// Helpers
// =====================================================
static inline uint16_t clampU16(int v, int lo, int hi) {
  if (v < lo) return (uint16_t)lo;
  if (v > hi) return (uint16_t)hi;
  return (uint16_t)v;
}

static inline int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static uint16_t degToPulseUs(int deg) {
  deg = clampInt(deg, 0, 180);
  // linear map deg -> pulse width
  float t = (float)deg / 180.0f;
  float us = (float)SERVO_MIN_US + t * (float)(SERVO_MAX_US - SERVO_MIN_US);
  return (uint16_t)(us + 0.5f);
}

static void setFanOutput(FanChannel &f, bool on) {
  f.state = on;
  digitalWrite(f.pin, on ? HIGH : LOW);
}

static void applyFanModes(float tempVolt) {
  // If overtemp latched -> force fans ON
  if (overtempLatched) {
    if (fan1.present) setFanOutput(fan1, true);
    if (fan2.present) setFanOutput(fan2, true);
    return;
  }

  // Manual modes
  if (fan1.present) {
    if (fan1.mode == FAN_ON) setFanOutput(fan1, true);
    else if (fan1.mode == FAN_OFF) setFanOutput(fan1, false);
  }
  if (fan2.present) {
    if (fan2.mode == FAN_ON) setFanOutput(fan2, true);
    else if (fan2.mode == FAN_OFF) setFanOutput(fan2, false);
  }

  // Auto mode
  // Simple placeholder:
  // - If tempVolt > threshold -> ON, else OFF
  // You will tune this later once you know real temperature mapping.
  const float autoOn = 2.2f;   // placeholder threshold
  const float autoOff = 2.0f;  // hysteresis
  static bool autoHot = false;

  if (!autoHot && tempVolt > autoOn) autoHot = true;
  if (autoHot && tempVolt < autoOff) autoHot = false;

  if (fan1.present && fan1.mode == FAN_AUTO) setFanOutput(fan1, autoHot);
  if (fan2.present && fan2.mode == FAN_AUTO) setFanOutput(fan2, autoHot);
}

// Non-blocking servo PWM generator (software PWM)
// We drive both servos with 50Hz pulses using micros() scheduling.
static void updateServo(ServoChan &s, uint32_t nowUs) {
  if (!s.present) return;

  // Compute where we are in the 20ms frame
  // We generate: HIGH for pulseUs, then LOW for remainder.
  static uint32_t frameStartUs = 0;
  static bool frameInit = false;

  if (!frameInit) {
    frameInit = true;
    frameStartUs = nowUs;
  }

  uint32_t frameElapsed = nowUs - frameStartUs;

  // Start a new frame if elapsed >= period
  if (frameElapsed >= SERVO_PERIOD_US) {
    frameStartUs = nowUs;
    frameElapsed = 0;
  }

  // Within frame: HIGH for pulse width, else LOW
  bool shouldHigh = (frameElapsed < s.pulseUs);

  if (shouldHigh != s.high) {
    s.high = shouldHigh;
    digitalWrite(s.pin, s.high ? HIGH : LOW);
  }
}

// Read temperature ADC (raw + voltage)
static void readTemp(int &raw, float &volt) {
  raw = analogRead(TEMP_ADC_PIN); // 0..4095
  volt = (raw / (float)ADC_MAX) * ADC_REF_V;
}

// =====================================================
// Commands
// =====================================================
static void printHelp() {
  Serial.println("WireBot PicoB v0.1 commands:");
  Serial.println("  HELP");
  Serial.println("  PING");
  Serial.println("  STATUS?");
  Serial.println("  TEMP?                 (raw + voltage)");
  Serial.println("  SERVO <1|2> <deg>      (0..180)");
  Serial.println("  SERVOUS <1|2> <us>     (pulse width in microseconds)");
  Serial.println("  FAN <1|2> <ON|OFF|AUTO>");
  Serial.println("  OVERTEMP <0|1>         (enable/disable overtemp latch)");
  Serial.println("  OVTSET <voltage>       (set overtemp voltage threshold)");
  Serial.println("  OVTCLR                 (clear overtemp latch)");
}

static void printStatus() {
  int raw; float v;
  readTemp(raw, v);

  Serial.print("STATUS ");
  Serial.print("temp_raw=");
  Serial.print(raw);
  Serial.print(" temp_v=");
  Serial.print(v, 3);

  Serial.print(" fan1=");
  Serial.print(fan1.present ? (fan1.state ? "ON" : "OFF") : "NA");
  Serial.print(" fan1mode=");
  if (fan1.mode == FAN_ON) Serial.print("ON");
  else if (fan1.mode == FAN_OFF) Serial.print("OFF");
  else Serial.print("AUTO");

  Serial.print(" fan2=");
  Serial.print(fan2.present ? (fan2.state ? "ON" : "OFF") : "NA");
  Serial.print(" fan2mode=");
  if (fan2.mode == FAN_ON) Serial.print("ON");
  else if (fan2.mode == FAN_OFF) Serial.print("OFF");
  else Serial.print("AUTO");

  Serial.print(" servo1=");
  Serial.print(s1.deg);
  Serial.print(" servo2=");
  Serial.print(s2.deg);

  Serial.print(" ovt_en=");
  Serial.print(overtempEnabled ? 1 : 0);
  Serial.print(" ovt_latch=");
  Serial.print(overtempLatched ? 1 : 0);
  Serial.print(" ovt_v=");
  Serial.println(overtempVolt, 3);
}

// tokenizes up to 6 tokens by spaces
static int tokenize(const String &line, String outTok[], int maxTok) {
  int n = 0;
  int start = 0;
  for (int i = 0; i <= (int)line.length(); i++) {
    if (i == (int)line.length() || line[i] == ' ') {
      if (i > start) {
        if (n < maxTok) outTok[n++] = line.substring(start, i);
      }
      start = i + 1;
    }
  }
  return n;
}

static ServoChan* getServoById(int id) {
  if (id == 1) return &s1;
  if (id == 2) return &s2;
  return nullptr;
}

static FanChannel* getFanById(int id) {
  if (id == 1) return &fan1;
  if (id == 2) return &fan2;
  return nullptr;
}

static void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  String tok[6];
  int n = tokenize(line, tok, 6);
  if (n == 0) return;

  String cmd = tok[0];
  cmd.toUpperCase();

  if (cmd == "HELP") { printHelp(); return; }
  if (cmd == "PING") { Serial.println("OK PONG"); return; }

  if (cmd == "STATUS?" || cmd == "STATUS") { printStatus(); return; }

  if (cmd == "TEMP?") {
    int raw; float v;
    readTemp(raw, v);
    Serial.print("TEMP raw=");
    Serial.print(raw);
    Serial.print(" v=");
    Serial.println(v, 3);
    return;
  }

  if (cmd == "SERVO") {
    if (n < 3) { Serial.println("ERR SERVO usage: SERVO <1|2> <deg>"); return; }
    int id = tok[1].toInt();
    int deg = tok[2].toInt();

    ServoChan* s = getServoById(id);
    if (!s || !s->present) { Serial.println("ERR BAD_SERVO"); return; }

    s->deg = clampInt(deg, 0, 180);
    s->pulseUs = degToPulseUs(s->deg);

    Serial.print("OK SERVO "); Serial.print(id); Serial.print(" "); Serial.println(s->deg);
    return;
  }

  if (cmd == "SERVOUS") {
    if (n < 3) { Serial.println("ERR SERVOUS usage: SERVOUS <1|2> <us>"); return; }
    int id = tok[1].toInt();
    int us = tok[2].toInt();

    ServoChan* s = getServoById(id);
    if (!s || !s->present) { Serial.println("ERR BAD_SERVO"); return; }

    s->pulseUs = clampU16(us, (int)SERVO_MIN_US, (int)SERVO_MAX_US);
    // best-effort update degrees for status (approx)
    float t = (s->pulseUs - SERVO_MIN_US) / (float)(SERVO_MAX_US - SERVO_MIN_US);
    s->deg = clampInt((int)(t * 180.0f + 0.5f), 0, 180);

    Serial.print("OK SERVOUS "); Serial.print(id); Serial.print(" "); Serial.println(s->pulseUs);
    return;
  }

  if (cmd == "FAN") {
    if (n < 3) { Serial.println("ERR FAN usage: FAN <1|2> <ON|OFF|AUTO>"); return; }
    int id = tok[1].toInt();
    String mode = tok[2]; mode.toUpperCase();

    FanChannel* f = getFanById(id);
    if (!f || !f->present) { Serial.println("ERR BAD_FAN"); return; }

    if (mode == "ON") f->mode = FAN_ON;
    else if (mode == "OFF") f->mode = FAN_OFF;
    else if (mode == "AUTO") f->mode = FAN_AUTO;
    else { Serial.println("ERR BAD_MODE"); return; }

    Serial.print("OK FAN "); Serial.print(id); Serial.print(" ");
    Serial.println(mode);
    return;
  }

  if (cmd == "OVERTEMP") {
    if (n < 2) { Serial.println("ERR OVERTEMP usage: OVERTEMP <0|1>"); return; }
    int en = tok[1].toInt();
    overtempEnabled = (en != 0);
    Serial.print("OK OVERTEMP "); Serial.println(overtempEnabled ? 1 : 0);
    return;
  }

  if (cmd == "OVTSET") {
    if (n < 2) { Serial.println("ERR OVTSET usage: OVTSET <voltage>"); return; }
    overtempVolt = tok[1].toFloat();
    Serial.print("OK OVTSET "); Serial.println(overtempVolt, 3);
    return;
  }

  if (cmd == "OVTCLR") {
    overtempLatched = false;
    digitalWrite(OVERTEMP_LED, LOW);
    Serial.println("OK OVTCLR");
    return;
  }

  Serial.println("ERR UNKNOWN_CMD");
}

// =====================================================
// Setup / Loop
// =====================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) { /* wait a bit */ }

  // Outputs
  pinMode(SERVO1_PIN, OUTPUT);
  pinMode(SERVO2_PIN, OUTPUT);
  digitalWrite(SERVO1_PIN, LOW);
  digitalWrite(SERVO2_PIN, LOW);

  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);
  digitalWrite(FAN1_PIN, LOW);
  digitalWrite(FAN2_PIN, LOW);

  pinMode(IO_STATUS_LED, OUTPUT);
  pinMode(OVERTEMP_LED, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(IO_STATUS_LED, LOW);
  digitalWrite(OVERTEMP_LED, LOW);
  digitalWrite(ONBOARD_LED, LOW);

  // ADC setup
  analogReadResolution(12); // 0..4095

  // Initialize servos to default
  s1.pulseUs = degToPulseUs(s1.deg);
  s2.pulseUs = degToPulseUs(s2.deg);

  // Initialize fans
  fan1.mode = FAN_AUTO;
  fan2.mode = FAN_AUTO;

  Serial.println("OK WireBot PicoB v0.1 READY");
  Serial.println("Type HELP for commands");
}

void loop() {
  // Blink onboard LED
  static uint32_t lastBlink = 0;
  static bool led = false;
  uint32_t nowMs = millis();
  if (nowMs - lastBlink >= 500) {
    lastBlink = nowMs;
    led = !led;
    digitalWrite(ONBOARD_LED, led ? HIGH : LOW);
  }

  // Read serial input lines
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = inLine;
      inLine = "";
      handleCommand(line);
    } else {
      if (inLine.length() < 160) inLine += c;
    }
  }

  // Read temperature and apply fan mode logic
  int raw; float tempV;
  readTemp(raw, tempV);

  // Optional overtemp latch
  if (overtempEnabled && !overtempLatched) {
    if (tempV >= overtempVolt) {
      overtempLatched = true;
      digitalWrite(OVERTEMP_LED, HIGH);
      Serial.print("ERR OVERTEMP_LATCH v=");
      Serial.println(tempV, 3);
    }
  }

  applyFanModes(tempV);

  // Status LED indicates controller is alive
  digitalWrite(IO_STATUS_LED, HIGH);

  // Update software servo PWM
  uint32_t nowUs = micros();
  updateServo(s1, nowUs);
  updateServo(s2, nowUs);
}
