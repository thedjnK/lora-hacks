/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>

enum led_t {
	LED_RED,
	LED_GREEN,
	LED_BLUE,

	LED_COUNT
};

/* Initialise LEDs */
int leds_init(void);

/* Turn an LED on */
void led_on(enum led_t led);

/* Turn an LED off */
void led_off(enum led_t led);

/* Blink an LED once for the specified duration */
void led_blink(enum led_t led, k_timeout_t delay);
