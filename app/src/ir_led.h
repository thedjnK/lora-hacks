/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

enum AC_CMD {
/* AC_CMD_ON_AC_AUTO_18C_FAN_MOVE, */
	AC_CMD_ON_AC_HIGH_18C_FAN_MOVE,
	AC_CMD_ON_AC_MEDIUM_18C_FAN_MOVE,
	AC_CMD_OFF_COOLDOWN_2H,
	AC_CMD_OFF,

	AC_CMD_COUNT,
};

/* Setup Infrared LED up */
int ir_led_setup();

/* Send Infrared command */
int ir_led_send(enum AC_CMD command);
