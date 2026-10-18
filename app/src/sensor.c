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

#if CONFIG_DT_HAS_SILABS_SI7006_ENABLED
#include <zephyr/drivers/i2c.h>
#include <si7006.h>

//Taken from ...
struct si7006_config {
	struct i2c_dt_spec i2c;
	const struct device *vin_supply;
	/** Use "read temp" vs "read old temp" command, the latter only with SiLabs sensors. */
	uint8_t read_temp_cmd;
};

static int si7060_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct si7006_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

void sensor_run_heater(void)
{
//	si7060_reg_write(sensor, SI7006_WRITE_HEATER_CONTR, 0x4);
	si7060_reg_write(sensor, SI7006_WRITE_HEATER_CONTR, 0x0);
	si7060_reg_write(sensor, SI7006_WRITE_HUMIDITY_TEMP_CONTR, 0x3e);
//	k_sleep(K_MSEC(50));
//	k_sleep(K_MSEC(500));
//	si7060_reg_write(sensor, SI7006_WRITE_HUMIDITY_TEMP_CONTR, 0x3a);
}
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
