/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include "adc.h"

LOG_MODULE_REGISTER(adc, CONFIG_APP_ADC_LOG_LEVEL);

const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));

int adc_setup()
{
	struct adc_channel_cfg adc_channel_config = {
		.gain = ADC_GAIN_1_3,
//		.gain = ADC_GAIN_2_3,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		.input_positive = 0,
	};
	int rc;

	if (!device_is_ready(adc)) {
		LOG_ERR("ADC device not ready");
		return -ENOENT;
	}

	rc = adc_channel_setup(adc, &adc_channel_config);

	if (rc != 0) {
		LOG_ERR("ADC channel setup failed: %d", rc);
	}

	return rc;
}

int adc_read_internal(uint16_t *voltage)
{
	int rc;
	int16_t adc_value = 0;
	uint32_t conversion;
	struct adc_sequence adc_sequence = {
		.channels = BIT(0),
		.buffer = &adc_value,
		.buffer_size = sizeof(adc_value),
		.oversampling = 0,
		.calibrate = true,
		.resolution = 10,
	};

	rc = adc_read(adc, &adc_sequence);

	if (rc != 0) {
		LOG_ERR("ADC reading failed: %d", rc);
		return rc;
	}

	conversion = ((adc_value * 1000) / 1024 * 1200 * 3) / 1000;
	*voltage = conversion;

	LOG_INF("Power: %dmV", conversion);

	return rc;
}
