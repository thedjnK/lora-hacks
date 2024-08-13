/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
#include "hfclk.h"

LOG_MODULE_REGISTER(hfclk, CONFIG_APP_HFCLK_LOG_LEVEL);

#define HFCLK_START_TIMEOUT K_MSEC(200)

static K_SEM_DEFINE(hfclk_usage_sem, 1, 1);
static uint8_t hfclk_count = 0;
static const struct device *const clock = DEVICE_DT_GET(DT_NODELABEL(clock));

int hfclk_enable()
{
	int rc = 0;

	k_sem_take(&hfclk_usage_sem, K_FOREVER);

	if (hfclk_count == 0) {
		rc = clock_control_on(clock, CLOCK_CONTROL_NRF_TYPE_HFCLK);

		if (rc < 0) {
			LOG_ERR("HFCLK enable failed: %d", rc);
			goto finish;
		}
	}

	++hfclk_count;

finish:
	k_sem_give(&hfclk_usage_sem);

	return rc;
}

int hfclk_disable()
{
	int rc = 0;

	k_sem_take(&hfclk_usage_sem, K_FOREVER);

	if (hfclk_count == 0) {
		LOG_ERR("Tried to disable HFCLK when HFCLK is not running");
		rc = -EINVAL;
		goto finish;
	} else if (hfclk_count > 1) {
		--hfclk_count;
		goto finish;
	}

	rc = clock_control_off(clock, CLOCK_CONTROL_NRF_TYPE_HFCLK);

	if (rc < 0) {
		LOG_ERR("HFCLK disable failed: %d", rc);
	} else {
		hfclk_count = 0;
	}

finish:
	k_sem_give(&hfclk_usage_sem);

	return rc;
}
