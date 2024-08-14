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
#include "app_version.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

#define SENSOR_READING_TIME K_SECONDS(CONFIG_APP_SENSOR_READING_TIME)
#define SEND_ATTEMPTS 3
#define ADC_OFFSET_DEFAULT_MV 500

enum lora_uplink_types {
	LORA_UPLINK_TYPE_STARTUP,
	LORA_UPLINK_TYPE_READINGS,
	LORA_UPLINK_TYPE_ERROR_READINGS,
	LORA_UPLINK_TYPE_ERROR_ADC,
	LORA_UPLINK_TYPE_ERROR_NO_HANDLER,
};

enum lora_downlink_types {
	LORA_DOWNLINK_TYPE_IR,
	LORA_DOWNLINK_TYPE_SETTING,
	LORA_DOWNLINK_TYPE_GARAGE,
	LORA_DOWNLINK_TYPE_BLUETOOTH,
	LORA_DOWNLINK_TYPE_DEVICE,
};

enum device_command_op_t {
	DEVICE_COMMAND_OP_REBOOT,
	DEVICE_COMMAND_OP_CLEAR_SETTINGS,
	DEVICE_COMMAND_OP_BLINK_LED,

	DEVICE_COMMAND_OP_COUNT,
};

int main(void)
{
	int rc;
	uint16_t application_type = CONFIG_APP_TYPE;
	int8_t temperature[2];
	int8_t humidity[2];
	uint8_t lora_data[8];
	uint8_t data_size;

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

	/* Enable HFCLK for better timing during setup */
	(void)hfclk_enable();

	rc = lora_setup();

	if (rc != 0) {
		LOG_ERR("LoRa setup failed: cannot continue");
		return 0;
	}

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
	} else {
		LOG_ERR("Connect message failed to send: %d", rc);
	}

	/* Skip enabling clock for the first iteration as it is already enabled */
	goto first_start;

	while (1) {
		(void)hfclk_enable();

first_start:
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
			LOG_INF("Message sent");
		} else {
			LOG_ERR("Message failed to send: %d", rc);
		}

		(void)hfclk_disable();

#ifdef CONFIG_APP_WATCHDOG
		watchdog_feed();
#endif

		k_sleep(SENSOR_READING_TIME);
	}
}

#ifdef CONFIG_APP_LORA_ALLOW_DOWNLINKS
static int device_command(enum device_command_op_t op)
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
	int rc;

	if (port == 1) {
		if (len == 0) {
			LOG_ERR("Received 0 byte download on port 1");
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

			case LORA_DOWNLINK_TYPE_SETTING:
			{
				setting_lora(data[1], &data[2], (len - 2));
				break;
			}

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
				device_command(data[1]);
				break;
			}

			default:
			{
				LOG_ERR("No handler for LoRa Downlink type %d", data[0]);

				response[response_size++] = LORA_UPLINK_TYPE_ERROR_NO_HANDLER;
				response[response_size++] = data[0];

				(void)hfclk_enable();

				rc = lora_send_message(response, sizeof(response), false, SEND_ATTEMPTS);

				if (rc == 0) {
					LOG_INF("Message sent");
				} else {
					LOG_ERR("Message failed to send: %d", rc);
				}

				(void)hfclk_disable();
			}
		};
	}
}
#endif
