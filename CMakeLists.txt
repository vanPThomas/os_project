cmake_minimum_required(VERSION 3.13)

include(/home/thomas/pico-projects/pico-sdk/external/pico_sdk_import.cmake)

project(os_project VERSION 0.3.0 LANGUAGES C CXX ASM)

set(PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH})

pico_sdk_init()

add_executable(os_project
    src/main.cpp
    src/IrRemote.cpp
    src/Oled.cpp
    src/MenuApp.cpp
)

target_include_directories(os_project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(os_project PRIVATE
    pico_stdlib
    hardware_i2c
)

target_compile_definitions(os_project PRIVATE
    PROJECT_VERSION=\"${PROJECT_VERSION}\"
)

# USB serial output, no UART
pico_enable_stdio_usb(os_project 1)
pico_enable_stdio_uart(os_project 0)

# Generate .uf2, .hex, etc.
pico_add_extra_outputs(os_project)