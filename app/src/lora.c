/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "lora.h"
#include "settings.h"
#include "leds.h"
#include "watchdog.h"

#define LORA_JOIN_FAIL_LED_BLINK_TIME K_MSEC(750)
#define LORA_JOIN_FAIL_DELAY K_SECONDS(30)
#define LORA_SEND_FAIL_DELAY K_SECONDS(5)

#define LORA_JOIN_ATTEMPTS 64

#define LORA_DEVICE DEVICE_DT_GET(DT_ALIAS(lora0))

LOG_MODULE_REGISTER(lora, CONFIG_APP_LORA_LOG_LEVEL);

#if CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER > 0
static uint8_t unconfirmed_packets = CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER;
#endif

#ifdef CONFIG_APP_LORA_ALLOW_DOWNLINKS
static void lora_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len,
			  const uint8_t *hex_data);

static struct lorawan_downlink_cb downlink_cb = {
	.port = 1,
	.cb = lora_downlink
};

static void lora_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len,
			  const uint8_t *hex_data)
{
	lora_message_callback(port, hex_data, len);
}
#endif

int lora_setup(void)
{
	int rc;
	uint16_t join_attempts = 0;
	const struct device *lora_dev = LORA_DEVICE;
	uint8_t dev_eui[LORA_DEV_EUI_SIZE] = { 0 };
	uint8_t join_eui[LORA_JOIN_EUI_SIZE] = { 0 };
	uint8_t app_key[LORA_APP_KEY_SIZE] = { 0 };
	uint8_t empty_check[LORA_APP_KEY_SIZE] = { 0 };

	struct lorawan_join_config join_cfg = {
		.mode = LORAWAN_ACT_OTAA,
		.dev_eui = dev_eui,
		.otaa.join_eui = join_eui,
		.otaa.app_key = app_key,
		.otaa.nwk_key = app_key,
		.otaa.dev_nonce = 0u,
	};

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("LoRa device not ready");
		return -EIO;
	}

	/* Fetch keys */
	rc = settings_runtime_get("lora_keys/dev_eui", dev_eui, sizeof(dev_eui));

	if (rc != sizeof(dev_eui)) {
		LOG_ERR("Invalid setting get dev eui response: %d", rc);
		return rc;
	} else if (memcmp(dev_eui, empty_check, sizeof(dev_eui)) == 0) {
		LOG_ERR("Key not set: dev eui, cannot start LoRa");
		return -ENOENT;
	}

	rc = settings_runtime_get("lora_keys/join_eui", join_eui, sizeof(join_eui));

	if (rc != sizeof(join_eui)) {
		LOG_ERR("Invalid setting get dev eui response: %d", rc);
		return rc;
	} else if (memcmp(join_eui, empty_check, sizeof(join_eui)) == 0) {
		LOG_ERR("Key not set: join eui, cannot start LoRa");
		return -ENOENT;
	}

	rc = settings_runtime_get("lora_keys/app_key", app_key, sizeof(app_key));

	if (rc != sizeof(app_key)) {
		LOG_ERR("Invalid setting get dev eui response: %d", rc);
		return rc;
	} else if (memcmp(app_key, empty_check, sizeof(app_key)) == 0) {
		LOG_ERR("Key not set: app key, cannot start LoRa");
		return -ENOENT;
	}

	rc = lorawan_start();

	if (rc < 0) {
		LOG_ERR("LoRa failed to start: %d", rc);
		return rc;
	}

#ifdef CONFIG_APP_LORA_ALLOW_DOWNLINKS
	lorawan_register_downlink_callback(&downlink_cb);
#endif

	while (join_attempts < LORA_JOIN_ATTEMPTS) {
		rc = lorawan_join(&join_cfg);

		if (rc < 0) {
			++join_attempts;
			LOG_ERR("LoRa join failed: %d", rc);
			led_blink(LED_RED, LORA_JOIN_FAIL_LED_BLINK_TIME);
			k_sleep(LORA_JOIN_FAIL_DELAY);
		} else if (rc == 0) {
			break;
		}
	}

	if (join_attempts == LORA_JOIN_ATTEMPTS) {
		return -ETIMEDOUT;
	}

#if CONFIG_APP_LORA_USE_SPECIFIC_DATARATE
	/* Change to desired datarate */
#if CONFIG_APP_LORA_DATARATE_0
	(void)lorawan_set_datarate(LORAWAN_DR_0);
#elif CONFIG_APP_LORA_DATARATE_1
	(void)lorawan_set_datarate(LORAWAN_DR_1);
#elif CONFIG_APP_LORA_DATARATE_2
	(void)lorawan_set_datarate(LORAWAN_DR_2);
#elif CONFIG_APP_LORA_DATARATE_3
	(void)lorawan_set_datarate(LORAWAN_DR_3);
#elif CONFIG_APP_LORA_DATARATE_4
	(void)lorawan_set_datarate(LORAWAN_DR_4);
#elif CONFIG_APP_LORA_DATARATE_5
	(void)lorawan_set_datarate(LORAWAN_DR_5);
#endif

#endif

#ifdef CONFIG_APP_LORA_USE_ADR
	lorawan_enable_adr(true);
#endif

	return 0;
}

int lora_send_message(uint8_t *data, uint16_t length, bool force_confirmed, uint8_t attempts)
{
	int rc = 0;
	bool confirmed = false;

	while (attempts > 0) {
#if CONFIG_APP_LORA_CONFIRMED_PACKET_ALWAYS
		confirmed = true;
#elif CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER > 0
		confirmed = unconfirmed_packets == CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER ? true : false;
#endif


		if (force_confirmed == true) {
			confirmed = true;
#if CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER > 0
		} else if (confirmed == false) {
			++unconfirmed_packets;

			if (unconfirmed_packets == CONFIG_APP_LORA_CONFIRMED_PACKET_AFTER) {
				unconfirmed_packets = 0;
			} else {
				++unconfirmed_packets;
			}
#endif
		}

		rc = lorawan_send(1, data, length, (confirmed == true ? LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

		if (rc < 0) {
			--attempts;
			LOG_ERR("LoRa send failed: %d", rc);

			if (attempts > 0) {
				k_sleep(LORA_SEND_FAIL_DELAY);
			}
		} else {
			break;
		}
	}

#ifdef CONFIG_APP_WATCHDOG
	if (attempts == 0 && rc < 0) {
		LOG_ERR("LoRa send failed too much, triggering watchdog");
		watchdog_fatal();
	}
#endif

	return rc;
}
