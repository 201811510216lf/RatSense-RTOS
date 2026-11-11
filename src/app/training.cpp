#include "ratsense/app/training.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace ratsense::app {
namespace {

void put_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
}

drivers::RecordType record_type_for(const std::string& name) {
    if (name == "PRESS_DOWN" || name == "PRESS_UP" || name == "LONG_PRESS") {
        return drivers::RecordType::Press;
    }
    if (name == "FEED_ATTEMPT" || name == "FEED_COMPLETE") {
        return drivers::RecordType::FeedAttempt;
    }
    if (name == "PELLET_DETECTED" || name == "PELLET_REMOVED") {
        return drivers::RecordType::Pellet;
    }
    if (name == "FAULT" || name == "FEED_TIMEOUT") {
        return drivers::RecordType::Error;
    }
    return drivers::RecordType::Session;
}

}  // namespace

TrainingController::TrainingController(TrainingConfig config, std::filesystem::path log_path)
    : config_(config),
      motor_(drivers::MotorConfig{3U, config.motor_safety_timeout_ms, drivers::MotorMode::HalfStep}),
      log_(std::move(log_path)) {
    motor_.disable();
}

bool TrainingController::self_check(bool pressure_ok, bool thermal_ok, bool storage_ok) {
    state_ = TrainingState::SelfCheck;
    self_check_ok_ = pressure_ok && thermal_ok && storage_ok;
    if (!self_check_ok_ || !motor_.enable()) {
        state_ = TrainingState::Fault;
        emit("SELF_CHECK_FAILED", 0, false);
        return false;
    }
    state_ = TrainingState::Idle;
    emit("SELF_CHECK_OK", 0);
    return true;
}

bool TrainingController::start_session(std::uint64_t now_ms) {
    if (!self_check_ok_ || state_ != TrainingState::Idle || now_ms < next_feed_allowed_ms_) {
        return false;
    }
    session_id_ = now_ms == 0U ? 1U : now_ms;
    counters_ = {};
    next_feed_allowed_ms_ = now_ms;
    state_ = TrainingState::Armed;
    emit("SESSION_STARTED", now_ms);
    return true;
}

bool TrainingController::pause(std::uint64_t now_ms) {
    if (state_ != TrainingState::Armed && state_ != TrainingState::Feeding && state_ != TrainingState::AwaitPellet) {
        return false;
    }
    motor_.stop();
    state_ = TrainingState::Paused;
    emit("SESSION_PAUSED", now_ms);
    return true;
}

bool TrainingController::resume(std::uint64_t now_ms) {
    if (state_ != TrainingState::Paused || !self_check_ok_) {
        return false;
    }
    state_ = TrainingState::Armed;
    next_feed_allowed_ms_ = now_ms;
    emit("SESSION_RESUMED", now_ms);
    return true;
}

bool TrainingController::stop(std::uint64_t now_ms) {
    if (state_ == TrainingState::Boot || state_ == TrainingState::SelfCheck) {
        return false;
    }
    motor_.stop();
    state_ = TrainingState::Idle;
    emit("SESSION_STOPPED", now_ms);
    return true;
}

std::vector<TrainingEvent> TrainingController::on_pressure(const drivers::PressureEvent& event) {
    std::vector<TrainingEvent> emitted;
    const std::uint64_t now = event.sample.time_ms;
    if (event.type == drivers::PressureEventType::PressDown) {
        ++counters_.press_count;
        emit("PRESS_DOWN", now);
        if (state_ == TrainingState::Armed && now >= next_feed_allowed_ms_) {
            ++counters_.motor_attempt_count;
            if (motor_.start(config_.feed_steps, true, now)) {
                state_ = TrainingState::Feeding;
                feed_deadline_ms_ = now + config_.pellet_timeout_ms;
                emit("FEED_ATTEMPT", now);
                emitted.push_back({"FEED_ATTEMPT", now, next_event_id_ - 1U, true});
            } else {
                state_ = TrainingState::Fault;
                emit("FAULT", now, false);
                emitted.push_back({"FAULT", now, next_event_id_ - 1U, false});
            }
        }
    } else if (event.type == drivers::PressureEventType::LongPress) {
        emit("LONG_PRESS", now);
        emitted.push_back({"LONG_PRESS", now, next_event_id_ - 1U, true});
    } else if (event.type == drivers::PressureEventType::PressUp) {
        emit("PRESS_UP", now);
        emitted.push_back({"PRESS_UP", now, next_event_id_ - 1U, true});
    } else if (event.type == drivers::PressureEventType::Saturated ||
               event.type == drivers::PressureEventType::Disconnected) {
        emit("PRESSURE_SENSOR_FAULT", now, false);
        emitted.push_back({"PRESSURE_SENSOR_FAULT", now, next_event_id_ - 1U, false});
    }
    return emitted;
}

std::vector<TrainingEvent> TrainingController::tick(std::uint64_t now_ms) {
    std::vector<TrainingEvent> emitted;
    if (state_ == TrainingState::Feeding) {
        const drivers::MotorEvent motor_event = motor_.tick(now_ms);
        if (motor_event.type == drivers::MotorEvent::Type::Completed) {
            state_ = TrainingState::AwaitPellet;
            feed_deadline_ms_ = now_ms + config_.pellet_timeout_ms;
            emit("FEED_COMPLETE", now_ms);
            emitted.push_back({"FEED_COMPLETE", now_ms, next_event_id_ - 1U, true});
        } else if (motor_event.type == drivers::MotorEvent::Type::TimedOut ||
                   motor_event.type == drivers::MotorEvent::Type::Fault) {
            state_ = TrainingState::Fault;
            emit("FAULT", now_ms, false);
            emitted.push_back({"FAULT", now_ms, next_event_id_ - 1U, false});
        }
    } else if (state_ == TrainingState::AwaitPellet && now_ms >= feed_deadline_ms_) {
        motor_.stop();
        state_ = TrainingState::Fault;
        emit("FEED_TIMEOUT", now_ms, false);
        emitted.push_back({"FEED_TIMEOUT", now_ms, next_event_id_ - 1U, false});
    }
    return emitted;
}

std::vector<TrainingEvent> TrainingController::on_ir(const drivers::IrEvent& event) {
    std::vector<TrainingEvent> emitted;
    if (event.event_id == 0U || event.event_id <= last_ir_event_id_) {
        return emitted;
    }
    last_ir_event_id_ = event.event_id;
    if (event.type == drivers::IrEventType::PelletPassed) {
        ++counters_.pellet_pass_count;
        emit("PELLET_DETECTED", event.time_ms);
        emitted.push_back({"PELLET_DETECTED", event.time_ms, next_event_id_ - 1U, true});
        if (state_ == TrainingState::AwaitPellet) {
            state_ = TrainingState::Armed;
            next_feed_allowed_ms_ = event.time_ms + config_.feed_cooldown_ms;
        }
    } else {
        ++counters_.pellet_removed_count;
        emit("PELLET_REMOVED", event.time_ms);
        emitted.push_back({"PELLET_REMOVED", event.time_ms, next_event_id_ - 1U, true});
    }
    return emitted;
}

void TrainingController::emit(const std::string& name, std::uint64_t now_ms, bool success) {
    TrainingEvent event{name, now_ms, next_event_id_++, success};
    std::vector<std::uint8_t> payload;
    payload.reserve(24U + name.size());
    put_u64(payload, event.event_id);
    put_u64(payload, session_id_);
    payload.push_back(success ? 1U : 0U);
    payload.insert(payload.end(), name.begin(), name.end());
    drivers::EventRecord record{};
    record.type = record_type_for(name);
    record.session_id = session_id_;
    record.monotonic_ms = now_ms;
    record.rtc_ms = 0;
    record.payload = payload;
    log_.append(record);

    protocol::MessageType type = protocol::MessageType::ErrorEvent;
    if (name == "PRESS_DOWN" || name == "PRESS_UP" || name == "LONG_PRESS") {
        type = protocol::MessageType::PressEvent;
    } else if (name == "FEED_ATTEMPT" || name == "FEED_COMPLETE") {
        type = protocol::MessageType::FeedAttempt;
    } else if (name == "PELLET_DETECTED" || name == "PELLET_REMOVED") {
        type = protocol::MessageType::PelletDetected;
    }
    protocol::Frame frame{};
    frame.message_type = type;
    frame.flags = protocol::AckRequested;
    frame.sequence = static_cast<std::uint32_t>(event.event_id);
    frame.device_time_ms = now_ms;
    frame.payload = payload;
    protocol_events_.push_back(std::move(frame));
}

std::vector<protocol::Frame> TrainingController::drain_protocol_events() {
    std::vector<protocol::Frame> result;
    while (!protocol_events_.empty()) {
        result.push_back(std::move(protocol_events_.front()));
        protocol_events_.pop_front();
    }
    return result;
}

const char* training_state_name(TrainingState state) {
    switch (state) {
        case TrainingState::Boot:
            return "boot";
        case TrainingState::SelfCheck:
            return "self_check";
        case TrainingState::Idle:
            return "idle";
        case TrainingState::Armed:
            return "armed";
        case TrainingState::Feeding:
            return "feeding";
        case TrainingState::AwaitPellet:
            return "await_pellet";
        case TrainingState::Paused:
            return "paused";
        case TrainingState::Fault:
            return "fault";
    }
    return "unknown";
}

}  // namespace ratsense::app

