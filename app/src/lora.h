/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>

/* Setup LoRa */
int lora_setup(void);

/* Send LoRa message */
int lora_send_message(uint8_t *data, uint16_t length, bool force_confirmed, uint8_t attempts);

/* Callback on LoRa downlink message */
void lora_message_callback(uint8_t port, const uint8_t *data, uint8_t len);
