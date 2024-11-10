/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys_clock.h>
#include "settings.h"
#include "sensor.h"
#include "lora.h"
#include "leds.h"
#include "adc.h"
#include "ir_led.h"
#include "peripherals.h"
#include "bluetooth.h"
#include "garage.h"
#include "hfclk.h"
#include "watchdog.h"
#include "error_messages.h"
#include "app_version.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

#define SENSOR_READING_TIME_MIN 30
#define SENSOR_READING_TIME_MAX 7200
#define SEND_ATTEMPTS 3
#define ADC_OFFSET_DEFAULT_MV 500

enum lora_uplink_types {
	LORA_UPLINK_TYPE_STARTUP,
	LORA_UPLINK_TYPE_READINGS,
	LORA_UPLINK_TYPE_ERROR_READINGS,
	LORA_UPLINK_TYPE_ERROR_ADC,
	LORA_UPLINK_TYPE_ERROR_NO_HANDLER,
	LORA_UPLINK_TYPE_UPTIME,
};

enum lora_downlink_types {
	LORA_DOWNLINK_TYPE_IR,
	LORA_DOWNLINK_TYPE_UNUSED,
	LORA_DOWNLINK_TYPE_GARAGE,
	LORA_DOWNLINK_TYPE_BLUETOOTH,
	LORA_DOWNLINK_TYPE_DEVICE,
};

enum device_command_op_t {
	DEVICE_COMMAND_OP_REBOOT,
	DEVICE_COMMAND_OP_CLEAR_SETTINGS,
	DEVICE_COMMAND_OP_BLINK_LED,
	DEVICE_COMMAND_OP_GET_UPTIME,
	DEVICE_COMMAND_OP_SET_SENSOR_INTEVAL,

	DEVICE_COMMAND_OP_COUNT,
};

enum send_flags_t {
	SEND_FLAG_UPTIME = 0x01,
};

static void sensor_timer_handler(struct k_timer *dummy);

static uint16_t sensor_reading_time = CONFIG_APP_DEFAULT_SENSOR_READING_TIME;
static uint8_t send_flags = 0;
static K_SEM_DEFINE(send_message_sem, 1, 2);
static K_TIMER_DEFINE(sensor_timer, sensor_timer_handler, NULL);

static void sensor_timer_handler(struct k_timer *dummy)
{
	k_sem_give(&send_message_sem);
}

void lora_message_added(void)
{
	k_sem_give(&send_message_sem);
}

int main(void)
{
	int rc;
	uint16_t application_type = CONFIG_APP_TYPE;
	int8_t temperature[2];
	int8_t humidity[2];
	uint8_t lora_data[8];
	uint8_t data_size;
	uint8_t failed_messages = 0;
	bool lora_joined = false;
	bool lora_sent_join_message = false;

#ifdef CONFIG_ADC
	uint16_t voltage;
#endif

	LOG_INF("Application version %s, built " __DATE__, APP_VERSION_EXTENDED_STRING);

	peripheral_setup();
	(void)leds_init();
	lora_keys_load();
	app_keys_load();

#ifdef CONFIG_APP_GARAGE_DOOR
	garage_init();
#endif

#ifdef CONFIG_BT
	rc = bluetooth_init();

	if (rc != 0) {
		LOG_ERR("Bluetooth setup failed");
	}
#endif

	rc = sensor_setup();

#ifdef CONFIG_ADC
	adc_setup();
#endif

	if (rc != 0) {
		LOG_ERR("Sensor setup failed: cannot continue");
		return 0;
	}

#ifdef CONFIG_APP_IR_LED
	rc = ir_led_setup();

	if (rc != 0) {
		LOG_ERR("IR LED setup failed: cannot continue");
		return 0;
	}
#endif

	/* LED flashing to indicate start up */
	led_on(LED_RED);
	k_sleep(K_MSEC(300));
	led_off(LED_RED);
	led_on(LED_GREEN);
	k_sleep(K_MSEC(300));
	led_off(LED_GREEN);
	led_on(LED_BLUE);
	k_sleep(K_MSEC(300));
	led_off(LED_BLUE);

	while (1) {
		uint8_t i = 0;
		uint8_t l;

		k_sem_take(&send_message_sem, K_FOREVER);
		(void)hfclk_enable();

		if (lora_joined == false) {
			rc = lora_setup();

			if (rc == 0) {
#ifdef CONFIG_APP_WATCHDOG
				watchdog_feed();
#endif
				lora_joined = true;
			} else {
				goto wait;
			}
		}

		if (lora_sent_join_message == false) {
			/* Send connect message with version and application type */
			data_size = 0;
			lora_data[data_size++] = LORA_UPLINK_TYPE_STARTUP;
			lora_data[data_size++] = APP_VERSION_MAJOR;
			lora_data[data_size++] = APP_VERSION_MINOR;
			lora_data[data_size++] = APP_PATCHLEVEL;
			lora_data[data_size++] = APP_TWEAK;
			lora_data[data_size++] = ((uint8_t *)&application_type)[0];
			lora_data[data_size++] = ((uint8_t *)&application_type)[1];

			rc = lora_send_message(lora_data, data_size, true, SEND_ATTEMPTS);

			if (rc == 0) {
				LOG_INF("Connect message sent");
				lora_sent_join_message = true;
			} else {
				LOG_ERR("Connect message failed to send: %d", rc);
				++failed_messages;
				goto wait;
			}
		}

		if (send_flags & SEND_FLAG_UPTIME) {
			/* Send device uptime */
			uint32_t uptime = (uint32_t)(k_uptime_get() / MSEC_PER_SEC);

			lora_data[0] = LORA_UPLINK_TYPE_UPTIME;
			memcpy(&lora_data[1], &uptime, sizeof(uptime));

			rc = lora_send_message(lora_data, 5, false, SEND_ATTEMPTS);

			if (rc == 0) {
				LOG_INF("Message sent");
				send_flags &= ~SEND_FLAG_UPTIME;
			} else {
				LOG_ERR("Message failed to send: %d", rc);
				++failed_messages;
			}
		}

		/* Check for any error mesages to report */
		l = error_message_get_count();

		if (l > 0) {
			const struct error_message_holder_t *errors = error_message_get_array();

			error_message_lock();

			while (i < l) {
				rc = lora_send_message((errors[i].data_size == 0 ? NULL :
							errors[i].data), errors[i].data_size,
						       false, SEND_ATTEMPTS);

				++i;
			}

			error_message_clear();
			error_message_unlock();

			/* Ensure that there is a pending timer event or semaphore count so this
			 * runs again
			 */
			if (k_timer_remaining_get(&sensor_timer) == 0 &&
			    k_sem_count_get(&send_message_sem) == 0) {
				/* Something has been missed, restart the timer */
				goto restart_timer;
			}
		}

		data_size = 0;

		rc = sensor_fetch_readings(temperature, humidity);

		if (rc != 0) {
			lora_data[data_size++] = LORA_UPLINK_TYPE_ERROR_READINGS;
		}

#ifdef CONFIG_ADC
		if (rc == 0) {
			rc = adc_read_internal(&voltage);

			if (rc != 0) {
				lora_data[data_size++] = LORA_UPLINK_TYPE_ERROR_ADC;
			} else {
#ifdef CONFIG_APP_EXTERNAL_DCDC
				int16_t adc_offset;

				rc = settings_runtime_get("app/power_offset", (uint8_t *)&adc_offset, sizeof(adc_offset));

				if (rc != sizeof(adc_offset) || adc_offset == 0) {
					/* No offset, use default */
					adc_offset = ADC_OFFSET_DEFAULT_MV;
				}

				voltage += adc_offset;

				rc = 0;
#endif
			}
		}
#endif

		if (rc == 0) {
			lora_data[data_size++] = LORA_UPLINK_TYPE_READINGS;
			lora_data[data_size++] = temperature[0];
			lora_data[data_size++] = temperature[1];
			lora_data[data_size++] = humidity[0];
			lora_data[data_size++] = humidity[1];

#ifdef CONFIG_ADC
			lora_data[data_size++] = (voltage & 0xff00) >> 8;
			lora_data[data_size++] = voltage & 0xff;
#else
			lora_data[data_size++] = 0xff;
			lora_data[data_size++] = 0xff;
#endif
		} else {
			LOG_ERR("Failed to fetch sensor readings or ADC value");

			/* Uplink type is already set, append error code */
			lora_data[data_size++] = rc & 0xff;
		}

		rc = lora_send_message(lora_data, data_size, false, SEND_ATTEMPTS);

		if (rc == 0) {
#ifdef CONFIG_APP_WATCHDOG
			watchdog_feed();
#endif
			LOG_INF("Message sent");
		} else {
			LOG_ERR("Message failed to send: %d", rc);
			++failed_messages;
		}

wait:
		(void)hfclk_disable();

		if (failed_messages > CONFIG_APP_LORA_RECONNECT_FAILED_PACKETS) {
			/* No successful messages after a period of time, consider connection dead
			 * and reconnect
			 */
			failed_messages = 0;
			lora_joined = false;
			lora_sent_join_message = false;
		}

restart_timer:
		k_timer_start(&sensor_timer, K_SECONDS(sensor_reading_time), K_NO_WAIT);
	}
}

#ifdef CONFIG_APP_LORA_ALLOW_DOWNLINKS
static int device_command(const enum device_command_op_t op, const uint8_t *data, const uint8_t data_size)
{
	switch (op) {
		case DEVICE_COMMAND_OP_REBOOT:
		{
			sys_reboot(SYS_REBOOT_COLD);
			break;
		}
		case DEVICE_COMMAND_OP_CLEAR_SETTINGS:
		{
			const struct flash_area *fap;
			int rc = flash_area_open(FIXED_PARTITION_ID(storage_partition), &fap);

			if (rc < 0) {
				return -ENOENT;
			} else {
				rc = flash_area_flatten(fap, 0, fap->fa_size);

				if (rc < 0) {
					return -EIO;
				}

				flash_area_close(fap);
			}

			break;
		}
		case DEVICE_COMMAND_OP_BLINK_LED:
		{
			led_on(LED_RED);
			led_on(LED_GREEN);
			led_on(LED_BLUE);
			k_sleep(K_SECONDS(3));
			led_off(LED_RED);
			led_off(LED_GREEN);
			led_off(LED_BLUE);
			break;
		}
		case DEVICE_COMMAND_OP_GET_UPTIME:
		{
			send_flags |= SEND_FLAG_UPTIME;
			break;
		}
		case DEVICE_COMMAND_OP_SET_SENSOR_INTEVAL:
		{
			const uint16_t *reading_time = (uint16_t *)data;

			if (data_size != sizeof(uint16_t)) {
				return -EINVAL;
			} else if (*reading_time < SENSOR_READING_TIME_MIN || *reading_time > SENSOR_READING_TIME_MAX) {
				return -EINVAL;
			}

			sensor_reading_time = *reading_time;
			break;
		}
		default:
		{
			return -EINVAL;
		}
	};

	return 0;
}

void lora_message_callback(uint8_t port, const uint8_t *data, uint8_t len)
{
	uint8_t response[2];
	uint8_t response_size = 0;

	if (port == 1) {
		if (len == 0) {
			LOG_DBG("Received 0 byte download on port 1");
			return;
		}

		switch (data[0]) {
#ifdef CONFIG_APP_IR_LED
			case LORA_DOWNLINK_TYPE_IR:
			{
				(void)ir_led_send(data[1]);
				break;
			}
#endif

#ifdef CONFIG_APP_GARAGE_DOOR
			case LORA_DOWNLINK_TYPE_GARAGE:
			{
				garage_door_open_close();
				break;
			}
#endif

#ifdef CONFIG_BT
			case LORA_DOWNLINK_TYPE_BLUETOOTH:
			{
				bluetooth_remote(data[1]);
				break;
			}
#endif

			case LORA_DOWNLINK_TYPE_DEVICE:
			{
				device_command(data[1], &data[2], (len - 2));
				break;
			}

			default:
			{
				LOG_ERR("No handler for LoRa Downlink type %d", data[0]);

				response[response_size++] = LORA_UPLINK_TYPE_ERROR_NO_HANDLER;
				response[response_size++] = data[0];

				error_message_lock();
				error_message_add_error(response, response_size);
				error_message_unlock();
			}
		};
	}
}
#endif
