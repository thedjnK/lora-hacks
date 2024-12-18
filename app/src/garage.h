/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_GARAGE_H
#define APP_GARAGE_H

#include <zephyr/kernel.h>

/* Initialise garage */
int garage_init(void);

/* Open or close garage door */
void garage_door_open_close(void);

#endif /* APP_GARAGE_H */
