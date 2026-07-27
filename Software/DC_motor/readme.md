This software example drives a DC motor on each 01x02 screw terminal channel
You can connect a motor to each channel, or only to one. remember that 3 pin terminal is for WS2812 or servo

All three channels start at 0% PWM and ramp up to 100% . Then back to 0%, to -100% and so on in
triangle wave pattern. The 100% peak is `pwm_max_percent` in `src/main.c`.

The user LED runs on the same PWM as the motors, so its brightness follows the drive level.

Holding the boot button stops the drive. The motors coast and the LED goes out until it is released,
the triangle keeps running in the background.

Note: the negative half of the triangle only turns the motors if safety DIP switch enabled. (LED high)

