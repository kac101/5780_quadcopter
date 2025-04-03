cmake_minimum_required(VERSION 3.13)

# Set project name and language
project(drone C CXX ASM)

# Set C standard and C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set Pico board type (Pico W here)
set(PICO_BOARD pico_w)

# Import Pico SDK
include(pico_sdk_import.cmake)

# Initialize the SDK
pico_sdk_init()

# Create executable from source file(s)
add_executable(drone
    drone.cpp
)

# Generate PIO header if you're using PIO
pico_generate_pio_header(drone ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Enable USB and UART stdio
pico_enable_stdio_uart(drone 1)
pico_enable_stdio_usb(drone 1)

# Add include directories
target_include_directories(drone PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link required libraries (THIS is the KEY PART!)
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

# Create UF2 and binary files
pico_add_extra_outputs(drone)
