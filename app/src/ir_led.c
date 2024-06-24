/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "ir_led.h"

LOG_MODULE_REGISTER(ir_led, CONFIG_APP_IR_LED_LOG_LEVEL);

#define ASM_SLEEP_1X() \
	__asm("NOP")

#define ASM_SLEEP_4X() \
	ASM_SLEEP_1X(); \
	ASM_SLEEP_1X(); \
	ASM_SLEEP_1X(); \
	ASM_SLEEP_1X()

#define ASM_SLEEP_16X() \
	ASM_SLEEP_4X(); \
	ASM_SLEEP_4X(); \
	ASM_SLEEP_4X(); \
	ASM_SLEEP_4X()

#define ASM_SLEEP_64X() \
	ASM_SLEEP_16X(); \
	ASM_SLEEP_16X(); \
	ASM_SLEEP_16X(); \
	ASM_SLEEP_16X()

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(ir_led), gpios);

static void trigger_signal()
{
	/* Run 15 loops per signal */
	int rc;
	uint8_t loops = 15;

	while (loops > 0) {
		rc = gpio_pin_toggle_dt(&led);

		if (rc != 0) {
			LOG_ERR("GPIO toggle failed: %d", rc);
		}

		/* Wait 8.564us */
		ASM_SLEEP_64X();
		ASM_SLEEP_16X();
		ASM_SLEEP_16X();

		rc = gpio_pin_toggle_dt(&led);

		if (rc != 0) {
			LOG_ERR("GPIO toggle failed: %d", rc);
		}

		/* Wait 17.468us */
		ASM_SLEEP_64X();
		ASM_SLEEP_64X();
		ASM_SLEEP_64X();
		ASM_SLEEP_16X();
		ASM_SLEEP_16X();
		ASM_SLEEP_1X();

		--loops;
	}
}

static uint8_t command_ac_high_18c_move[] = {
	0b01001010,
	0b01110101,
	0b11000011,
	0b01100100,
	0b10011011,
	0b11111111,
	0b00000000,
	0b11110110,
	0b00001001,
	0b01100111,
	0b10011000
};

static uint8_t command_ac_medium_18c_move[] = {
	0b01001010,
	0b01110101,
	0b11000011,
	0b01100100,
	0b10011011,
	0b11111111,
	0b00000000,
	0b11110001,
	0b00001110,
	0b01100111,
	0b10011000
};

static uint8_t command_cooldown_2h[] = {
	0b01001010,
	0b01110101,
	0b11000011,
	0b01100100,
	0b10011011,
	0b11111011,
	0b00000100,
	0b11110001,
	0b00001110,
	0b01100111,
	0b10011000
};

static uint8_t command_off[] = {
	0b01001010,
	0b01110101,
	0b11000011,
	0b01100100,
	0b10011011,
	0b11111111,
	0b00000000,
	0b11110001,
	0b00001110,
	0b01110111,
	0b10001000
};

const static uint8_t command_size = ARRAY_SIZE(command_off);

int ir_led_send(enum AC_CMD command)
{
	int rc;
	uint8_t *data;
	uint8_t q = 0;
	uint8_t t;

	switch (command) {
		case AC_CMD_ON_AC_HIGH_18C_FAN_MOVE:
		{
			data = command_ac_high_18c_move;
			break;
		}

		case AC_CMD_ON_AC_MEDIUM_18C_FAN_MOVE:
		{
			data = command_ac_medium_18c_move;
			break;
		}

		case AC_CMD_OFF_COOLDOWN_2H:
		{
			data = command_cooldown_2h;
			break;
		}

		case AC_CMD_OFF:
		{
			data = command_off;
			break;
		}

		default:
		{
			return -EINVAL;
		}
	};

	/* Send start signal header */
	trigger_signal();
	trigger_signal();
	trigger_signal();
	trigger_signal();
	trigger_signal();
	trigger_signal();
	trigger_signal();
	trigger_signal();

	/* Wait for header space signal */
	k_busy_wait(1608);
	__asm("NOP");

	while (q < command_size)
	{
		uint8_t check = data[q];
		t = 0;

		while (t < sizeof(uint8_t))
		{
			trigger_signal();

			if ((check & 0x80) == 0)
			{
				/* 420us wait */
				k_busy_wait(404);
				ASM_SLEEP_4X();
				ASM_SLEEP_4X();
				ASM_SLEEP_1X();
				ASM_SLEEP_1X();
			}
			else
			{
				/* 1.214ms wait */
				k_busy_wait(1205);
				ASM_SLEEP_1X();
			}

			check = check << 1;

			++t;
		}

		++q;
	}

	trigger_signal();

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	return rc;
}

int ir_led_setup()
{
	if (!gpio_is_ready_dt(&led)) {
		return -EIO;
	}

	return 0;
}
