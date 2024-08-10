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

#define HFCLK_START_TIMEOUT K_MSEC(50)

static K_SEM_DEFINE(hfclk_ready_sem, 0, 1);
static struct onoff_client hfclk_client;
static uint8_t hfclk_count = 0;
static struct onoff_manager *hfclk_manager;

static void clock_ready(struct onoff_manager *manager, struct onoff_client *client, uint32_t state,
			int rc)
{
	k_sem_give(&hfclk_ready_sem);
}

int hfclk_enable()
{
	int rc;

	rc = onoff_request(hfclk_manager, &hfclk_client);

	if (rc < 0) {
		LOG_ERR("HFCLK enable failed: %d", rc);
		goto finish;
	}

	rc = k_sem_take(&hfclk_ready_sem, HFCLK_START_TIMEOUT);

	if (rc != 0) {
		LOG_ERR("HFCLK start wait timeout: %d", rc);
	} else {
		++hfclk_count;
	}

finish:
	return rc;
}

int hfclk_disable()
{
	int rc;

	if (hfclk_count == 0) {
		LOG_ERR("Tried to disable HFCLK when HFCLK is not running");
		return -EINVAL;
	}

	rc = onoff_release(hfclk_manager);

	if (rc < 0) {
		LOG_ERR("HFCLK disable failed: %d", rc);
	} else {
		--hfclk_count;
	}

	return rc;
}

int hfclk_setup()
{
	hfclk_manager = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	sys_notify_init_callback(&hfclk_client.notify, clock_ready);

	return 0;
}
