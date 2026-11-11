# Verification matrix

| Deliverable | Status | Evidence / next verification |
|---|---|---|
| Host CMake + CTest | HOST_TESTED | MSVC build and CTest passed; out/verification/ctest_host_v1.txt |
| ASan/UBSan | BLOCKED_EXTERNAL | requires non-MSVC compiler; run host-asan preset |
| Pressure FSM | HOST_TESTED | tests/unit_tests.cpp and all simulator scenarios; then ADC/DMA HIL |
| 28BYJ sequence | HOST_TESTED | tests/unit_tests.cpp; phase capture and feed calibration HIL |
| IR counters | HOST_TESTED | tests/unit_tests.cpp; optical HIL |
| MLX pair/frame pool | HOST_TESTED | tests/unit_tests.cpp; real EEPROM/calibration HIL |
| RQ-GPR real model | BLOCKED_EXTERNAL | specified RAT-MONITOR branch unavailable; export/check real model |
| LCD framebuffer | HOST_TESTED | PBM/row command unit checks; SPI/VCOM HIL |
| Log persistence | HOST_TESTED | CRC/replay unit checks; SDMMC/card-removal HIL |
| FreeRTOS/static ISR boundary | DESIGNED/IMPLEMENTED | port contract and FREERTOS_DESIGN.md; CubeMX target build |
| STM32 UART RX DMA + IDLE | DESIGNED | port ring contract; HAL callback mapping and waveform HIL |
| UART COBS/CRC | HOST_TESTED | protocol unit checks and simulator faults; fuzz and two-board HIL |
| ESP Wi-Fi/UART/MQTT/TLS | DESIGNED/IMPLEMENTED | platform/esp32s3; idf.py build and board test |
| MQTT/backend | HOST_TESTED | Python tests and schema; Docker/e2e when daemon exists |
| OTA policy/package format | HOST_TESTED | Python policy tests and bootloader/image_format.c; flash/rollback HIL |
| Pin/power claims | NEEDS_HARDWARE | populate from confirmed schematic/board |

## Status definitions

DESIGNED = interface and rationale recorded.
IMPLEMENTED = source exists with bounded behaviour.
HOST_TESTED = executable tests passed on a real host toolchain.
TARGET_COMPILED = vendor target build passed.
HIL_TESTED = bench/instrumented test passed.
HARDWARE_VERIFIED = complete physical acceptance evidence.
BLOCKED_EXTERNAL = required external source/toolchain/service unavailable.
NEEDS_HARDWARE = physical hardware is required.

