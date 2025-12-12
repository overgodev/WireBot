# Serial Protocol – Planning

The Pi will talk to Pico A and Pico B using a **simple text protocol** over USB serial.

Basic ideas (not final):

- Commands are ASCII lines terminated with `\n`.
- Each line has:
  - A command word
  - Optional arguments
- Picos respond with:
  - `OK` or `ERR <reason>`
  - Status messages as needed

Examples (possible, not final):

- To Pico A:
  - `HOME ALL`
  - `HOME FEED`
  - `MOVE M1 1000`
  - `CUT 120 50`
  - `STATUS?`

- To Pico B:
  - `SERVO 1 90`
  - `FAN 1 ON`
  - `FAN 1 AUTO`
  - `TEMP?`

The backend will act as the "brain", translating HTTP calls into these commands.
