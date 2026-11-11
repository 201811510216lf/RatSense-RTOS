# Memory, DMA and cache plan

STM32H7 DTCM/ITCM is not a generic DMA target. ADC/UART/SPI/SDMMC buffers must be
placed in a DMA-accessible SRAM region selected from the actual linker script.
Every DMA buffer is aligned to the 32-byte Cortex-M7 cache line or is placed in a
known non-cacheable MPU region.

CPU to DMA: clean the affected cache lines before starting transfer.
DMA to CPU: invalidate after completion and before consuming data. Do not let two
owners write the same cache line. SDMMC IDMA access is checked independently from
DMA1/DMA2; an ordinary ADC template cannot be reused for the card.

The Host pool and protocol use fixed bounded buffers. Target stack high-water,
linker map sizes, cache maintenance traces and deadline measurements are
NEEDS_HARDWARE and must be produced from a real map file and trace.

