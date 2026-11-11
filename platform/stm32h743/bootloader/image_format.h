#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RATSENSE_IMAGE_MAGIC 0x52534657UL
#define RATSENSE_IMAGE_SIGNATURE_BYTES 64U

typedef struct {
    uint32_t magic;
    uint32_t header_version;
    uint32_t target_id;
    uint32_t image_version;
    uint32_t image_size;
    uint32_t min_bootloader_version;
    uint8_t sha256[32];
    uint8_t signature[RATSENSE_IMAGE_SIGNATURE_BYTES];
} ratsense_image_header_t;

bool ratsense_image_header_valid(const ratsense_image_header_t* header, size_t available_bytes);
bool ratsense_upgrade_safe_window(bool session_active, bool motor_running, bool storage_flushed, bool power_stable);

