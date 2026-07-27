#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pinout.h"

/* Plain analog RGB strip, one colour per H-bridge channel. Run with safe mode
   on: IN1 carries the PWM, IN2 stays low, so each bridge drives its output
   single ended instead of reversing. */

#define led_brightness 255 /* 0..255 */
#define hue_step 1		   /* degrees of hue per frame */
#define frame_ms 20

#define pwm_freq_hz 20000
#define pwm_duty_full ((1 << LEDC_TIMER_10_BIT) - 1)

/* Channel order is red, green, blue. */
static const int rgb_in1[3] = {pin_hbridge1_in1, pin_hbridge2_in1, pin_hbridge3_in1};
static const int rgb_in2[3] = {pin_hbridge1_in2, pin_hbridge2_in2, pin_hbridge3_in2};

/* Safe mode already forces IN2 low in hardware, this keeps it low even if the
   DIP switch is off so the bridge can never reverse. */
static void in2_low(void)
{
	uint64_t mask = 0;
	for (int i = 0; i < 3; i++) {
		mask |= 1ULL << rgb_in2[i];
	}
	gpio_config_t in2 = {
		.pin_bit_mask = mask,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&in2));
	for (int i = 0; i < 3; i++) {
		ESP_ERROR_CHECK(gpio_set_level(rgb_in2[i], 0));
	}
}

static void user_led_off(void)
{
	gpio_config_t led = {
		.pin_bit_mask = 1ULL << pin_user_LED,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&led));
	ESP_ERROR_CHECK(gpio_set_level(pin_user_LED, 0));
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
		ledc_channel_config_t channel = {
			.gpio_num = rgb_in1[i],
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.channel = i,
			.intr_type = LEDC_INTR_DISABLE,
			.timer_sel = LEDC_TIMER_0,
			.duty = 0,
			.hpoint = 0,
		};
		ESP_ERROR_CHECK(ledc_channel_config(&channel));
	}
}

/* Fully saturated rainbow, hue in 0..359. */
static void hue_to_rgb(uint16_t hue, uint8_t rgb[3])
{
	uint8_t up = (hue % 60) * 255 / 60;
	uint8_t down = 255 - up;

	switch (hue / 60) {
	case 0:  rgb[0] = 255;  rgb[1] = up;   rgb[2] = 0;    break;
	case 1:  rgb[0] = down; rgb[1] = 255;  rgb[2] = 0;    break;
	case 2:  rgb[0] = 0;    rgb[1] = 255;  rgb[2] = up;   break;
	case 3:  rgb[0] = 0;    rgb[1] = down; rgb[2] = 255;  break;
	case 4:  rgb[0] = up;   rgb[1] = 0;    rgb[2] = 255;  break;
	default: rgb[0] = 255;  rgb[1] = 0;    rgb[2] = down; break;
	}
}

void app_main(void)
{
	in2_low();
	user_led_off();
	pwm_init();

	uint16_t hue = 0;
	for (;;) {
		uint8_t rgb[3];
		hue_to_rgb(hue, rgb);

		for (int i = 0; i < 3; i++) {
			uint32_t duty = (uint32_t)rgb[i] * led_brightness * pwm_duty_full / (255 * 255);
			ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, i, duty));
			ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, i));
		}

		hue = (hue + hue_step) % 360;
		vTaskDelay(pdMS_TO_TICKS(frame_ms));
	}
}
