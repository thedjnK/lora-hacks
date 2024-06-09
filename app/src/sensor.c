/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_DBG);

#define SENSOR_DEV DT_NODELABEL(si7021)

static const struct device *const sensor = DEVICE_DT_GET(SENSOR_DEV);

int sensor_fetch_readings(int8_t *temperature, int8_t *humidity)
{
	struct sensor_value temp;
	int rc;

	rc = sensor_sample_fetch(sensor);

	if (rc) {
		LOG_ERR("Sensor fetch failed: %d", rc);
		return rc;
	}

	rc = sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);

	if (rc) {
		LOG_ERR("Sensor temperature get failed: %d", rc);
		return rc;
	}

	temperature[0] = temp.val1;
	temperature[1] = temp.val2;

	LOG_INF("Temperature: %.1fC", sensor_value_to_double(&temp));

	rc = sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &temp);

	if (rc) {
		LOG_ERR("Sensor humidity get failed: %d", rc);
		return rc;
	}

	if (temp.val1 < 0) {
		temp.val1 = 0;
	} else if (temp.val1 > 100) {
		temp.val1 = 100;
	}

	*humidity = (int8_t)temp.val1;

	LOG_INF("Humidity: %.1f%c", sensor_value_to_double(&temp), '%');

	return 0;
}

int sensor_setup(void)
{
        if (!device_is_ready(sensor)) {
                LOG_ERR("Device not ready: %s", sensor->name);
                return -EIO;
        }

	return 0;
}


