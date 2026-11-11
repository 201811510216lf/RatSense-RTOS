cmake_minimum_required(VERSION 3.20)
project(ratsense_stm32h743 C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
option(RATSENSE_STM32_VENDOR_HAL "Build against a checked STM32CubeH7 checkout" OFF)

add_library(ratsense_stm32_port STATIC
    BSP/ratsense_stm32_port.c
    BSP/ratsense_cache.c
    FreeRTOS/ratsense_freertos_bridge.c
    bootloader/image_format.c
)
target_include_directories(ratsense_stm32_port PUBLIC
    BSP
    FreeRTOS
    bootloader
    board_config
)
target_compile_definitions(ratsense_stm32_port PUBLIC
    RATSENSE_STM32H743=1
    RATSENSE_TARGET_COMPILED=0
)
target_compile_options(ratsense_stm32_port PRIVATE -Wall -Wextra -Wpedantic)

if(RATSENSE_STM32_VENDOR_HAL)
    message(FATAL_ERROR "Provide the checked STM32CubeH7 path and generated CubeMX sources before enabling vendor HAL.")
endif()

message(STATUS "STM32 port-contract library only; real HAL/CubeMX target remains NEEDS_HARDWARE.")

