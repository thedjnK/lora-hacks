/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#define LORA_DEV_EUI_SIZE 8
#define LORA_JOIN_EUI_SIZE 8
#define LORA_APP_KEY_SIZE 16
#define POWER_OFFSET_MV_SIZE 2

/* Load LoRa keys */
void lora_keys_load(void);

/* Clear LoRa keys */
void lora_keys_clear(void);

/* Load application keys */
void app_keys_load(void);

/* Clear application keys */
void app_keys_clear(void);
