#pragma once

#include "ratsense/protocol/frame.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ratsense::drivers {

enum class RecordType : std::uint8_t { Sample = 1, Press = 2, FeedAttempt = 3, Pellet = 4, Error = 5, Session = 6 };

struct EventRecord {
    RecordType type{RecordType::Sample};
    std::uint32_t sequence{0};
    std::uint64_t session_id{0};
    std::uint64_t monotonic_ms{0};
    std::uint64_t rtc_ms{0};
    std::vector<std::uint8_t> payload;
};

class LogStore {
public:
    explicit LogStore(std::filesystem::path path);
    bool append(const EventRecord& record);
    std::vector<EventRecord> recover() const;
    bool replay_from(std::uint32_t sequence, std::vector<EventRecord>& output) const;
    void set_fail_after_records(std::optional<std::size_t> count);
    bool healthy() const { return healthy_; }
    std::uint32_t next_sequence() const { return next_sequence_; }
    static std::vector<std::uint8_t> serialize(const EventRecord& record);

private:
    std::filesystem::path path_;
    std::optional<std::size_t> fail_after_;
    mutable bool healthy_{true};
    std::uint32_t next_sequence_{1};
    static std::optional<EventRecord> deserialize(const std::vector<std::uint8_t>& bytes);
};

std::uint32_t record_crc(const EventRecord& record);

}  // namespace ratsense::drivers

