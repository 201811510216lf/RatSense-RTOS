#include "ratsense/drivers/thermal.hpp"

#include <algorithm>

namespace ratsense::drivers {

std::optional<MlxPair> MlxSubpagePairer::push(const MlxSubpage& subpage) {
    if (subpage.index > 1U) {
        ++dropped_subpages_;
        return std::nullopt;
    }
    if (!pending_.has_value()) {
        pending_ = subpage;
        return std::nullopt;
    }
    if (pending_->index == subpage.index) {
        pending_ = subpage;
        ++dropped_subpages_;
        return std::nullopt;
    }
    MlxPair pair{};
    pair.first = *pending_;
    pair.second = subpage;
    pair.quality = ThermalQuality::SubpagePairValid | ThermalQuality::CalibrationValid;
    pending_.reset();
    return pair;
}

void MlxSubpagePairer::reset() {
    pending_.reset();
    dropped_subpages_ = 0;
}

ThermalFramePool::ThermalFramePool() {
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        frames_[i].slot = static_cast<std::uint8_t>(i);
    }
}

ThermalFrame* ThermalFramePool::acquire_fill() {
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        if (states_[i] == State::Free) {
            states_[i] = State::Filling;
            return &frames_[i];
        }
    }
    return nullptr;
}

bool ThermalFramePool::publish_ready(ThermalFrame* frame) {
    if (frame == nullptr) {
        return false;
    }
    const std::size_t index = frame->slot;
    if (index >= frames_.size() || &frames_[index] != frame || states_[index] != State::Filling) {
        return false;
    }
    states_[index] = State::Ready;
    return true;
}

ThermalFrame* ThermalFramePool::acquire_ready() {
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        if (states_[i] == State::Ready) {
            states_[i] = State::Processing;
            return &frames_[i];
        }
    }
    return nullptr;
}

bool ThermalFramePool::release(ThermalFrame* frame) {
    if (frame == nullptr) {
        return false;
    }
    const std::size_t index = frame->slot;
    if (index >= frames_.size() || &frames_[index] != frame || states_[index] != State::Processing) {
        return false;
    }
    states_[index] = State::Free;
    return true;
}

ThermalFramePool::State ThermalFramePool::state(std::size_t index) const {
    return index < states_.size() ? states_[index] : State::Free;
}

std::size_t ThermalFramePool::ready_count() const {
    std::size_t count = 0;
    for (State state : states_) {
        if (state == State::Ready) {
            ++count;
        }
    }
    return count;
}

std::uint8_t htu21d_crc8(const std::uint8_t* data, std::size_t size) {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U ? static_cast<std::uint8_t>((crc << 1U) ^ 0x31U)
                                      : static_cast<std::uint8_t>(crc << 1U);
        }
    }
    return crc;
}

bool htu21d_crc_valid(std::uint16_t raw_value, std::uint8_t received_crc) {
    const std::array<std::uint8_t, 2> bytes{static_cast<std::uint8_t>(raw_value >> 8U),
                                             static_cast<std::uint8_t>(raw_value & 0xffU)};
    return htu21d_crc8(bytes.data(), bytes.size()) == received_crc;
}

}  // namespace ratsense::drivers

