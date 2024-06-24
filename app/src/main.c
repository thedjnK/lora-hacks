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

int main(void)
{
	int rc;
	int8_t temperature[2];
	int8_t humidity[2];
	uint8_t lora_data[6];
	uint8_t unconfirmed_packets = CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER;

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

	while (1) {
		rc = sensor_fetch_readings(temperature, humidity);

#ifdef CONFIG_ADC
		if (rc == 0) {
			rc = adc_read_internal(&voltage);
		}
#endif

		if (rc == 0) {
			lora_data[0] = temperature[0];
			lora_data[1] = temperature[1];
			lora_data[2] = humidity[0];
			lora_data[3] = humidity[1];

#ifdef CONFIG_ADC
			lora_data[4] = (voltage & 0xff00) >> 8;
			lora_data[5] = voltage & 0xff;
#endif

#if CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER > 0
			rc = lora_send_message(lora_data, sizeof(lora_data),
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
