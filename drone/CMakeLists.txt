cmake_minimum_required(VERSION 3.13)

project(drone C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use the correct board
set(PICO_BOARD pico_w)

#  Update this to your actual SDK path
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")

# Import the Pico SDK
include(pico_sdk_import.cmake)
pico_sdk_init()

# Add your main executable
add_executable(drone
    drone.cpp
)

# Optional: add PIO headers if you’re using PIO assembly (.pio files)
# pico_generate_pio_header(drone ${CMAKE_CURRENT_LIST_DIR}/your_program.pio)

# Enable USB and UART stdio
pico_enable_stdio_uart(drone 1)
pico_enable_stdio_usb(drone 1)

# Include current directory
target_include_directories(drone PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link necessary libraries
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

# Force ELF output extension to avoid build error
set_target_properties(drone PROPERTIES SUFFIX ".elf")

# Generate bin, uf2, etc.
pico_add_extra_outputs(drone)
