/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "settings.h"

LOG_MODULE_REGISTER(app_settings, CONFIG_APP_SETTINGS_LOG_LEVEL);

#define MAX_SETTING_KEY_LENGTH 24

static uint8_t lora_dev_eui[LORA_DEV_EUI_SIZE];
static uint8_t lora_join_eui[LORA_JOIN_EUI_SIZE];
static uint8_t lora_app_key[LORA_APP_KEY_SIZE];

#ifdef CONFIG_APP_EXTERNAL_DCDC
static uint16_t power_offset_mv;
#endif

#ifdef CONFIG_BT
static uint8_t bluetooth_device_name[BLUETOOTH_DEVICE_NAME_SIZE] = CONFIG_BT_DEVICE_NAME;
#endif

static int lora_keys_handle_set(const char *name, size_t len, settings_read_cb read_cb,
				void *cb_arg)
{
	const char *next;
	size_t name_len;
	int rc = -ENOENT;
	uint8_t *output = NULL;
	uint8_t output_size = 0;

	name_len = settings_name_next(name, &next);

	if (!next) {
		if (strncmp(name, "dev_eui", name_len) == 0) {
			if (len != sizeof(lora_dev_eui)) {
				return -EINVAL;
			}

			output = lora_dev_eui;
			output_size = sizeof(lora_dev_eui);
/*			rc = read_cb(cb_arg, lora_dev_eui, sizeof(lora_dev_eui));

			if (rc < 0) {
				goto finish;
			}

			rc = 0;*/
		} else if (strncmp(name, "join_eui", name_len) == 0) {
			if (len != sizeof(lora_join_eui)) {
				return -EINVAL;
			}

			output = lora_join_eui;
			output_size = sizeof(lora_join_eui);
/*			rc = read_cb(cb_arg, lora_join_eui, sizeof(lora_join_eui));

			if (rc < 0) {
				goto finish;
			}

			rc = 0;*/
		} else if (strncmp(name, "app_key", name_len) == 0) {
			if (len != sizeof(lora_app_key)) {
				return -EINVAL;
			}

			output = lora_app_key;
			output_size = sizeof(lora_app_key);
/*			rc = read_cb(cb_arg, lora_app_key, sizeof(lora_app_key));

			if (rc < 0) {
				goto finish;
			}

			rc = 0;*/
		}
	}

	if (output != NULL) {
		if (len != output_size) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, output, output_size);

		if (rc < 0) {
			goto finish;
		}

		rc = 0;
	}

	if (rc == 0) {
		uint8_t key_name[MAX_SETTING_KEY_LENGTH] = "lora_keys/";

		strcpy(&key_name[strlen(key_name)], name);

		rc = settings_save_one(key_name, output, len);
	}

finish:
	return rc;
}

static int lora_keys_handle_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	(void)cb("lora_keys/dev_eui", lora_dev_eui, sizeof(lora_dev_eui));
	(void)cb("lora_keys/join_eui", lora_join_eui, sizeof(lora_join_eui));
	(void)cb("lora_keys/app_key", lora_app_key, sizeof(lora_app_key));

	return 0;
}

static int lora_keys_handle_commit(void)
{
	return 0;
}

static int lora_keys_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	if (settings_name_steq(name, "dev_eui", &next) && !next) {
		if (val_len_max < sizeof(lora_dev_eui)) {
			return -E2BIG;
		}

		memcpy(val, lora_dev_eui, sizeof(lora_dev_eui));
		return sizeof(lora_dev_eui);
	} else if (settings_name_steq(name, "join_eui", &next) && !next) {
		if (val_len_max < sizeof(lora_join_eui)) {
			return -E2BIG;
		}

		memcpy(val, lora_join_eui, sizeof(lora_join_eui));
		return sizeof(lora_join_eui);
	} else if (settings_name_steq(name, "app_key", &next) && !next) {
		if (val_len_max < sizeof(lora_app_key)) {
			return -E2BIG;
		}

		memcpy(val, lora_app_key, sizeof(lora_app_key));
		return sizeof(lora_app_key);
	}

	return -ENOENT;
}

void lora_keys_load(void)
{
	settings_subsys_init();
	settings_load_subtree("lora_keys");
}

void lora_keys_clear(void)
{
	(void)settings_delete("lora_keys");
}

SETTINGS_STATIC_HANDLER_DEFINE(lora_keys, "lora_keys", lora_keys_handle_get, lora_keys_handle_set,
			       lora_keys_handle_commit, lora_keys_handle_export);


/* Application settings */

static int app_handle_set(const char *name, size_t len, settings_read_cb read_cb,
				void *cb_arg)
{
	const char *next;
	size_t name_len;
	int rc = -ENOENT;
	uint8_t *output = NULL;
	uint8_t output_size = 0;

	name_len = settings_name_next(name, &next);

	if (!next) {
#ifdef CONFIG_APP_EXTERNAL_DCDC
		if (strncmp(name, "power_offset", name_len) == 0) {
			if (len != sizeof(power_offset_mv)) {
				return -EINVAL;
			}

			output = (uint8_t *)&power_offset_mv;
			output_size = sizeof(power_offset_mv);
/*			rc = read_cb(cb_arg, &power_offset_mv, sizeof(power_offset_mv));

			if (rc < 0) {
				goto finish;
			}

			rc = 0;*/
		}
#endif

#ifdef CONFIG_BT
		if (strncmp(name, "bluetooth_name", name_len) == 0) {
			if (len == 0 || len >= sizeof(bluetooth_device_name) || ((uint8_t *)cb_arg)[len] == 0) {
				return -EINVAL;
			}

			output = bluetooth_device_name;
			output_size = sizeof(bluetooth_device_name);
			memset(bluetooth_device_name, 0, sizeof(bluetooth_device_name));
			goto save;
		}
#endif
	}

	if (output != NULL) {
		if (len != output_size) {
			return -EINVAL;
		}

save:
		rc = read_cb(cb_arg, output, output_size);

		if (rc < 0) {
			goto finish;
		}

		rc = 0;
	}

	if (rc == 0) {
		uint8_t key_name[MAX_SETTING_KEY_LENGTH] = "app/";

		strcpy(&key_name[strlen(key_name)], name);

		rc = settings_save_one(key_name, output, len);
	}

finish:
	return rc;
}

static int app_handle_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
#ifdef CONFIG_APP_EXTERNAL_DCDC
	(void)cb("app/power_offset", &power_offset_mv, sizeof(power_offset_mv));
#endif

#ifdef CONFIG_BT
	(void)cb("app/bluetooth_name", bluetooth_device_name, strlen(bluetooth_device_name));
#endif

	return 0;
}

static int app_handle_commit(void)
{
	return 0;
}

static int app_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

#ifdef CONFIG_APP_EXTERNAL_DCDC
	if (settings_name_steq(name, "power_offset", &next) && !next) {
		if (val_len_max < sizeof(power_offset_mv)) {
			return -E2BIG;
		}

		memcpy(val, &power_offset_mv, sizeof(power_offset_mv));
		return sizeof(power_offset_mv);
	}
#endif

#ifdef CONFIG_BT
	if (settings_name_steq(name, "bluetooth_name", &next) && !next) {
		if (val_len_max < strlen(bluetooth_device_name)) {
			return -E2BIG;
		}

		memcpy(val, bluetooth_device_name, strlen(bluetooth_device_name));
		return strlen(bluetooth_device_name);
	}
#endif

	return -ENOENT;
}

void app_keys_load(void)
{
	settings_load_subtree("app");
}

void app_keys_clear(void)
{
	(void)settings_delete("app");
}

SETTINGS_STATIC_HANDLER_DEFINE(app, "app", app_handle_get, app_handle_set,
			       app_handle_commit, app_handle_export);


void setting_lora(enum lora_setting_index index, const uint8_t *data, uint8_t data_size)
{
	switch (index) {
#ifdef CONFIG_APP_EXTERNAL_DCDC
		case LORA_SETTING_INDEX_ADC_OFFSET:
		{
			if (data_size == POWER_OFFSET_MV_SIZE) {
				(void)settings_runtime_set("app/power_offset", data, data_size);
			} else {
				LOG_ERR("Invalid ADC offset length: %d", data_size);
			}

			break;
		}
#endif

#ifdef CONFIG_BT
		case LORA_SETTING_INDEX_BLUETOOTH_DEVICE_NAME:
		{
			if (data_size > 0 && data_size <= BLUETOOTH_DEVICE_NAME_SIZE) {
				(void)settings_runtime_set("app/bluetooth_name", data, data_size);
			} else {
				LOG_ERR("Invalid Bluetooth device name length: %d", data_size);
			}

			break;
		}
#endif

		default:
		{
			LOG_ERR("Invalid LoRa setting index: %d", index);
		}
	};
}
