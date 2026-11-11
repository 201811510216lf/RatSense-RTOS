#pragma once

#include <cstdint>
#include <optional>

namespace ratsense::drivers {

enum class IrEventType { PelletPassed, PelletRemoved };

struct IrConfig {
    bool active_high{true};
    std::uint32_t debounce_ms{20U};
    std::uint32_t min_pulse_ms{1U};
    std::uint32_t max_pulse_ms{1000U};
    bool second_sensor_supported{false};
};

struct IrEvent {
    IrEventType type{IrEventType::PelletPassed};
    std::uint64_t time_ms{0};
    std::uint32_t pulse_ms{0};
    std::uint64_t event_id{0};
};

class PelletIr {
public:
    explicit PelletIr(IrConfig config = {});
    std::optional<IrEvent> edge(bool level, std::uint64_t now_ms, bool second_sensor = false);
    void reset();
    std::uint64_t pellet_passed() const { return pellet_passed_; }
    std::uint64_t pellet_removed() const { return pellet_removed_; }
    const IrConfig& config() const { return config_; }

private:
    IrConfig config_;
    bool active_{false};
    bool second_active_{false};
    std::uint64_t active_start_ms_{0};
    std::uint64_t second_start_ms_{0};
    std::uint64_t last_event_ms_{0};
    std::uint64_t next_event_id_{1};
    std::uint64_t pellet_passed_{0};
    std::uint64_t pellet_removed_{0};
};

}  // namespace ratsense::drivers

