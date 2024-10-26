/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_HFCLK_H
#define APP_HFCLK_H

/* Enable HFCLK and wait for it to be ready */
int hfclk_enable();

/* Disable HFCLK without waiting for it to stop */
int hfclk_disable();

#endif /* APP_HFCLK_H */
