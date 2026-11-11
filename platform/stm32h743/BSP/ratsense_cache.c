#include <stddef.h>
#include <stdint.h>

void ratsense_cache_clean(void* address, size_t size) {
    (void)address;
    (void)size;
    /* Target implementation must call SCB_CleanDCache_by_Addr with 32-byte alignment. */
}

void ratsense_cache_invalidate(void* address, size_t size) {
    (void)address;
    (void)size;
    /* Target implementation must call SCB_InvalidateDCache_by_Addr with 32-byte alignment. */
}

uint32_t ratsense_dma_memory_is_target_accessible(const void* address) {
    (void)address;
    return 0U;
}

