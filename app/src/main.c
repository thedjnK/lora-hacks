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

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define SENSOR_READING_TIME K_SECONDS(2 * 60 + 30)
#define CONFIRMED_PACKET_ATTEMPTS 20
#define SEND_ATTEMPTS 3

int main(void)
{
	int rc;
	int8_t temperature[2];
	int8_t humidity;
	uint8_t lora_data[3];
	uint8_t unconfirmed_packets = CONFIRMED_PACKET_ATTEMPTS;

	(void)leds_init();
	lora_keys_load();
	rc = sensor_setup();

	if (rc != 0) {
		LOG_ERR("Sensor setup failed: cannot continue");
		return 0;
	}

	rc = lora_setup();

	if (rc != 0) {
		LOG_ERR("LoRa setup failed: cannot continue");
		return 0;
	}

	while (1) {
		rc = sensor_fetch_readings(temperature, &humidity);

		if (rc == 0) {
			lora_data[0] = temperature[0];
			lora_data[1] = temperature[1];
			lora_data[2] = humidity;

			rc = lora_send_message(lora_data, sizeof(lora_data), (unconfirmed_packets == CONFIRMED_PACKET_ATTEMPTS ? true : false), SEND_ATTEMPTS);

			if (unconfirmed_packets == CONFIRMED_PACKET_ATTEMPTS) {
				unconfirmed_packets = 0;
			} else {
				++unconfirmed_packets;
			}

			if (rc == 0) {
				LOG_INF("Reading sent");
			} else {
				LOG_ERR("Reading failed to send: %d", rc);
			}
		} else {
			LOG_ERR("Failed to fetch sensor readings");
		}

		k_sleep(SENSOR_READING_TIME);
	}
}
