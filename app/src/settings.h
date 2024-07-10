/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#define LORA_DEV_EUI_SIZE 8
#define LORA_JOIN_EUI_SIZE 8
#define LORA_APP_KEY_SIZE 16
#define POWER_OFFSET_MV_SIZE 2

enum lora_setting_index {
	LORA_SETTING_INDEX_ADC_OFFSET,

	LORA_SETTING_INDEX_COUNT
};

/* Load LoRa keys */
void lora_keys_load(void);

/* Clear LoRa keys */
void lora_keys_clear(void);

/* Load application keys */
void app_keys_load(void);

/* Clear application keys */
void app_keys_clear(void);

/* Set settina (from LoRa) */
void setting_lora(enum lora_setting_index index, const uint8_t *data, uint8_t data_size);
