/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>

enum bluetooth_remote_op_t {
	BLUETOOTH_REMOTE_OP_ADVERT_START,
	BLUETOOTH_REMOTE_OP_ADVERT_STOP,
	BLUETOOTH_REMOTE_OP_ADVERT_DISCONNECT,
	BLUETOOTH_REMOTE_OP_ADVERT_CLEAR_BONDS,

	BLUETOOTH_REMOTE_OP_COUNT,
};

/* Initialise Bluetooth */
int bluetooth_init(void);

/* Clear all bonds */
void bluetooh_clear_bonds(void);

/* Security changed to level 4 callback */
void bluetooth_security_changed(void);

/* Garage characteristic written callback */
void bluetooth_garage_characteristic_written(void);

/* Clears all Bluetooth bonds */
void bluetooth_clear_bonds(void);

/* Remote LoRa function for bluetooth */
int bluetooth_remote(enum bluetooth_remote_op_t op);
