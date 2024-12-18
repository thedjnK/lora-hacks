/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_WATCHDOG_H
#define APP_WATCHDOG_H

/* Initialise watchdog */
int watchdog_init(void);

/* Feed watchdog (if no errors) */
void watchdog_feed(void);

/* Mark fatal error (stop feeding watchdog) */
void watchdog_fatal(void);

#endif /* APP_WATCHDOG_H */
