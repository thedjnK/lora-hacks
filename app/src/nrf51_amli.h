/****************************************************************************************************//**
 * @file     nRF51.h
 *
 * @brief    CMSIS Cortex-M0 Peripheral Access Layer Header File for
 *           nRF51 from Nordic Semiconductor.
 *
 * @version  V522
 * @date     30. June 2014
 *
 * @note     Generated with SVDConv V2.81d 
 *           from CMSIS SVD File 'nRF51.xml' Version 522,
 *
 * @par      Copyright (c) 2013, Nordic Semiconductor ASA
 *           All rights reserved.
 *           
 *           Redistribution and use in source and binary forms, with or without
 *           modification, are permitted provided that the following conditions are met:
 *           
 *           * Redistributions of source code must retain the above copyright notice, this
 *           list of conditions and the following disclaimer.
 *           
 *           * Redistributions in binary form must reproduce the above copyright notice,
 *           this list of conditions and the following disclaimer in the documentation
 *           and/or other materials provided with the distribution.
 *           
 *           * Neither the name of Nordic Semiconductor ASA nor the names of its
 *           contributors may be used to endorse or promote products derived from
 *           this software without specific prior written permission.
 *           
 *           THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *           AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *           IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *           DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *           FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *           DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *           SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *           CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *           OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *           OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *           
 *
 *******************************************************************************************************/
#ifndef NRF51_AMLI_H
#define NRF51_AMLI_H

#include <stdint.h>

typedef struct {
	uint32_t CPU0;			/*!< Configurable priority configuration register for CPU0.	*/
	uint32_t SPIS1;			/*!< Configurable priority configuration register for SPIS1.	*/
	uint32_t RADIO;			/*!< Configurable priority configuration register for RADIO.	*/
	uint32_t ECB;			/*!< Configurable priority configuration register for ECB.	*/
	uint32_t CCM;			/*!< Configurable priority configuration register for CCM.	*/
	uint32_t AAR;			/*!< Configurable priority configuration register for AAR.	*/
} AMLI_RAMPRI_Type;

typedef struct {			/*!< AMLI Structure 						*/
	uint32_t  RESERVED0[896];
	AMLI_RAMPRI_Type RAMPRI;	/*!< RAM configurable priority configuration structure.		*/
} NRF_AMLI_Type;

#define NRF_AMLI_BASE	0x40000000UL
#define NRF_AMLI	((NRF_AMLI_Type *) NRF_AMLI_BASE)

/* Sets peripheral priorities up */
void nrf51_almi_setup(void);

#endif  /* NRF51_AMLI_H */
