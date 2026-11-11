#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ratsense::drivers {

enum class PressureEventType { PressDown, PressUp, LongPress, Saturated, Disconnected };

struct PressureConfig {
    float adc_reference_v{3.3F};
    std::uint16_t adc_full_scale{4095U};
    float threshold_on_v{1.20F};
    float threshold_off_v{0.95F};
    float filter_alpha{0.20F};
    float baseline_alpha{0.005F};
    std::uint32_t debounce_ms{30U};
    std::uint32_t long_press_ms{1000U};
    std::uint32_t repeat_min_interval_ms{500U};
    std::uint32_t sample_rate_hz{1000U};
    std::size_t dma_half_samples{32U};
};

struct PressureSample {
    std::uint16_t raw{0};
    float voltage{0.0F};
    float filtered_voltage{0.0F};
    float baseline_voltage{0.0F};
    std::uint64_t time_ms{0};
    bool saturated{false};
};

struct PressureEvent {
    PressureEventType type{PressureEventType::PressDown};
    PressureSample sample{};
};

class PressureProcessor {
public:
    explicit PressureProcessor(PressureConfig config = {});
    void reset();
    std::optional<PressureEvent> process_sample(std::uint16_t raw, std::uint64_t time_ms);
    std::vector<PressureEvent> consume_dma_half(const std::uint16_t* raw, std::size_t count, std::uint64_t start_ms);
    const PressureConfig& config() const { return config_; }
    std::uint64_t worst_case_dma_latency_ms() const;
    bool pressed() const { return pressed_; }
    std::uint32_t press_count() const { return press_count_; }

private:
    PressureConfig config_;
    float filtered_{0.0F};
    float baseline_{0.0F};
    std::uint64_t press_start_ms_{0};
    std::uint64_t last_edge_ms_{0};
    std::uint64_t last_press_ms_{0};
    bool initialized_{false};
    bool pressed_{false};
    bool long_reported_{false};
    std::uint32_t press_count_{0};
};

}  // namespace ratsense::drivers

