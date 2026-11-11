#include "image_format.h"

bool ratsense_image_header_valid(const ratsense_image_header_t* header, size_t available_bytes) {
    if (header == 0 || available_bytes < sizeof(ratsense_image_header_t)) {
        return false;
    }
    if (header->magic != RATSENSE_IMAGE_MAGIC || header->header_version == 0U || header->image_size == 0U) {
        return false;
    }
    return true;
}

bool ratsense_upgrade_safe_window(bool session_active, bool motor_running, bool storage_flushed, bool power_stable) {
    return !session_active && !motor_running && storage_flushed && power_stable;
}

