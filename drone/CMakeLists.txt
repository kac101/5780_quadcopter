cmake_minimum_required(VERSION 3.13)

project(drone C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w)

# Update this to the path of your pico-sdk
set(PICO_SDK_PATH "/path/to/pico-sdk")

include(pico_sdk_import.cmake)
pico_sdk_init()

add_executable(drone
    drone.cpp
)

# If you're not using PIO, comment this out
# pico_generate_pio_header(drone ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

pico_enable_stdio_uart(drone 1)
pico_enable_stdio_usb(drone 1)

target_include_directories(drone PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(drone
    pico_stdlib
    hardware_pwm
    hardware_spi
    hardware_i2c
    hardware_dma
    hardware_pio
    hardware_timer
    hardware_clocks
    pico_cyw43_arch_none
)

pico_add_extra_outputs(drone)
