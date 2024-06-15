/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include "leds.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#define LED_RED_DEVICE GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios)
#define LED_GREEN_DEVICE GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios)
#define LED_BLUE_DEVICE GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios)

static const struct gpio_dt_spec led_red = LED_RED_DEVICE;
static const struct gpio_dt_spec led_green = LED_GREEN_DEVICE;
static const struct gpio_dt_spec led_blue = LED_BLUE_DEVICE;

LOG_MODULE_REGISTER(leds, CONFIG_APP_LEDS_LOG_LEVEL);

int leds_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&led_red)) {
		LOG_ERR("Red LED not ready");
		return -EIO;
	} else if (!gpio_is_ready_dt(&led_green)) {
		LOG_ERR("Green LED not ready");
		return -EIO;
	} else if (!gpio_is_ready_dt(&led_blue)) {
		LOG_ERR("Blue LED not ready");
		return -EIO;
	}

	rc = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);

	if (rc < 0) {
		LOG_ERR("Red LED configure failed: %d", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);

	if (rc < 0) {
		LOG_ERR("Green LED configure failed: %d", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

	if (rc < 0) {
		LOG_ERR("Blue LED configure failed: %d", rc);
		return rc;
	}

	return 0;
}

static const struct gpio_dt_spec *enum_to_led(enum led_t led)
{
	switch (led)
	{
		case LED_RED:
			return &led_red;
		case LED_GREEN:
			return &led_green;
		case LED_BLUE:
			return &led_blue;
		default:
			return NULL;
	};
}

void led_on(enum led_t led)
{
	int rc;
	const struct gpio_dt_spec *led_device = enum_to_led(led);

	if (led_device == NULL) {
		LOG_ERR("NULL LED device");
		return;
	}

	rc = gpio_pin_set_dt(led_device, 1);

	if (rc < 0) {
		LOG_ERR("LED on failed: %d", rc);
	}
}

void led_off(enum led_t led)
{
	int rc;
	const struct gpio_dt_spec *led_device = enum_to_led(led);

	if (led_device == NULL) {
		LOG_ERR("NULL LED device");
		return;
	}

	rc = gpio_pin_set_dt(led_device, 0);

	if (rc < 0) {
		LOG_ERR("LED off failed: %d", rc);
	}
}

void led_blink(enum led_t led, k_timeout_t delay)
{
	int rc;
	const struct gpio_dt_spec *led_device = enum_to_led(led);

	if (led_device == NULL) {
		LOG_ERR("NULL LED device");
		return;
	}

	rc = gpio_pin_set_dt(led_device, 1);

	if (rc < 0) {
		LOG_ERR("LED on failed: %d", rc);
		return;
	}

	k_sleep(delay);

	rc = gpio_pin_set_dt(led_device, 0);

	if (rc < 0) {
		LOG_ERR("LED off failed: %d", rc);
	}

	k_sleep(delay);
}
