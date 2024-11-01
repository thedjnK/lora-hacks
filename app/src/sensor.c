/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/regulator.h>

LOG_MODULE_REGISTER(sensor, CONFIG_APP_SENSOR_LOG_LEVEL);

#if CONFIG_DT_HAS_SILABS_SI7006_ENABLED
#define SENSOR_DEV DT_NODELABEL(si7021)
#elif CONFIG_DT_HAS_BOSCH_BME680_ENABLED
#define SENSOR_DEV DT_NODELABEL(bme680)
#else
#error "No sensor selected"
#endif

#ifdef CONFIG_APP_POWER_DOWN_EXTERNAL_SENSOR
#define REGULATOR_DEV DT_NODELABEL(ext_power)
#endif

static const struct device *const sensor = DEVICE_DT_GET(SENSOR_DEV);

#ifdef REGULATOR_DEV
static const struct device *const regulator = DEVICE_DT_GET(REGULATOR_DEV);
#endif

int sensor_fetch_readings(int8_t *temperature, int8_t *humidity)
{
	struct sensor_value temp;
	int rc;

#ifdef CONFIG_APP_POWER_DOWN_EXTERNAL_SENSOR
	rc = regulator_enable(regulator);

	if (rc) {
		LOG_ERR("Regulator enable failed: %d", rc);
		return rc;
	}
#endif

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
	temperature[1] = temp.val2 / 10000;

	LOG_INF("Temperature: %.1fC", sensor_value_to_double(&temp));

	rc = sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &temp);

	if (rc) {
		LOG_ERR("Sensor humidity get failed: %d", rc);
		return rc;
	}

	if (temp.val1 <= 0) {
		temp.val1 = 0;

		if (temp.val2 < 0) {
			temp.val2 = 0;
		}
	} else if (temp.val1 >= 100) {
		temp.val1 = 100;
		temp.val2 = 0;
	}

	humidity[0] = temp.val1;
	humidity[1] = temp.val2 / 10000;

	LOG_INF("Humidity: %.1f%c", sensor_value_to_double(&temp), '%');

#ifdef CONFIG_APP_POWER_DOWN_EXTERNAL_SENSOR
	rc = regulator_disable(regulator);

	if (rc) {
		LOG_ERR("Regulator disable failed: %d", rc);
		return rc;
	}
#endif

	return 0;
}

int sensor_setup(void)
{
	int rc = 0;

	if (!device_is_ready(sensor)) {
		LOG_ERR("Device not ready: %s", sensor->name);
		rc = -EIO;
		goto finish;
	}

#ifdef CONFIG_APP_POWER_DOWN_EXTERNAL_SENSOR
	if (!device_is_ready(regulator)) {
		LOG_ERR("Device not ready: %s", regulator->name);
		rc = -EIO;
		goto finish;
	}

	rc = regulator_disable(regulator);
#endif

finish:
	return rc;
}
