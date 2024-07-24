/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>

/* Initialise Bluetooth */
int bluetooth_init(void);

/* Clear all bonds */
void bluetooh_clear_bonds(void);

/* Security changed to level 4 callback */
void bluetooth_security_changed(void);

/* Garage characteristic written callback */
void bluetooth_garage_characteristic_written(void);
