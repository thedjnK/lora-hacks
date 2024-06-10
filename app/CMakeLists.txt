cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lora-hacks)

target_sources(app PRIVATE src/sensor.c src/settings.c src/lora.c src/leds.c src/main.c)
target_sources_ifdef(CONFIG_SHELL app PRIVATE src/shell.c)
