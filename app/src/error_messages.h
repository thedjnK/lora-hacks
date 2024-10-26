/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_ERROR_MESSAGES_H
#define APP_ERROR_MESSAGES_H

#include <stdint.h>

#define ERROR_MESSAGE_MAX_SIZE 3
#define ERROR_MESSAGE_COUNT 3

struct error_message_holder_t {
        uint8_t data[ERROR_MESSAGE_MAX_SIZE];
        uint8_t data_size;
	uint8_t frame_port;
};

/* Lock error message system for usage, this must be done prior to use of other functions */
void error_message_lock(void);

/* Unlock error message system, this must be done when finished using system */
void error_message_unlock(void);

/* Get number of pending error messages */
uint8_t error_message_get_count(void);

/* Get the error message array */
const struct error_message_holder_t *error_message_get_array(void);

/* Add an error to the list */
int error_message_add_error(uint8_t *data, uint8_t data_size);

/* Clear all errors in the list */
void error_message_clear(void);

#endif /* APP_ERROR_MESSAGES_H */
