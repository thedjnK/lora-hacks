/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "settings.h"
#include "sensor.h"
#include "lora.h"
#include "leds.h"
#include "adc.h"
#include "ir_led.h"
#include "peripherals.h"
#include "bluetooth.h"
#include "app_version.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

#define SENSOR_READING_TIME K_SECONDS(CONFIG_APP_SENSOR_READING_TIME)
#define SEND_ATTEMPTS 3

enum lora_uplink_types {
	LORA_UPLINK_TYPE_STARTUP,
	LORA_UPLINK_TYPE_READINGS,
};

enum lora_downlink_types {
	LORA_DOWNLINK_TYPE_IR,
};

int main(void)
{
	int rc;
	uint16_t application_type = CONFIG_APP_TYPE;
	int8_t temperature[2];
	int8_t humidity[2];
	uint8_t lora_data[8];
	uint8_t unconfirmed_packets = CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER;
	uint8_t data_size;

#ifdef CONFIG_ADC
	uint16_t voltage;
#endif

	LOG_INF("Application version %s, built " __DATE__, APP_VERSION_EXTENDED_STRING);

	peripheral_setup();
	(void)leds_init();
	lora_keys_load();
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

	rc = lora_setup();

	if (rc != 0) {
		LOG_ERR("LoRa setup failed: cannot continue");
		return 0;
	}

#ifdef CONFIG_BT
	rc = bluetooth_init();

	if (rc != 0) {
		LOG_ERR("Bluetooth setup failed: cannot continue");
	}
#endif

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

	while (1) {
		rc = sensor_fetch_readings(temperature, humidity);

#ifdef CONFIG_ADC
		if (rc == 0) {
			rc = adc_read_internal(&voltage);
		}
#endif

		if (rc == 0) {
			data_size = 0;
			lora_data[data_size++] = LORA_UPLINK_TYPE_READINGS;
			lora_data[data_size++] = temperature[0];
			lora_data[data_size++] = temperature[1];
			lora_data[data_size++] = humidity[0];
			lora_data[data_size++] = humidity[1];

#ifdef CONFIG_ADC
			lora_data[data_size++] = (voltage & 0xff00) >> 8;
			lora_data[data_size++] = voltage & 0xff;
#endif

#if CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER > 0
			rc = lora_send_message(lora_data, data_size,
					       (unconfirmed_packets ==
						CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER ? true :
						false), SEND_ATTEMPTS);

			if (unconfirmed_packets == CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER) {
				unconfirmed_packets = 0;
			} else {
				++unconfirmed_packets;
			}
#else
			rc = lora_send_message(lora_data, sizeof(lora_data), false, SEND_ATTEMPTS);
#endif

			if (rc == 0) {
				LOG_INF("Reading sent");
			} else {
				LOG_ERR("Reading failed to send: %d", rc);
			}
		} else {
			LOG_ERR("Failed to fetch sensor readings or ADC value");
		}

		k_sleep(SENSOR_READING_TIME);
	}
}

#ifdef CONFIG_APP_LORA_ALLOW_DOWNLINKS
void lora_message_callback(uint8_t port, const uint8_t *data, uint8_t len)
{
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
			default:
			{
				LOG_ERR("No handler for LoRa Downlink type %d", data[0]);
			}
		};
	}
}
#endif
