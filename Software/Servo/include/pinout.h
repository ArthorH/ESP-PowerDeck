#pragma once

/*
Pinout imported from the KiCad project (PCB source/, U22 = ESP32-S3-WROOM-1).
Values are raw GPIO numbers.
*/

/* --- User I/O --------------------------------------------------------- */
#define pin_user_dip_switch 1 // channel 1 of the safety dip switch
#define pin_user_LED 2	      // active HIGH
#define pin_button_boot 15    // active LOW, the only button wired to the MCU

/* --- H-bridges (DRV8870, one INA219 current monitor each) -------------- */
#define pin_hbridge1_in1 17
#define pin_hbridge1_in2 18
#define pin_hbridge2_in1 21
#define pin_hbridge2_in2 35
#define pin_hbridge3_in1 36
#define pin_hbridge3_in2 37

/* --- I2C1: internal bus, the three INA219 monitors --------------------- */
#define pin_i2c1_scl 8
#define pin_i2c1_sda 7

/* --- I2C2: external, header J2 (1:+3V3 2:SCL 3:SDA 4:GND) -------------- */
#define pin_i2c2_scl 5
#define pin_i2c2_sda 4

/* --- SPI: external, header J6 (1:+3V3 2:CS 3:MOSI 4:MISO 5:SCK 6:GND) -- */
#define pin_spi_cs 10
#define pin_spi_mosi 11
#define pin_spi_sck 12
#define pin_spi_miso 13

/* --- 5V data/PWM: drives both J4 (servo, +5V)
	   and J5 (WS2812 screw terminal, +12V) from this one pin ------------ */
#define pin_5v_pwm 38

/* --- Native USB (to SL2.1A hub) --------------------------------------- */
#define pin_usb_dm 19
#define pin_usb_dp 20

/* --- UART0 to the CH340 programmer ------------------------------------ */
#define pin_uart0_tx 43
#define pin_uart0_rx 44

/*
Notes:
- No I2C pullups are fitted on either bus - enable the internal ones or add
  pullups on the external device.
- DIP switch channel 1 reads back on IO1. Channel 2 (~SAFE_OPERATION) is not
  readable, it gates the H-bridge IN2 lines in hardware.
- The other push buttons are not wired to the MCU, only BOOT on IO15 is.
*/
