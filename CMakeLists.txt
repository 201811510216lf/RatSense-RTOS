cmake_minimum_required(VERSION 3.20)
project(RatSenseRTOS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(RATSENSE_ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer on Host" OFF)
set(RATSENSE_PLATFORM "HOST_SIM" CACHE STRING "Build platform: HOST_SIM or STM32H743")

if(RATSENSE_PLATFORM STREQUAL "HOST_SIM")
    enable_testing()
    add_library(ratsense_core
        src/protocol/frame.cpp
        src/drivers/pressure.cpp
        src/drivers/motor.cpp
        src/drivers/ir.cpp
        src/drivers/thermal.cpp
        src/model/rq_gpr.cpp
        src/drivers/display.cpp
        src/drivers/storage.cpp
        src/app/training.cpp
    )
    target_include_directories(ratsense_core PUBLIC include)
    target_compile_definitions(ratsense_core PUBLIC RATSENSE_HOST_SIM=1)
    if(MSVC)
        target_compile_options(ratsense_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ratsense_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    if(RATSENSE_ENABLE_ASAN AND NOT MSVC)
        target_compile_options(ratsense_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(ratsense_core PUBLIC -fsanitize=address,undefined)
    endif()

    add_executable(ratsense_sim tools/ratsense_sim.cpp)
    target_link_libraries(ratsense_sim PRIVATE ratsense_core)
    add_executable(ratsense_gateway_sim tools/ratsense_gateway_sim.cpp)
    target_link_libraries(ratsense_gateway_sim PRIVATE ratsense_core)
    add_executable(uart_frame_tool tools/uart_frame_tool.cpp)
    target_link_libraries(uart_frame_tool PRIVATE ratsense_core)
    add_executable(lcd_preview tools/lcd_preview.cpp)
    target_link_libraries(lcd_preview PRIVATE ratsense_core)
    add_executable(log_decoder tools/log_decoder.cpp)
    target_link_libraries(log_decoder PRIVATE ratsense_core)

    add_executable(ratsense_unit_tests tests/unit_tests.cpp)
    target_link_libraries(ratsense_unit_tests PRIVATE ratsense_core)
    add_test(NAME ratsense_unit_tests COMMAND ratsense_unit_tests)
else()
    message(FATAL_ERROR "Target platform '${RATSENSE_PLATFORM}' is not configured in this host CMake. Use platform/stm32h743 with the ARM toolchain.")
endif()

