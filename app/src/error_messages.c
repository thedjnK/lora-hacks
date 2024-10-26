/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include "error_messages.h"

extern void lora_message_added(void);

static struct error_message_holder_t error_messages[ERROR_MESSAGE_COUNT];
static volatile uint8_t error_message_count = 0;
static K_SEM_DEFINE(lock_sem, 1, 1);

void error_message_lock(void)
{
	k_sem_take(&lock_sem, K_FOREVER);
}

void error_message_unlock(void)
{
	k_sem_give(&lock_sem);
}

uint8_t error_message_get_count(void)
{
	return error_message_count;
}

const struct error_message_holder_t *error_message_get_array(void)
{
	return error_messages;
}

int error_message_add_error(uint8_t *data, uint8_t data_size)
{
	if (data_size > ERROR_MESSAGE_MAX_SIZE) {
		return -EINVAL;
	} else if (error_message_count == ERROR_MESSAGE_COUNT) {
		return -E2BIG;
	}

	if (data_size > 0) {
		memcpy(error_messages[error_message_count].data, data, data_size);
	}

	error_messages[error_message_count].data_size = data_size;
	++error_message_count;
	lora_message_added();

	return 0;
}

void error_message_clear(void)
{
	memset(error_messages, 0, sizeof(error_messages));
	error_message_count = 0;
}
