cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(ebenstahl C CXX ASM)

#
# Boot stage 2.
#
# These boards boot from a serial EEPROM rather than NOR flash, so the XIP
# setup must use plain 03h reads. Without this the build silently falls back
# to the PICO_BOARD default (boot2_w25q080), which issues Winbond-specific
# QSPI commands; the resulting image links cleanly but hangs the moment the
# bootrom hands over, with no USB and no LED.
#
# This must be set before pico_sdk_init(). Using the short name rather than
# a full path keeps it working across SDK versions -- the boot_stage2 sources
# moved from src/rp2_common/ to src/rp2040/ in SDK 2.x.
#
set(PICO_DEFAULT_BOOT_STAGE2 boot2_generic_03h)

add_executable(ebenstahl)

target_sources(ebenstahl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ebenstahl.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/led.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mapper.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drv_fram.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drv_eeprom.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drv_flash.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_msc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
  )

target_include_directories(ebenstahl PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/src
   )

pico_sdk_init()

target_compile_definitions(ebenstahl PUBLIC
   PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
   )

target_link_libraries(ebenstahl PRIVATE pico_stdlib hardware_resets hardware_irq hardware_spi hardware_pwm tinyusb_device)

pico_add_extra_outputs(ebenstahl)
