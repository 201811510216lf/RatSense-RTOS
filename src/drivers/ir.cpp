#include "ratsense/drivers/ir.hpp"

namespace ratsense::drivers {

PelletIr::PelletIr(IrConfig config) : config_(config) {}

void PelletIr::reset() {
    active_ = false;
    second_active_ = false;
    active_start_ms_ = 0;
    second_start_ms_ = 0;
    last_event_ms_ = 0;
    next_event_id_ = 1;
    pellet_passed_ = 0;
    pellet_removed_ = 0;
}

std::optional<IrEvent> PelletIr::edge(bool level, std::uint64_t now_ms, bool second_sensor) {
    const bool active = level == config_.active_high;
    bool& state = second_sensor ? second_active_ : active_;
    std::uint64_t& start = second_sensor ? second_start_ms_ : active_start_ms_;
    if (active == state) {
        return std::nullopt;
    }
    state = active;
    if (active) {
        start = now_ms;
        return std::nullopt;
    }
    const std::uint64_t pulse_ms = now_ms >= start ? now_ms - start : 0U;
    if (pulse_ms < config_.min_pulse_ms || pulse_ms > config_.max_pulse_ms ||
        now_ms - last_event_ms_ < config_.debounce_ms) {
        return std::nullopt;
    }
    last_event_ms_ = now_ms;
    const IrEventType type = second_sensor ? IrEventType::PelletRemoved : IrEventType::PelletPassed;
    const std::uint64_t id = next_event_id_++;
    if (second_sensor) {
        if (!config_.second_sensor_supported) {
            return std::nullopt;
        }
        ++pellet_removed_;
    } else {
        ++pellet_passed_;
    }
    return IrEvent{type, now_ms, static_cast<std::uint32_t>(pulse_ms), id};
}

}  // namespace ratsense::drivers

