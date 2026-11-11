# STM32H743 port

The C files here are a portable port contract for UART circular RX/IDLE
accounting, cache maintenance boundaries, FreeRTOS task budgets, and signed image
header policy. They are deliberately free of guessed HAL handles and pin numbers.

Once STM32CubeH7 and a generated NUCLEO-H743ZI2 project are available, replace the
port-contract functions with HAL/LL calls, add the real linker script/startup,
enable the vendor build option, and record the map/stack/cache evidence. Until
then the status is DESIGNED/IMPLEMENTED plus NEEDS_HARDWARE.

