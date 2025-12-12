# Pico B Pin Map – IO Controller (Planning)

Pico B handles **non-motion IO** such as servos, fans, and temperature sensing.

## Servos

| Servo | GPIO | Notes                    |
|--------|------|-------------------------|
| S1     | GP2  | Servo 1 signal output   |
| S2     | GP3  | Servo 2 signal output   |

Both pins support PWM and are suitable for 50 Hz servo control.

## Fans / SSR Outputs

These pins will drive SSR inputs or MOSFET gates (for DC fans):

| Channel | GPIO | Notes                      |
|---------|------|---------------------------|
| Fan1    | GP4  | SSR / MOSFET channel 1    |
| Fan2    | GP5  | SSR / MOSFET channel 2    |

## Temperature Sensor

A single analog temperature input (e.g. NTC in a divider):

| Signal        | GPIO | Notes              |
|---------------|------|--------------------|
| Temp sensor   | GP26 | ADC0 input         |

Additional ADC pins for future use:
- GP27 – ADC1
- GP28 – ADC2

## Status / Debug Pins

| Pin  | Function                 |
|------|--------------------------|
| GP6  | IO status LED            |
| GP7  | Over-temperature LED     |
| GP25 | Onboard LED              |

Extra digital IO (for future expansion) could use GP8–GP21 if needed.
