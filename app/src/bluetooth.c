/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "bluetooth.h"
#include "settings.h"
#include "leds.h"
#include "watchdog.h"

LOG_MODULE_REGISTER(bluetooth, CONFIG_APP_BLUETOOTH_LOG_LEVEL);

#define BUTTON_ALIAS DT_ALIAS(sw0)

#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
#if DT_NODE_HAS_STATUS(BUTTON_ALIAS, okay)
#define BUTTON_DEVICE GPIO_DT_SPEC_GET(BUTTON_ALIAS, gpios)

static struct k_work button_work;
static struct gpio_callback button_cb_data;
static const struct gpio_dt_spec button = BUTTON_DEVICE;
#endif

static struct k_work advertise_work;
static void stop_advertising_function(struct k_timer *timer_id);
static bool continue_advert = false;
static bool in_connection = false;

K_TIMER_DEFINE(stop_advertising_timer, stop_advertising_function, NULL);
#endif

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

#if defined(CONFIG_BT_SMP)
/* Advertising interval minimum/maximum in 0.625us units, 1.5-2.25 seconds */
#define BT_ADV_INTERVAL_MIN 2400
#define BT_ADV_INTERVAL_MAX 3600

static uint8_t bonds;
static uint8_t gatt_value;

static void bond_loop(const struct bt_bond_info *info, void *user_data);
#endif

#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
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
#endif

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

		sd->data = device_name;
		sd->data_len = strlen(device_name);
	}

#if defined(CONFIG_BT_SMP)
	bonds = 0;
	bt_le_filter_accept_list_clear();
	bt_foreach_bond(BT_ID_DEFAULT, bond_loop, NULL);

	if (bonds > 0) {
		rc = bt_le_adv_start(BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONNECTABLE |
						      BT_LE_ADV_OPT_FILTER_CONN |
						      BT_LE_ADV_OPT_FILTER_SCAN_REQ),
				     BT_ADV_INTERVAL_MIN, BT_ADV_INTERVAL_MAX, NULL),
				     ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	} else {
		rc = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE,
				     BT_ADV_INTERVAL_MIN, BT_ADV_INTERVAL_MAX, NULL), ad,
				     ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	}
#else
	rc = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
#endif

	if (rc) {
		LOG_ERR("Advert start failed: %d", rc);

#ifdef CONFIG_APP_WATCHDOG
		watchdog_fatal();
#endif
	}
}

#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
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
#endif

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
		do_advert();
#endif
	} else {
		LOG_INF("Connected");

#if defined(CONFIG_BT_SMP)
		if (bt_conn_set_security(conn, BT_SECURITY_L4)) {
			LOG_ERR("Failed to set security level");
			bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		}
#endif
	}
}

#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	do_advert();
}

static void on_conn_recycled(void)
{
	do_advert();
}
#endif

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_DBG("Security changed to %d: for %s", level, addr);

		if (level == BT_SECURITY_L4) {
			bluetooth_security_changed();
		}
	} else {
		LOG_ERR("Security failed: %s level %u err %d", addr, level, err);
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
	.disconnected = disconnected,
	.recycled = on_conn_recycled,
#endif
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

/* TODO: Move to separate file */
#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
#if DT_NODE_HAS_STATUS(BUTTON_ALIAS, okay)
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}
#endif
#endif

#if defined(CONFIG_BT_SMP)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("Pairing Complete");
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_ERR("Pairing Failed (%d). Disconnecting", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	LOG_INF("bond Deleted");
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb auth_cb_info = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
	.bond_deleted = bond_deleted,
};
#endif

int bluetooth_init(void)
{
	int rc;
#if defined(CONFIG_BT_FIXED_PASSKEY)
	uint32_t fixed_passkey = 0;
#endif

#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
	k_work_init(&advertise_work, advertise);
#if DT_NODE_HAS_STATUS(BUTTON_ALIAS, okay)
	k_work_init(&button_work, advertise2);
#endif
#endif

	rc = bt_enable(NULL);

	if (rc != 0) {
		LOG_ERR("Bluetooth enable failed: %d", rc);
		return rc;
	}

#ifdef CONFIG_APP_BT_MODE_ADVERTISE_ON_DEMAND
#if DT_NODE_HAS_STATUS(BUTTON_ALIAS, okay)
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
#endif
#endif

#if defined(CONFIG_BT_SMP)
	bt_conn_auth_cb_register(&auth_cb_display);
	bt_conn_auth_info_cb_register(&auth_cb_info);
#if defined(CONFIG_BT_FIXED_PASSKEY)

	rc = settings_runtime_get("app/bluetooth_fixed_passkey", (uint8_t *)&fixed_passkey, sizeof(fixed_passkey));
	if (rc == sizeof(fixed_passkey)) {
		bt_passkey_set(fixed_passkey);
		LOG_DBG("Fixed passkey set to %06u - THIS IS VERY INSECURE", fixed_passkey);
	}

	rc = 0;
#endif
#endif

#ifdef CONFIG_APP_BT_ADVERTISE_ON_START
#if DT_NODE_HAS_STATUS(BUTTON_ALIAS, okay)
	k_work_submit(&button_work);
#else
	advertise2(NULL);
#endif
#endif

#ifdef CONFIG_APP_BT_MODE_ALWAYS_ADVERTISE
	advertise(NULL);
#endif

	return rc;
}

#if defined(CONFIG_BT_SMP)
void bluetooth_clear_bonds(void)
{
	int rc;

	rc = bt_unpair(BT_ID_DEFAULT, NULL);
	LOG_DBG("Unpair: %d", rc);
}

static void bond_loop(const struct bt_bond_info *info, void *user_data)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_le_filter_accept_list_add(&info->addr);
	bt_addr_le_to_str(&info->addr, addr_str, sizeof(addr_str));
	LOG_DBG("Added %s to advertising accept filter list", addr_str);
	bonds++;
}

static ssize_t read_u8(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
		       uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	if (bt_conn_get_security(conn) != BT_SECURITY_L4) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	bluetooth_garage_characteristic_written();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value, sizeof(value));
}

BT_GATT_SERVICE_DEFINE(ess_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	/* Garage door */
/* TODO: change this */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ,
			       (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_READ_AUTHEN |
				BT_GATT_PERM_READ_LESC), read_u8,
			       NULL, &gatt_value),
);
#endif
