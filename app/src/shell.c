/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>
#include "settings.h"

#define READ_ARGS 1
#define WRITE_ARGS 2

static int lora_dev_eui_handler(const struct shell *sh, size_t argc, char **argv)
{
	int rc = -EINVAL;
	uint8_t dev_eui[LORA_DEV_EUI_SIZE] = { 0 };

	if (argc == READ_ARGS) {
		/* Read */
		uint8_t print_buffer[LORA_DEV_EUI_SIZE * 2 + 1] = { 0 };

		rc = settings_runtime_get("lora_keys/dev_eui", dev_eui, sizeof(dev_eui));

		if (rc == sizeof(dev_eui)) {
			rc = bin2hex(dev_eui, sizeof(dev_eui), print_buffer, sizeof(print_buffer));

			if (rc == (LORA_DEV_EUI_SIZE * 2)) {
				shell_print(sh, "dev eui: %s", print_buffer);
				rc = 0;
			} else {
				shell_error(sh, "Failed to convert to hex: %d", rc);
			}
		} else if (rc >= 0) {
			shell_error(sh, "Invalid setting size");
		} else {
			shell_error(sh, "Invalid setting response: %d", rc);
		}
	} else if (argc == WRITE_ARGS) {
		/* Write */
		size_t data_size = strlen(argv[1]);

		if (data_size == (LORA_DEV_EUI_SIZE * 2)) {
			rc = hex2bin(argv[1], data_size, dev_eui, sizeof(dev_eui));

			if (rc == sizeof(dev_eui)) {
				rc = settings_runtime_set("lora_keys/dev_eui", dev_eui, sizeof(dev_eui));
				if (rc == 0) {
					shell_print(sh, "dev eui updated");
				} else {
					shell_print(sh, "Failed to update dev eui: %d", rc);
				}
			} else {
				shell_error(sh, "Invalid dev eui size");
			}
		} else {
			shell_error(sh, "Invalid dev eui size");
		}
	} else {
		shell_error(sh, "Invalid number of arguments");
	}

	return rc;
}

static int lora_join_eui_handler(const struct shell *sh, size_t argc, char **argv)
{
	int rc = -EINVAL;
	uint8_t join_eui[LORA_JOIN_EUI_SIZE] = { 0 };

	if (argc == READ_ARGS) {
		/* Read */
		uint8_t print_buffer[LORA_JOIN_EUI_SIZE * 2 + 1] = { 0 };

		rc = settings_runtime_get("lora_keys/join_eui", join_eui, sizeof(join_eui));

		if (rc == sizeof(join_eui)) {
			rc = bin2hex(join_eui, sizeof(join_eui), print_buffer, sizeof(print_buffer));

			if (rc == (LORA_JOIN_EUI_SIZE * 2)) {
				shell_print(sh, "join eui: %s", print_buffer);
				rc = 0;
			} else {
				shell_error(sh, "Failed to convert to hex: %d", rc);
			}
		} else if (rc >= 0) {
			shell_error(sh, "Invalid setting size");
		} else {
			shell_error(sh, "Invalid setting response: %d", rc);
		}
	} else if (argc == WRITE_ARGS) {
		/* Write */
		size_t data_size = strlen(argv[1]);

		if (data_size == (LORA_JOIN_EUI_SIZE * 2)) {
			rc = hex2bin(argv[1], data_size, join_eui, sizeof(join_eui));

			if (rc == sizeof(join_eui)) {
				rc = settings_runtime_set("lora_keys/join_eui", join_eui, sizeof(join_eui));
				if (rc == 0) {
					shell_print(sh, "join eui updated");
				} else {
					shell_print(sh, "Failed to update join eui: %d", rc);
				}
			} else {
				shell_error(sh, "Invalid join eui size");
			}
		} else {
			shell_error(sh, "Invalid join eui size");
		}
	} else {
		shell_error(sh, "Invalid number of arguments");
	}

	return rc;
}

static int lora_app_key_handler(const struct shell *sh, size_t argc, char **argv)
{
	int rc = -EINVAL;
	uint8_t app_key[LORA_APP_KEY_SIZE] = { 0 };

	if (argc == READ_ARGS) {
		/* Read */
		uint8_t print_buffer[LORA_APP_KEY_SIZE * 2 + 1] = { 0 };

		rc = settings_runtime_get("lora_keys/app_key", app_key, sizeof(app_key));

		if (rc == sizeof(app_key)) {
			rc = bin2hex(app_key, sizeof(app_key), print_buffer, sizeof(print_buffer));

			if (rc == (LORA_APP_KEY_SIZE * 2)) {
				shell_print(sh, "app key: %s", print_buffer);
				rc = 0;
			} else {
				shell_error(sh, "Failed to convert to hex: %d", rc);
			}
		} else if (rc >= 0) {
			shell_error(sh, "Invalid setting size");
		} else {
			shell_error(sh, "Invalid setting response: %d", rc);
		}
	} else if (argc == WRITE_ARGS) {
		/* Write */
		size_t data_size = strlen(argv[1]);

		if (data_size == (LORA_APP_KEY_SIZE * 2)) {
			rc = hex2bin(argv[1], data_size, app_key, sizeof(app_key));

			if (rc == sizeof(app_key)) {
				rc = settings_runtime_set("lora_keys/app_key", app_key, sizeof(app_key));
				if (rc == 0) {
					shell_print(sh, "app key updated");
				} else {
					shell_print(sh, "Failed to update app key: %d", rc);
				}
			} else {
				shell_error(sh, "Invalid app key size");
			}
		} else {
			shell_error(sh, "Invalid app key size");
		}
	} else {
		shell_error(sh, "Invalid number of arguments");
	}

	return rc;
}

#if 0
static int lora_dev_nonce_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}
#endif

static int lora_status_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int lora_clear_handler(const struct shell *sh, size_t argc, char **argv)
{
	lora_keys_clear();

	shell_print(sh, "LoRa keys cleared");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lora_cmd,
	/* Command handlers */
	SHELL_CMD(dev_eui, NULL, "Get/set LoRa dev EUI", lora_dev_eui_handler),
	SHELL_CMD(join_eui, NULL, "Get/set LoRa join EUI", lora_join_eui_handler),
	SHELL_CMD(app_key, NULL, "Get/set LoRa application key", lora_app_key_handler),
#if 0
	SHELL_CMD(dev_nonce, NULL, "Get/set LoRa device nonce", lora_dev_nonce_handler),
#endif
	SHELL_CMD(status, NULL, "Show LoRa status", lora_status_handler),
	SHELL_CMD(clear, NULL, "Clear LoRa configuration", lora_clear_handler),

	/* Array terminator. */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lora, &lora_cmd, "LoRa commands", NULL);

static int app_disable_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int app_enable_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int app_status_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(app_cmd,
	/* Command handlers */
	SHELL_CMD(disable, NULL, "Disable fetching readings", app_enable_handler),
	SHELL_CMD(enable, NULL, "Enable fetching readings", app_enable_handler),
	SHELL_CMD(status, NULL, "Show device status", app_status_handler),

	/* Array terminator. */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(app, &app_cmd, "Application commands", NULL);
