# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(solar_station_v2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(solar_station_v2 
        main.c
        drivers/network/tcp_client
        drivers/lux/bh1750
        drivers/angle/mpu6050
        drivers/energy/ina219
        drivers/i2c/i2c_bus
        drivers/display_2.0/ssd1306_i2c
        drivers/temperature/ds18b20
        drivers/security/crypto
)

add_subdirectory(drivers/onewire_library)

pico_set_program_name(solar_station_v2 "solar_station_v2")
pico_set_program_version(solar_station_v2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(solar_station_v2 1)
pico_enable_stdio_usb(solar_station_v2 1)

# Add the standard library to the build
target_link_libraries(solar_station_v2
        pico_stdlib
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
)

# Add the standard include files to the build
target_include_directories(solar_station_v2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(solar_station_v2 
        hardware_i2c
        hardware_adc
        onewire_library
        pico_cyw43_arch_lwip_threadsafe_background
        pico_mbedtls
        )

pico_add_extra_outputs(solar_station_v2)

