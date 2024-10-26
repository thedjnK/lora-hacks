/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_ADC_H
#define APP_ADC_H

#include <zephyr/kernel.h>

/* Initialise ADC */
int adc_setup();

/* Read internal voltage, in mV */
int adc_read_internal(uint16_t *voltage);

#endif /* APP_ADC_H */
