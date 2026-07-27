This software example drives a hobby servo on the 3 pin servo header J4

The servo sweeps end to end and back, 2s each way.

Standard 50Hz frame, 1000us to 2000us pulse. Widen `servo_min_us` and `servo_max_us` in
`src/main.c` to 500 / 2500 if your servo takes the extended range, check its datasheet first, driving
past the end stops will stall it.

PWM is 14 bit, the widest the ESP32-S3 LEDC supports. At 50Hz that is 1.22us per count, about 819
steps across the 1ms of travel.

The motors are not used, all six H-bridge inputs are held low. Safe mode only gates IN2 in hardware,
IN1 is not gated, so they are driven low rather than left floating.

The user LED mirrors the same duty, so you can see the pulse width without a scope. It is dim and
visibly flickers, that is normal, a servo pulse is only 5 to 10% duty at 50Hz.

Note: `pin_5v_pwm` (IO38) feeds both this servo header J4 and the WS2812 screw terminal J5. Only
connect one of them at a time, a strip on J5 will also see the servo pulses.

Note: IO38 is INVERTED in hardware. It goes through one SN74LVC1G00 NAND gate with the other input
tied to +3V3, which makes it an inverter. The LEDC channel sets `flags.output_invert = 1` to cancel
that out. Anything else driving this pin has to do the same, without it a 1ms pulse leaves the board
as a 19ms pulse.
