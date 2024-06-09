/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

static int lora_dev_eui_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int lora_join_eui_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int lora_app_key_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int lora_status_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

static int lora_clear_handler(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	shell_print(sh, "TODO");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lora_cmd,
	/* Command handlers */
	SHELL_CMD(dev_eui, NULL, "Get/set LoRa dev EUI", lora_dev_eui_handler),
	SHELL_CMD(join_eui, NULL, "Get/set LoRa join EUI", lora_join_eui_handler),
	SHELL_CMD(app_key, NULL, "Get/set LoRa application key", lora_app_key_handler),
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
