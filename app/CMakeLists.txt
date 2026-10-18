#
# Copyright (c) 2024, Jamie M.
#
# All right reserved. This code is NOT apache or FOSS/copyleft licensed.

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lora-hacks)

target_sources(app PRIVATE src/sensor.c src/settings.c src/lora.c src/leds.c src/main.c src/peripherals.c src/hfclk.c src/nrf51_amli.c src/error_messages.c)
target_sources_ifdef(CONFIG_SHELL app PRIVATE src/shell.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/bluetooth.c)
target_sources_ifdef(CONFIG_ADC app PRIVATE src/adc.c)
target_sources_ifdef(CONFIG_APP_IR_LED app PRIVATE src/ir_led.c)
target_sources_ifdef(CONFIG_APP_GARAGE_DOOR app PRIVATE src/garage.c)
target_sources_ifdef(CONFIG_APP_WATCHDOG app PRIVATE src/watchdog.c)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_LORAWAN app PRIVATE src/smp_lorawan.c)

set(ZEPHYR_CURRENT_LIBRARY loramac-node)

#zephyr_library_sources(src/soft-se.c)

zephyr_library_include_directories(
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se
  ${ZEPHYR_TINYCRYPT_MODULE_DIR}/lib/include
)

zephyr_library_compile_definitions(
  USE_LRWAN_1_0_X_CRYPTO
)

set_source_files_properties(
#  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/aes.c
#  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/cmac.c
#  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/soft-se.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacClassB.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/lorawan/
  PROPERTIES HEADER_FILE_ONLY ON
)

if(CONFIG_DT_HAS_SILABS_SI7006_ENABLED)
  zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/silabs/si7006)
endif()
