# Pinmux plan

This document intentionally uses signal names rather than guessed GPIO numbers.

| Signal | MCU | Candidate peripheral | State |
|---|---|---|---|
| pressure ADC trigger/DMA | STM32 | ADC + TIM trigger + DMA circular | TBD |
| MLX90640 + HTU21D | STM32 | I2C with bus lock | TBD |
| Sharp P3502 | STM32 | SPI TX DMA + CS/VCOM/EXTMOD/EXTIN/DISP GPIO | TBD |
| 28BYJ-48 phases A-D | STM32 | four GPIO outputs to ULN2003 | TBD |
| pellet IR | STM32 | EXTI/timer capture | TBD |
| microSD | STM32 | SDMMC 4-bit preferred, SPI fallback | TBD |
| STM32-ESP link | both | dedicated UART, 3.3 V CMOS | TBD |
| debug console | both | separate UART/USB | TBD |

CubeMX must check alternate functions, DMA request conflicts, D-cache/SRAM
accessibility and NVIC priority. ESP32-S3 must avoid boot-strapping, USB,
flash/PSRAM and strapping conflicts. Populate pinmap.csv only after the physical
boards and schematic are known.

