#
# Copyright (c) 2024, Jamie M.
#
# All right reserved. This code is NOT apache or FOSS/copyleft licensed.
#

if(CONFIG_MCUMGR_TRANSPORT_LORA)
  zephyr_library_amend()
  zephyr_library_include_directories($<TARGET_PROPERTY:SOURCE_DIR>)
  zephyr_library_sources(src/smp_lora.c)
endif()

