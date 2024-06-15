zephyr_library_amend()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/adc.h)
zephyr_library_include_directories($<TARGET_PROPERTY:SOURCE_DIR>)
zephyr_library_sources_ifdef(CONFIG_ADC_NRFX_ADC_INTERNAL	adc_nrfx_adc_internal.c)
