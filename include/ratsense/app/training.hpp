#pragma once

#include "ratsense/drivers/ir.hpp"
#include "ratsense/drivers/motor.hpp"
#include "ratsense/drivers/pressure.hpp"
#include "ratsense/drivers/storage.hpp"
#include "ratsense/model/rq_gpr.hpp"
#include "ratsense/protocol/frame.hpp"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ratsense::app {

enum class TrainingState { Boot, SelfCheck, Idle, Armed, Feeding, AwaitPellet, Paused, Fault };

struct TrainingConfig {
    std::uint32_t feed_steps{32U};
    std::uint32_t pellet_timeout_ms{1500U};
    std::uint32_t feed_cooldown_ms{1000U};
    std::uint32_t motor_safety_timeout_ms{5000U};
    bool second_ir_supported{false};
};

struct TrainingCounters {
    std::uint64_t press_count{0};
    std::uint64_t motor_attempt_count{0};
    std::uint64_t pellet_pass_count{0};
    std::uint64_t pellet_removed_count{0};
};

struct TrainingEvent {
    std::string name;
    std::uint64_t time_ms{0};
    std::uint64_t event_id{0};
    bool success{true};
};

class TrainingController {
public:
    TrainingController(TrainingConfig config = {}, std::filesystem::path log_path = "ratsense.log");
    bool self_check(bool pressure_ok, bool thermal_ok, bool storage_ok);
    bool start_session(std::uint64_t now_ms);
    bool pause(std::uint64_t now_ms);
    bool resume(std::uint64_t now_ms);
    bool stop(std::uint64_t now_ms);
    std::vector<TrainingEvent> on_pressure(const drivers::PressureEvent& event);
    std::vector<TrainingEvent> tick(std::uint64_t now_ms);
    std::vector<TrainingEvent> on_ir(const drivers::IrEvent& event);
    TrainingState state() const { return state_; }
    const TrainingCounters& counters() const { return counters_; }
    const drivers::Motor28BYJ48& motor() const { return motor_; }
    drivers::Motor28BYJ48& motor() { return motor_; }
    drivers::LogStore& log() { return log_; }
    std::uint64_t session_id() const { return session_id_; }
    std::vector<protocol::Frame> drain_protocol_events();

private:
    TrainingConfig config_;
    TrainingState state_{TrainingState::Boot};
    TrainingCounters counters_{};
    drivers::Motor28BYJ48 motor_{};
    drivers::LogStore log_;
    std::uint64_t session_id_{0};
    std::uint64_t next_event_id_{1};
    std::uint64_t feed_deadline_ms_{0};
    std::uint64_t next_feed_allowed_ms_{0};
    std::uint64_t last_ir_event_id_{0};
    std::deque<protocol::Frame> protocol_events_;
    bool self_check_ok_{false};

    void emit(const std::string& name, std::uint64_t now_ms, bool success = true);
};

const char* training_state_name(TrainingState state);

}  // namespace ratsense::app

