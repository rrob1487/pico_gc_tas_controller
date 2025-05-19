cmake_minimum_required(VERSION 3.12)

# Set Board
set(PICO_BOARD pico2_w)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(PICO_SDK_FETCH_FROM_GIT 1)

include(pico_sdk_import.cmake)

pico_sdk_init(enable_ble)

project(pico_tas_playback CXX C)
include_directories(include pico-joybus-comms/include ble_files)

# Create a target name
set(TARGET_NAME "pico_tas_playback")

# Add an executable target
add_executable(${TARGET_NAME}
src/main.cpp
src/BLEServer.c
src/ControllerInterface.cpp
src/ControllerInterfaceCWrapper.cpp
src/RKGReader.cpp
src/DTMReader.cpp
src/LUAReader.cpp
src/file.S
)
# enable usb output, disable uart output
pico_enable_stdio_usb(${TARGET_NAME} 1)
pico_enable_stdio_uart(${TARGET_NAME} 0)

# Generate the PIO header
pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/pio/my_pio.pio)

# Gyatt the Gatt
pico_btstack_make_gatt_header(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_files/cheaterconch.gatt")

# Add the custom command for generating the PIO header
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/my_pio.pio.h
      DEPENDS ${CMAKE_CURRENT_LIST_DIR}/my_pio.pio
      COMMAND Pioasm ${CMAKE_CURRENT_LIST_DIR}/my_pio.pio ${CMAKE_CURRENT_LIST_DIR}/generated/my_pio.pio.h
)

# Link libraries
target_link_libraries(${TARGET_NAME} pico_stdlib
    pico_cyw43_arch_none
    hardware_pio
    pico_time
    pico_bootrom
    pico_btstack_ble
    pico_btstack_cyw43
    hardware_resets
    hardware_timer
    hardware_irq
    hardware_sync
    hardware_flash
    )


target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated/pico_tas_playback_gatt_header
)

# Add extra outputs
pico_add_extra_outputs(${TARGET_NAME})
