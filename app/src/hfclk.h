/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

/* Enable HFCLK and wait for it to be ready */
int hfclk_enable();

/* Disable HFCLK without waiting for it to stop */
int hfclk_disable();
