/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include "settings.h"
#include "sensor.h"
#include "lora.h"
#include "leds.h"

int main(void)
{
	int rc;

	(void)leds_init();
	lora_keys_load();
	sensor_setup();
	rc = lora_setup();
}
