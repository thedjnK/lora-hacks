/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "bluetooth.h"
#include "settings.h"
#include "leds.h"

LOG_MODULE_REGISTER(bluetooth, CONFIG_APP_BLUETOOTH_LOG_LEVEL);

static struct k_work advertise_work;
static struct k_work button_work;

static void stop_advertising_function(struct k_timer *timer_id);
static bool continue_advert = false;
static bool in_connection = false;

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button_cb_data;

K_TIMER_DEFINE(stop_advertising_timer, stop_advertising_function, NULL);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
		      0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

static uint8_t device_name[BLUETOOTH_DEVICE_NAME_SIZE] = CONFIG_BT_DEVICE_NAME;

static struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, device_name, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void stop_advertising_function(struct k_timer *timer_id)
{
	continue_advert = false;

	if (in_connection == false) {
		led_off(LED_BLUE);
		bt_le_adv_stop();
	}
}

static void do_advert(void)
{
	if (continue_advert == true) {
		k_work_submit(&advertise_work);
	} else {
		led_off(LED_BLUE);
	}
}

static void advertise(struct k_work *work)
{
	int rc;
	uint8_t device_name[BLUETOOTH_DEVICE_NAME_SIZE] = { 0 };

	/* Get device name to advertise with */
	rc = settings_runtime_get("app/bluetooth_name", device_name, sizeof(device_name));

	if (rc <= 0 || device_name[0] == 0x00) {
		strcpy(device_name, CONFIG_BT_DEVICE_NAME);
		LOG_INF("Device name not set, using default");
	} else {
		rc = bt_set_name((char *)device_name);

		if (rc) {
			LOG_ERR("Device name set failed: %d", rc);
		}

		sd->data_len = strlen(device_name);
	}

	rc = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (rc) {
		LOG_ERR("Advert start failed: %d", rc);
	}
}

static void advertise2(struct k_work *work)
{
	k_timer_start(&stop_advertising_timer, K_SECONDS(20), K_NO_WAIT);

	if (continue_advert == false) {
		led_on(LED_BLUE);

		continue_advert = true;

		if (in_connection == false) {
			advertise(NULL);
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
		do_advert();
	} else {
		LOG_INF("Connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	do_advert();
}

static void on_conn_recycled(void)
{
	do_advert();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = on_conn_recycled,
};

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}

int bluetooth_init(void)
{
	int rc;

	k_work_init(&advertise_work, advertise);
	k_work_init(&button_work, advertise2);

	rc = bt_enable(NULL);

	if (rc != 0) {
		LOG_ERR("Bluetooth enable failed: %d", rc);
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

#ifdef CONFIG_APP_BT_ADVERTISE_ON_START
	k_work_submit(&button_work);
#endif

	return rc;
}
