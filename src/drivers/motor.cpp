#include "ratsense/drivers/motor.hpp"

#include <algorithm>

namespace ratsense::drivers {
namespace {

constexpr std::array<std::array<bool, 4>, 8> kHalfStep{{
    {{true, false, false, false}},
    {{true, true, false, false}},
    {{false, true, false, false}},
    {{false, true, true, false}},
    {{false, false, true, false}},
    {{false, false, true, true}},
    {{false, false, false, true}},
    {{true, false, false, true}},
}};

constexpr std::array<std::array<bool, 4>, 4> kFullStep{{
    {{true, true, false, false}},
    {{false, true, true, false}},
    {{false, false, true, true}},
    {{true, false, false, true}},
}};

MotorEvent make_event(MotorEvent::Type type, std::uint32_t index, const std::array<bool, 4>& coils) {
    return MotorEvent{type, index, coils};
}

}  // namespace

Motor28BYJ48::Motor28BYJ48(MotorConfig config) : config_(config) {}

bool Motor28BYJ48::enable() {
    if (state_ == MotorState::Fault) {
        return false;
    }
    enabled_ = true;
    if (state_ == MotorState::Disabled) {
        state_ = MotorState::Idle;
    }
    return true;
}

void Motor28BYJ48::disable() {
    enabled_ = false;
    remaining_steps_ = 0;
    coil_outputs_ = {false, false, false, false};
    state_ = MotorState::Disabled;
}

bool Motor28BYJ48::start(std::uint32_t steps, bool forward, std::uint64_t now_ms) {
    if (!enabled_ || steps == 0U || state_ == MotorState::Running || state_ == MotorState::Fault) {
        return false;
    }
    forward_ = forward;
    total_steps_ = steps;
    remaining_steps_ = steps;
    step_index_ = forward_ ? 0U : static_cast<std::uint32_t>(config_.mode == MotorMode::HalfStep ? 7U : 3U);
    started_ms_ = now_ms;
    next_step_ms_ = now_ms;
    state_ = MotorState::Running;
    if (config_.mode == MotorMode::HalfStep) {
        coil_outputs_ = half_step_sequence()[step_index_ % half_step_sequence().size()];
    } else {
        coil_outputs_ = full_step_sequence()[step_index_ % full_step_sequence().size()];
    }
    return true;
}

MotorEvent Motor28BYJ48::tick(std::uint64_t now_ms) {
    if (state_ != MotorState::Running) {
        return make_event(MotorEvent::Type::PhaseChanged, step_index_, coil_outputs_);
    }
    if (now_ms - started_ms_ > config_.safety_timeout_ms) {
        remaining_steps_ = 0;
        coil_outputs_ = {false, false, false, false};
        state_ = MotorState::Fault;
        return make_event(MotorEvent::Type::TimedOut, step_index_, coil_outputs_);
    }
    if (now_ms < next_step_ms_) {
        return make_event(MotorEvent::Type::PhaseChanged, step_index_, coil_outputs_);
    }
    const std::size_t count = config_.mode == MotorMode::HalfStep ? half_step_sequence().size() : full_step_sequence().size();
    if (remaining_steps_ > 0U) {
        --remaining_steps_;
    }
    if (remaining_steps_ == 0U) {
        coil_outputs_ = {false, false, false, false};
        state_ = MotorState::Complete;
        return make_event(MotorEvent::Type::Completed, total_steps_, coil_outputs_);
    }
    const std::uint32_t direction = forward_ ? 1U : static_cast<std::uint32_t>(count - 1U);
    step_index_ = (step_index_ + direction) % static_cast<std::uint32_t>(count);
    if (config_.mode == MotorMode::HalfStep) {
        coil_outputs_ = half_step_sequence()[step_index_ % half_step_sequence().size()];
    } else {
        coil_outputs_ = full_step_sequence()[step_index_ % full_step_sequence().size()];
    }
    next_step_ms_ = now_ms + std::max<std::uint32_t>(1U, config_.step_period_ms);
    return make_event(MotorEvent::Type::PhaseChanged, total_steps_ - remaining_steps_, coil_outputs_);
}

MotorEvent Motor28BYJ48::stop() {
    remaining_steps_ = 0;
    coil_outputs_ = {false, false, false, false};
    if (enabled_) {
        state_ = MotorState::Idle;
    } else {
        state_ = MotorState::Disabled;
    }
    return make_event(MotorEvent::Type::Stopped, 0, coil_outputs_);
}

const std::array<std::array<bool, 4>, 8>& Motor28BYJ48::half_step_sequence() {
    return kHalfStep;
}

const std::array<std::array<bool, 4>, 4>& Motor28BYJ48::full_step_sequence() {
    return kFullStep;
}

}  // namespace ratsense::drivers

