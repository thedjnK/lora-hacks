/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_SENSOR_H
#define APP_SENSOR_H

/* Setup sensor module */
int sensor_setup(void);

/* Fetch sensor readings in XX.YY format for temperature and humidity */
int sensor_fetch_readings(int8_t *temperature, int8_t *humidity);

#endif /* APP_SENSOR_H */
