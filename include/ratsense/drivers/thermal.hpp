#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ratsense::drivers {

constexpr std::size_t kMlxPixelCount = 768U;
constexpr std::size_t kMlxRawWordCount = 834U;

enum class ThermalQuality : std::uint32_t {
    None = 0,
    SubpagePairValid = 1U << 0U,
    CalibrationValid = 1U << 1U,
    AmbientValid = 1U << 2U,
    OcclusionSuspected = 1U << 3U,
    Stale = 1U << 4U,
    I2cError = 1U << 5U,
    ModelInvalid = 1U << 6U,
};

constexpr ThermalQuality operator|(ThermalQuality a, ThermalQuality b) {
    return static_cast<ThermalQuality>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
constexpr bool has_quality(ThermalQuality value, ThermalQuality bit) {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(bit)) != 0U;
}

struct MlxSubpage {
    std::uint8_t index{0};
    std::uint64_t time_ms{0};
    std::array<std::uint16_t, kMlxRawWordCount> raw{};
};

struct MlxPair {
    MlxSubpage first{};
    MlxSubpage second{};
    ThermalQuality quality{ThermalQuality::None};
};

class MlxSubpagePairer {
public:
    std::optional<MlxPair> push(const MlxSubpage& subpage);
    void reset();
    std::uint32_t dropped_subpages() const { return dropped_subpages_; }

private:
    std::optional<MlxSubpage> pending_;
    std::uint32_t dropped_subpages_{0};
};

struct ThermalFrame {
    std::array<std::uint16_t, kMlxRawWordCount> raw{};
    std::array<float, kMlxPixelCount> pixels{};
    std::uint64_t time_ms{0};
    ThermalQuality quality{ThermalQuality::None};
    std::uint8_t slot{0};
};

class ThermalFramePool {
public:
    enum class State : std::uint8_t { Free, Filling, Ready, Processing };
    ThermalFramePool();
    ThermalFrame* acquire_fill();
    bool publish_ready(ThermalFrame* frame);
    ThermalFrame* acquire_ready();
    bool release(ThermalFrame* frame);
    State state(std::size_t index) const;
    std::size_t ready_count() const;

private:
    std::array<ThermalFrame, 2> frames_{};
    std::array<State, 2> states_{State::Free, State::Free};
};

std::uint8_t htu21d_crc8(const std::uint8_t* data, std::size_t size);
bool htu21d_crc_valid(std::uint16_t raw_value, std::uint8_t received_crc);

}  // namespace ratsense::drivers

