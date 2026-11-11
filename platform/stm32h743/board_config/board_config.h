#pragma once

#include <stdint.h>

/*
 * GPIO and DMA instances are intentionally TBD until the exact NUCLEO-H743ZI2
 * board revision, CubeMX pinmux, sensor modules, SD socket and level shifting
 * are confirmed. Do not replace these values with guesses.
 */
#define RATSENSE_BOARD_NAME "NUCLEO-H743ZI2/TBD"
#define RATSENSE_UART_BAUD_CANDIDATE 921600U
#define RATSENSE_UART_BAUD_FALLBACK 115200U
#define RATSENSE_ADC_DMA_HALF_SAMPLES 32U
#define RATSENSE_CACHE_LINE_BYTES 32U

typedef struct {
    uint32_t pressure_adc_instance;
    uint32_t pressure_timer_instance;
    uint32_t stm32_esp_uart_instance;
    uint32_t sdmmc_instance;
    uint32_t sharp_spi_instance;
} ratsense_board_instances_t;

static inline ratsense_board_instances_t ratsense_board_instances(void) {
    return (ratsense_board_instances_t){0U, 0U, 0U, 0U, 0U};
}

