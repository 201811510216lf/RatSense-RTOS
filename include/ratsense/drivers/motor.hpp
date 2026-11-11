#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ratsense::drivers {

enum class MotorState { Disabled, Idle, Running, Complete, Fault };
enum class MotorMode { HalfStep, FullStep };

struct MotorConfig {
    std::uint32_t step_period_ms{3U};
    std::uint32_t safety_timeout_ms{5000U};
    MotorMode mode{MotorMode::HalfStep};
};

struct MotorEvent {
    enum class Type { Started, PhaseChanged, Completed, Stopped, TimedOut, Fault };
    Type type{Type::PhaseChanged};
    std::uint32_t step_index{0};
    std::array<bool, 4> coils{false, false, false, false};
};

class Motor28BYJ48 {
public:
    explicit Motor28BYJ48(MotorConfig config = {});
    bool enable();
    void disable();
    bool start(std::uint32_t steps, bool forward, std::uint64_t now_ms);
    MotorEvent tick(std::uint64_t now_ms);
    MotorEvent stop();
    MotorState state() const { return state_; }
    std::uint32_t remaining_steps() const { return remaining_steps_; }
    std::uint32_t total_steps() const { return total_steps_; }
    std::array<bool, 4> coil_outputs() const { return coil_outputs_; }
    static const std::array<std::array<bool, 4>, 8>& half_step_sequence();
    static const std::array<std::array<bool, 4>, 4>& full_step_sequence();

private:
    MotorConfig config_;
    MotorState state_{MotorState::Disabled};
    bool enabled_{false};
    bool forward_{true};
    std::uint32_t total_steps_{0};
    std::uint32_t remaining_steps_{0};
    std::uint32_t step_index_{0};
    std::uint64_t started_ms_{0};
    std::uint64_t next_step_ms_{0};
    std::array<bool, 4> coil_outputs_{false, false, false, false};
};

}  // namespace ratsense::drivers

