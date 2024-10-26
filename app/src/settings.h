/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#define LORA_DEV_EUI_SIZE 8
#define LORA_JOIN_EUI_SIZE 8
#define LORA_APP_KEY_SIZE 16
#define POWER_OFFSET_MV_SIZE 2
#define BLUETOOTH_DEVICE_NAME_SIZE CONFIG_BT_DEVICE_NAME_MAX
#define BLUETOOTH_FIXED_PASSKEY_SIZE 4

enum lora_setting_index {
	LORA_SETTING_INDEX_ADC_OFFSET,
	LORA_SETTING_INDEX_BLUETOOTH_DEVICE_NAME,
	LORA_SETTING_INDEX_BLUETOOTH_FIXED_PASSKEY,

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

/* Set setting (from LoRa) */
void setting_lora(enum lora_setting_index index, const uint8_t *data, uint8_t data_size);

#endif /* APP_SETTINGS_H */
