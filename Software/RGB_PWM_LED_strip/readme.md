This software example drives a plain analog RGB LED strip from the three H-bridge channels. One colour per channel

Run it with safe mode ON (safe mode led OFF). Only IN1 is used, it carries the PWM. IN2 is held low, both by the safe
mode gate in hardware and in software, so each bridge drives its output single ended and never
reverses.

The colour cycles through a fully saturated rainbow, one full sweep every 7.2s.
`led_brightness` in `src/main.c` is 0..255 and defaults to full.

PWM is 20kHz, 10 bit, on LEDC channels 0..2.

Note: This expects LED strip to have pinout {R G B +12V}. Connect each color channel to PIN 2 of 01x02 screw terminal (Out 2 of DRV887)
and 12V to pin 1 of 01x03 screw terminal (J5)
