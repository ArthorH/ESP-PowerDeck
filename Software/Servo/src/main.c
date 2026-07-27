#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pinout.h"

/* Hobby servo on the 3 pin servo header J4, driven from pin_5v_pwm. */

#define servo_freq_hz 50
#define servo_min_us 1000 /* widen to 500 / 2500 if your servo takes it */
#define servo_max_us 2000
#define servo_period_us (1000000 / servo_freq_hz)

#define sweep_ms 2000 /* min -> max */
#define frame_ms 20
#define sweep_steps (sweep_ms / frame_ms)

/* 14 bit is the widest the ESP32-S3 LEDC supports, at 50Hz that is 1.22us per
   count, so about 819 steps across the 1ms of servo travel. */
#define pwm_bits LEDC_TIMER_14_BIT
#define pwm_duty_max (1 << pwm_bits)

/* pin_5v_pwm goes through a single NAND gate with its other input tied to
   +3V3, which makes it an inverter, so the LEDC output has to be inverted to
   get the right pulse on the connector. The user LED is driven directly and
   mirrors the same duty without inverting. */
#define servo_channel LEDC_CHANNEL_0
#define led_channel LEDC_CHANNEL_1

static const int hbridge_in[6] = {
	pin_hbridge1_in1, pin_hbridge1_in2,
	pin_hbridge2_in1, pin_hbridge2_in2,
	pin_hbridge3_in1, pin_hbridge3_in2,
};

/* IN2 is gated by the safe mode DIP switch but IN1 is not, so drive all six
   low instead of leaving them floating. */
static void motors_off(void)
{
	uint64_t mask = 0;
	for (int i = 0; i < 6; i++) {
		mask |= 1ULL << hbridge_in[i];
	}
	gpio_config_t motors = {
		.pin_bit_mask = mask,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&motors));
	for (int i = 0; i < 6; i++) {
		ESP_ERROR_CHECK(gpio_set_level(hbridge_in[i], 0));
	}
}

static void servo_init(void)
{
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = pwm_bits,
		.timer_num = LEDC_TIMER_0,
		.freq_hz = servo_freq_hz,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ESP_ERROR_CHECK(ledc_timer_config(&timer));

	ledc_channel_config_t servo = {
		.gpio_num = pin_5v_pwm,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = servo_channel,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0,
		.flags.output_invert = 1,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&servo));

	ledc_channel_config_t led = {
		.gpio_num = pin_user_LED,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = led_channel,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0,
		.flags.output_invert = 0,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&led));
}

static void servo_set_us(uint32_t pulse_us)
{
	uint32_t duty = pulse_us * pwm_duty_max / servo_period_us;
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, servo_channel, duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, servo_channel));
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led_channel, duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led_channel));
}

void app_main(void)
{
	motors_off();
	servo_init();

	int32_t phase = 0;
	for (;;) {
		/* Triangle: 0 -> sweep_steps -> 0. */
		int32_t step = phase < sweep_steps ? phase : 2 * sweep_steps - phase;
		servo_set_us(servo_min_us + (servo_max_us - servo_min_us) * step / sweep_steps);

		phase = (phase + 1) % (2 * sweep_steps);
		vTaskDelay(pdMS_TO_TICKS(frame_ms));
	}
}
