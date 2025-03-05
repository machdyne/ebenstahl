cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(ebenstahl C CXX ASM)

add_executable(ebenstahl)

target_sources(ebenstahl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ebenstahl.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drv_fram.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_msc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
  )

target_include_directories(ebenstahl PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/src
   )

pico_sdk_init()

target_link_libraries(ebenstahl PRIVATE pico_stdlib hardware_resets hardware_irq hardware_spi tinyusb_device)

pico_add_extra_outputs(ebenstahl)
