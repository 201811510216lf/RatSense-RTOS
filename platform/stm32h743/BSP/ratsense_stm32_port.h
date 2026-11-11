#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    volatile uint8_t* data;
    size_t capacity;
    volatile size_t write_index;
    size_t read_index;
    uint32_t overflow_count;
    bool idle_seen;
} ratsense_uart_rx_ring_t;

void ratsense_uart_rx_ring_init(ratsense_uart_rx_ring_t* ring, volatile uint8_t* storage, size_t capacity);
void ratsense_uart_rx_dma_half_complete_isr(ratsense_uart_rx_ring_t* ring, size_t dma_index);
void ratsense_uart_rx_idle_isr(ratsense_uart_rx_ring_t* ring, size_t dma_index);
size_t ratsense_uart_rx_read(ratsense_uart_rx_ring_t* ring, uint8_t* out, size_t max_size);
bool ratsense_uart_tx_allowed(void);
void ratsense_motor_safe_off(void);

