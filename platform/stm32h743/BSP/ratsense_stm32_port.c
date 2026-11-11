#include "ratsense_stm32_port.h"

#include <string.h>

void ratsense_uart_rx_ring_init(ratsense_uart_rx_ring_t* ring, volatile uint8_t* storage, size_t capacity) {
    if (ring == NULL || storage == NULL || capacity == 0U) {
        return;
    }
    ring->data = storage;
    ring->capacity = capacity;
    ring->write_index = 0U;
    ring->read_index = 0U;
    ring->overflow_count = 0U;
    ring->idle_seen = false;
}

static void update_write_index(ratsense_uart_rx_ring_t* ring, size_t dma_index, bool idle) {
    if (ring == NULL || ring->capacity == 0U) {
        return;
    }
    const size_t next = dma_index % ring->capacity;
    const size_t unread = next >= ring->read_index ? next - ring->read_index
                                                   : ring->capacity - ring->read_index + next;
    if (unread == ring->capacity - 1U) {
        ++ring->overflow_count;
        ring->read_index = (ring->read_index + 1U) % ring->capacity;
    }
    ring->write_index = next;
    ring->idle_seen = idle;
}

void ratsense_uart_rx_dma_half_complete_isr(ratsense_uart_rx_ring_t* ring, size_t dma_index) {
    update_write_index(ring, dma_index, false);
}

void ratsense_uart_rx_idle_isr(ratsense_uart_rx_ring_t* ring, size_t dma_index) {
    update_write_index(ring, dma_index, true);
}

size_t ratsense_uart_rx_read(ratsense_uart_rx_ring_t* ring, uint8_t* out, size_t max_size) {
    if (ring == NULL || out == NULL || max_size == 0U || ring->data == NULL) {
        return 0U;
    }
    size_t copied = 0U;
    while (copied < max_size && ring->read_index != ring->write_index) {
        out[copied++] = ring->data[ring->read_index];
        ring->read_index = (ring->read_index + 1U) % ring->capacity;
    }
    return copied;
}

bool ratsense_uart_tx_allowed(void) {
    return true;
}

void ratsense_motor_safe_off(void) {
    /* CubeMX-generated GPIO writes belong here once the actual ULN2003 pins are confirmed. */
}

