# Porting and build notes

Host uses C++17 and CMake. The STM32 port contract uses C11 and is intentionally
vendor-neutral until a checked STM32CubeH7 version and CubeMX project are added.
The ESP project is an ESP-IDF component using official UART buffered APIs and
ESP-MQTT; it does not claim UHCI/GDMA UART support.

Target compile prerequisites:

- arm-none-eabi GCC, a pinned STM32CubeH7/CMSIS/FreeRTOS checkout, generated
  NUCLEO-H743ZI2 CubeMX sources, and an actual linker map.
- ESP-IDF version pinned in the local build environment, a module-specific
  sdkconfig, CA certificate provisioning, and a confirmed UART pin map.

If these are absent, the proper status is BLOCKED_EXTERNAL or NEEDS_HARDWARE.

