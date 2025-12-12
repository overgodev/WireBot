# Planned API Routes for WireBot Backend

This is a draft of the HTTP API that the Web UI will call.
Exact details may change once firmware commands are finalized.

## Base URL

- `http://<pi-address>:8000/api/…`

## Connection & Machine Info

- `GET /api/status`
  - Returns overall machine state (idle/running/error), temperatures, etc.

- `POST /api/connect`
  - (Optional) Trigger detection of Pico A & Pico B on USB serial.

## Homing & Motion

- `POST /api/home/all`
  - Homes all axes via Pico A.

- `POST /api/home/{axis}`
  - Homes a single axis (e.g. feed, cutter, strip, etc.).

## Job Configuration

- `POST /api/job/preview`
  - Accepts job parameters (length, quantity, profile) and returns a summary.
  - Used to update "Job Summary" in the Web UI before starting.

- `POST /api/job/start`
  - Starts a wire cutting job with JSON payload:
    ```json
    {
      "length_mm": 120,
      "quantity": 50,
      "feed_speed_mm_s": 80,
      "accel_mm_s2": 500,
      "profile": "normal",
      "servo_mode": "clamp",
      "fan_mode": "auto"
    }
    ```

- `POST /api/job/stop`
  - Immediate stop / emergency stop.

## IO Control (Pico B)

- `POST /api/fan`
  - Example payload:
    ```json
    { "channel": 1, "mode": "on" }
    ```

- `POST /api/servo`
  - Example payload:
    ```json
    { "id": 1, "angle": 90 }
    ```

## Logs

- `GET /api/logs`
  - Returns recent log lines from backend + firmware messages.

These routes are **not implemented yet** — this file is purely design documentation.
