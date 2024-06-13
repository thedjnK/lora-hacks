/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>

void peripheral_setup(void)
{
	/* We want DCDC enabled */
	NRF_POWER->DCDCEN = 1;

	/* Low power over constant latency */
	NRF_POWER->TASKS_LOWPWR = 1;

	/* Who cares about any of this */
	NRF_AAR->POWER = 0;
	NRF_ADC->POWER = 0;
	NRF_CCM->POWER = 0;
	NRF_ECB->POWER = 0;
	NRF_LPCOMP->POWER = 0;
	NRF_QDEC->POWER = 0;
	NRF_RADIO->POWER = 0;
	NRF_RTC0->POWER = 0;
	NRF_SPI0->POWER = 0;
	NRF_TIMER0->POWER = 0;
	NRF_TIMER1->POWER = 0;
	NRF_TIMER2->POWER = 0;
//	NRF_WDT->POWER = 0;
	NRF_TWI1->POWER = 0;

#if !(CONFIG_LOG) && !(CONFIG_SHELL)
	NRF_UART0->POWER = 0;
#endif
}
