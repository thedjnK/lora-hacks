/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "garage.h"
#include "bluetooth.h"
#include "watchdog.h"

LOG_MODULE_REGISTER(garage, CONFIG_APP_GARAGE_LOG_LEVEL);

#define BUTTON_ALIAS DT_ALIAS(sw0)
#define GARAGE_ALIAS DT_ALIAS(garage)

#if !DT_NODE_HAS_STATUS(BUTTON_ALIAS, okay) || !DT_NODE_HAS_STATUS(GARAGE_ALIAS, okay)
#error "Missing button devicetree entries"
#endif

#define BUTTON_DEVICE GPIO_DT_SPEC_GET(BUTTON_ALIAS, gpios)
#define DOOR_DEVICE GPIO_DT_SPEC_GET(GARAGE_ALIAS, gpios)

static struct k_work button_work;
static struct gpio_callback button_cb_data;
static const struct gpio_dt_spec button = BUTTON_DEVICE;
static const struct gpio_dt_spec door = DOOR_DEVICE;

/* Door thread config */
#define DOOR_THREAD_STACK_SIZE 384
#define DOOR_THREAD_PRIORITY 1
#define DOOR_ACTIVE_TIME_MS 350

K_THREAD_STACK_DEFINE(door_thread_stack, DOOR_THREAD_STACK_SIZE);
static k_tid_t door_thread_id;
static struct k_thread door_thread;
K_SEM_DEFINE(door_sem, 0, 1);

static void button_pressed_handler(struct k_work *work)
{
	uint8_t checks = 0;

	/* Check state of button 8 times to ensure it is pressed, this is to prevent
	 * noise on the line from other sources
	 */
	while (checks < 8) {
		if (gpio_pin_get_dt(&button) == 0) {
			/* Button is not active, assume noise */
			return;
		}

		++checks;

		if (checks < 8) {
			k_sleep(K_MSEC(5));
		}
	}

	bluetooth_clear_bonds();
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}

static void door_function(void *, void *, void *)
{
	int rc;

	while (1) {
		if (k_sem_take(&door_sem, K_FOREVER) == 0) {
			rc = gpio_pin_configure_dt(&door, GPIO_OUTPUT_ACTIVE);

			if (rc != 0) {
				goto failure;
			}

			k_sleep(K_MSEC(DOOR_ACTIVE_TIME_MS));

			rc = gpio_pin_configure_dt(&door, GPIO_OUTPUT_INACTIVE);

			if (rc != 0) {
				goto failure;
			}

			k_sleep(K_MSEC(DOOR_ACTIVE_TIME_MS));
		}
	}

failure:
#if CONFIG_APP_WATCHDOG
	watchdog_fatal();
#endif
}

int garage_init(void)
{
	int rc = -ENODEV;
	k_work_init(&button_work, button_pressed_handler);

	if (!gpio_is_ready_dt(&door)) {
		LOG_ERR("Door GPIO device not ready: %s", door.port->name);
		return rc;
	}

	rc = gpio_pin_configure_dt(&door, GPIO_OUTPUT_INACTIVE);

	if (rc != 0) {
		LOG_ERR("Door output configure failed: %d:", rc);
		return rc;
	}

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Button GPIO device not ready: %s", button.port->name);
		return rc;
	}

	rc = gpio_pin_configure_dt(&button, GPIO_INPUT);

	if (rc != 0) {
		LOG_ERR("Button pin configure failed: %d:", rc);
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

	if (rc != 0) {
		LOG_ERR("Button interrupt configure failed: %d", rc);
		return rc;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	door_thread_id = k_thread_create(&door_thread, door_thread_stack,
					 K_THREAD_STACK_SIZEOF(door_thread_stack), door_function,
					 NULL, NULL, NULL, DOOR_THREAD_PRIORITY, 0, K_NO_WAIT);

	return rc;
}

void bluetooth_security_changed(void)
{
	k_sem_give(&door_sem);
}

void bluetooth_garage_characteristic_written(void)
{
	k_sem_give(&door_sem);
}

void garage_door_open_close(void)
{
	k_sem_give(&door_sem);
}
