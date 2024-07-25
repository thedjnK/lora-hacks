/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include "watchdog.h"

LOG_MODULE_REGISTER(watchdog, CONFIG_APP_WATCHDOG_LOG_LEVEL);

/* Watchdog config, 10 minute timeout */
#define WDT_MIN_WINDOW  0U
#define WDT_MAX_WINDOW  600000U

static bool watchdog_feed_allowed;
static int wdt_channel_id;
const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

int watchdog_init(void)
{
	int rc;

	if (!device_is_ready(wdt)) {
		LOG_ERR("Watchdog device not ready");

		return -ENODEV;
	}

	struct wdt_timeout_cfg wdt_config = {
		/* Reset SoC when watchdog timer expires. */
		.flags = WDT_FLAG_RESET_SOC,

		/* Expire watchdog after max window */
		.window.min = WDT_MIN_WINDOW,
		.window.max = WDT_MAX_WINDOW,
	};

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);

	if (wdt_channel_id < 0) {
		LOG_ERR("Watchdog install error: %d", wdt_channel_id);

		return -ENODEV;
	}

	rc = wdt_setup(wdt, 0);

	if (rc < 0) {
		LOG_ERR("Watchdog setup error: %d", rc);

		return -ENODEV;
	}

	watchdog_feed_allowed = true;

	return 0;
}

void watchdog_feed(void)
{
	if (!watchdog_feed_allowed) {
		return;
	}

	wdt_feed(wdt, wdt_channel_id);
}

void watchdog_fatal(void)
{
	watchdog_feed_allowed = false;
}
