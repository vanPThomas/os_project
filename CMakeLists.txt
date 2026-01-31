cmake_minimum_required(VERSION 3.13)

# Use hardcoded path to eliminate env var issues
include(/home/thomas/pico-projects/pico-sdk/external/pico_sdk_import.cmake)

project(os_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Critical: Enables pico_w.h and CYW43 defines
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(os_project
    src/main.cpp
    src/IrRemote.cpp
    src/Oled.cpp
)

target_include_directories(os_project PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Critical: This adds the include path for pico/cyw43_arch.h and links the arch code
target_link_libraries(os_project
    pico_stdlib
    pico_cyw43_arch_none
    hardware_i2c
)

# USB serial for debug
pico_enable_stdio_usb(os_project 1)
pico_enable_stdio_uart(os_project 0)
pico_add_extra_outputs(os_project)

pico_add_extra_outputs(os_project)
