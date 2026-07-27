#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pinout.h"

/* Peak duty of the triangle wave, in percent, in both directions. */
#define pwm_max_percent 100

#define pwm_freq_hz 20000 /* above audible, well inside the DRV8870 limit */
#define pwm_duty_full ((1 << LEDC_TIMER_10_BIT) - 1)
#define pwm_duty_peak (pwm_duty_full * pwm_max_percent / 100)

#define ramp_ms 3000 /* 0 -> peak, one quarter of the triangle period */
#define step_ms 20
#define ramp_steps (ramp_ms / step_ms)

/* LEDC channels 0..5 are the three H-bridges, channel 6 mirrors them on the LED. */
static const int hbridge_in[3][2] = {
	{pin_hbridge1_in1, pin_hbridge1_in2},
	{pin_hbridge2_in1, pin_hbridge2_in2},
	{pin_hbridge3_in1, pin_hbridge3_in2},
};
#define led_channel LEDC_CHANNEL_6

static void pwm_set(ledc_channel_t channel, uint32_t duty)
{
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
}

static void pwm_init(void)
{
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = LEDC_TIMER_10_BIT,
		.timer_num = LEDC_TIMER_0,
		.freq_hz = pwm_freq_hz,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ESP_ERROR_CHECK(ledc_timer_config(&timer));

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			ledc_channel_config_t channel = {
				.gpio_num = hbridge_in[i][j],
				.speed_mode = LEDC_LOW_SPEED_MODE,
				.channel = 2 * i + j,
				.intr_type = LEDC_INTR_DISABLE,
				.timer_sel = LEDC_TIMER_0,
				.duty = 0,
				.hpoint = 0,
			};
			ESP_ERROR_CHECK(ledc_channel_config(&channel));
		}
	}

	ledc_channel_config_t led = {
		.gpio_num = pin_user_LED,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = led_channel,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&led));
}

/* Positive duty drives IN1, negative drives IN2, the idle input stays low so
   the off time of the cycle coasts. */
static void motors_set(int32_t duty)
{
	for (int i = 0; i < 3; i++) {
		pwm_set(2 * i, duty > 0 ? duty : 0);
		pwm_set(2 * i + 1, duty < 0 ? -duty : 0);
	}
}

/* Triangle over a 4 * ramp_steps period, returns -ramp_steps..+ramp_steps. */
static int32_t triangle(int32_t phase)
{
	if (phase < ramp_steps) {
		return phase;
	}
	if (phase < 3 * ramp_steps) {
		return 2 * ramp_steps - phase;
	}
	return phase - 4 * ramp_steps;
}

void app_main(void)
{
	gpio_config_t button = {
		.pin_bit_mask = 1ULL << pin_button_boot,
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&button));

	pwm_init();

	int32_t phase = 0;
	for (;;) {
		int32_t duty = triangle(phase) * pwm_duty_peak / ramp_steps;

		/* Button is active low, hold it to stop the drive. */
		if (gpio_get_level(pin_button_boot) == 0) {
			duty = 0;
		}

		motors_set(duty);
		pwm_set(led_channel, duty < 0 ? -duty : duty);

		phase = (phase + 1) % (4 * ramp_steps);
		vTaskDelay(pdMS_TO_TICKS(step_ms));
	}
}
