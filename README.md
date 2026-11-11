# RatSense-RTOS

RatSense-RTOS is a safety-oriented STM32H743 + ESP32-S3 rat behaviour training and
temperature-monitoring reference implementation. The Host build is deterministic and
contains the protocol, driver logic, training state machine, thermal frame pool,
synthetic RQ-GPR inference, Sharp P3502 framebuffer, durable event log, simulator,
gateway simulator, and fault-injection tests. Hardware-specific code is separated and
marked for bring-up.

The current workspace has no development boards and the C++ toolchain was not present
in the execution environment on 2026-11-11. The source and commands below are still
kept reproducible; docs/PROGRESS.md records the exact verification status.

## Host build

    cmake -S . -B build/host -DRATSENSE_PLATFORM=HOST_SIM -DCMAKE_BUILD_TYPE=Debug
    cmake --build build/host -j
    ctest --test-dir build/host --output-on-failure
    build/host/ratsense_sim --scenario normal
    build/host/ratsense_sim --scenario uart_crc_error
    build/host/ratsense_sim --scenario esp_reset

On Windows PowerShell, replace the last three paths with build\host\ratsense_sim.exe,
etc. The ASan/UBSan configuration is:

    cmake -S . -B build/host-asan -DRATSENSE_PLATFORM=HOST_SIM -DRATSENSE_ENABLE_ASAN=ON
    cmake --build build/host-asan -j
    ctest --test-dir build/host-asan --output-on-failure

The verified Windows build used Visual Studio 2022 MSVC through the absolute tool
paths discovered on the workstation. Its CTest and simulator evidence is kept in
out/verification; see docs/VERIFICATION_LOG.md.

## Target builds

STM32 and ESP32 projects are intentionally independent:

    cmake -S platform/stm32h743 -B build/stm32 -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-none-eabi.cmake
    cmake --build build/stm32 -j
    cd platform/esp32s3
    idf.py set-target esp32s3
    idf.py build

These commands require the matching vendor SDK/toolchain. They are not claimed as
compiled until the toolchain is installed and the resulting logs are recorded.

## Local server

With Docker:

    cd server
    docker compose up --build -d

The API binds to localhost by default. See server/README.md and docs/MQTT_SCHEMA.md
for topics, authentication, persistence, and the offline/replay model.

## Safety and verification status

The motor is disabled on boot and can only be enabled by the local training state
machine after self-check. Remote commands never directly toggle motor GPIO. Real
pinmux, voltage levels, DMA/cache placement, LCD VCOM, SDMMC wiring, RF/MQTT TLS,
OTA rollback, and all physical actuator behaviour are NEEDS_HARDWARE.

Read the continuation state in docs/PROGRESS.md, the test matrix in
docs/VERIFICATION_MATRIX.md, and the next actions in docs/NEXT_ACTIONS.md.

