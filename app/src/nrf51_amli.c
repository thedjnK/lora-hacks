/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf51_amli.h"

void nrf51_almi_setup(void)
{
	NRF_AMLI->RAMPRI.CPU0    = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.SPIS1   = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.RADIO   = 0x00000000UL;
	NRF_AMLI->RAMPRI.ECB     = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.CCM     = 0x00000000UL;
	NRF_AMLI->RAMPRI.AAR     = 0xFFFFFFFFUL;
}
