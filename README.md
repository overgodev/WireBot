# WireBot ⚙️  
### Web-Controlled Automated Wire Cutting Machine

WireBot is an open-source, high-precision wire cutting robot powered by:

- **Raspberry Pi Zero 2 W** (Web UI + API backend)
- **Pico A** (Real-time motion engine for 5× steppers)
- **Pico B** (Servos, fans, temperature IO)
- **TMC2209 stepper drivers**
- **24 V CNC-style power architecture**

---

## 🚀 Features

- Full **Web UI** (no physical buttons required)
- Real-time multi-stepper motion engine  
- Automatic homing with 4 endstops  
- Job configuration (length, quantity, profile)
- Servo-controlled cutter / clamp mechanisms  
- Temperature monitoring + fan SSR control  
- Live logs & progress tracking  
- Modular architecture (easy to expand)

---

## 🧠 System Architecture

| Component | Role |
|----------|------|
| **Pi Zero 2 W** | Hosts Web UI + API → sends commands to Picos via USB |
| **Pico A** | Controls 5× TMC2209 steppers + endstops (real-time motion) |
| **Pico B** | Controls 2 servos, 1–2 fans, temperature sensor |
| **24V PSU** | Powers motors + fans |
| **5V buck converter** | Powers Pi + Picos + servos |

See `/docs/system_architecture.md` for full details.

---

## 📦 Project Layout

