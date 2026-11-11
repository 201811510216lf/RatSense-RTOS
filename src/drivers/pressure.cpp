#include "ratsense/drivers/pressure.hpp"

#include <algorithm>
#include <cmath>

namespace ratsense::drivers {

PressureProcessor::PressureProcessor(PressureConfig config) : config_(config) {
    config_.filter_alpha = std::clamp(config_.filter_alpha, 0.001F, 1.0F);
    config_.baseline_alpha = std::clamp(config_.baseline_alpha, 0.0001F, 1.0F);
    config_.threshold_off_v = std::min(config_.threshold_off_v, config_.threshold_on_v);
}

void PressureProcessor::reset() {
    filtered_ = 0.0F;
    baseline_ = 0.0F;
    press_start_ms_ = 0;
    last_edge_ms_ = 0;
    last_press_ms_ = 0;
    initialized_ = false;
    pressed_ = false;
    long_reported_ = false;
    press_count_ = 0;
}

std::optional<PressureEvent> PressureProcessor::process_sample(std::uint16_t raw, std::uint64_t time_ms) {
    const std::uint16_t bounded = std::min(raw, config_.adc_full_scale);
    const float voltage = static_cast<float>(bounded) * config_.adc_reference_v /
                          static_cast<float>(config_.adc_full_scale);
    const bool saturated = raw >= config_.adc_full_scale;
    if (!initialized_) {
        filtered_ = voltage;
        baseline_ = voltage;
        initialized_ = true;
    } else {
        filtered_ += config_.filter_alpha * (voltage - filtered_);
        if (!pressed_) {
            baseline_ += config_.baseline_alpha * (filtered_ - baseline_);
        }
    }
    const float signal = std::max(0.0F, filtered_ - baseline_);
    const bool above_on = signal >= config_.threshold_on_v;
    const bool below_off = signal <= config_.threshold_off_v;
    PressureSample sample{raw, voltage, filtered_, baseline_, time_ms, saturated};

    if (saturated && !pressed_) {
        return PressureEvent{PressureEventType::Saturated, sample};
    }
    if (!pressed_ && above_on && (time_ms - last_edge_ms_ >= config_.debounce_ms)) {
        if (time_ms - last_press_ms_ < config_.repeat_min_interval_ms && press_count_ != 0U) {
            return std::nullopt;
        }
        pressed_ = true;
        press_start_ms_ = time_ms;
        last_edge_ms_ = time_ms;
        last_press_ms_ = time_ms;
        long_reported_ = false;
        ++press_count_;
        return PressureEvent{PressureEventType::PressDown, sample};
    }
    if (pressed_ && below_off && (time_ms - last_edge_ms_ >= config_.debounce_ms)) {
        pressed_ = false;
        last_edge_ms_ = time_ms;
        long_reported_ = false;
        return PressureEvent{PressureEventType::PressUp, sample};
    }
    if (pressed_ && !long_reported_ && time_ms - press_start_ms_ >= config_.long_press_ms) {
        long_reported_ = true;
        return PressureEvent{PressureEventType::LongPress, sample};
    }
    if (raw <= 1U && filtered_ < 0.01F && !pressed_) {
        return PressureEvent{PressureEventType::Disconnected, sample};
    }
    return std::nullopt;
}

std::vector<PressureEvent> PressureProcessor::consume_dma_half(const std::uint16_t* raw, std::size_t count,
                                                                std::uint64_t start_ms) {
    std::vector<PressureEvent> events;
    if (raw == nullptr || count == 0U) {
        return events;
    }
    const std::uint32_t rate = std::max<std::uint32_t>(1U, config_.sample_rate_hz);
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint64_t sample_ms = start_ms + (i * 1000U) / rate;
        const std::optional<PressureEvent> event = process_sample(raw[i], sample_ms);
        if (event.has_value()) {
            events.push_back(*event);
        }
    }
    return events;
}

std::uint64_t PressureProcessor::worst_case_dma_latency_ms() const {
    const std::uint32_t rate = std::max<std::uint32_t>(1U, config_.sample_rate_hz);
    return (static_cast<std::uint64_t>(config_.dma_half_samples) * 1000U + rate - 1U) / rate;
}

}  // namespace ratsense::drivers

