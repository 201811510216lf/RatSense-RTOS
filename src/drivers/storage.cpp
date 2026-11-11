#include "ratsense/drivers/storage.hpp"

#include <array>
#include <fstream>
#include <utility>

namespace ratsense::drivers {
namespace {

constexpr std::uint16_t kRecordMagic = 0x4c52U;
constexpr std::uint8_t kRecordVersion = 1U;
constexpr std::size_t kFixedHeaderSize = 34U;

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffU));
    out.push_back(static_cast<std::uint8_t>(value >> 8U));
}
void put_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
}
void put_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
}
std::uint16_t get_u16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | static_cast<std::uint16_t>(p[1] << 8U));
}
std::uint32_t get_u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U) |
           (static_cast<std::uint32_t>(p[2]) << 16U) | (static_cast<std::uint32_t>(p[3]) << 24U);
}
std::uint64_t get_u64(const std::uint8_t* p) {
    std::uint64_t value = 0;
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        value |= static_cast<std::uint64_t>(p[shift / 8U]) << shift;
    }
    return value;
}

}  // namespace

LogStore::LogStore(std::filesystem::path path) : path_(std::move(path)) {
    const std::vector<EventRecord> records = recover();
    if (!records.empty()) {
        next_sequence_ = records.back().sequence + 1U;
    }
}

std::vector<std::uint8_t> LogStore::serialize(const EventRecord& record) {
    std::vector<std::uint8_t> out;
    out.reserve(kFixedHeaderSize + record.payload.size() + 4U);
    put_u16(out, kRecordMagic);
    out.push_back(kRecordVersion);
    out.push_back(static_cast<std::uint8_t>(record.type));
    put_u32(out, record.sequence);
    put_u64(out, record.session_id);
    put_u64(out, record.monotonic_ms);
    put_u64(out, record.rtc_ms);
    put_u16(out, static_cast<std::uint16_t>(record.payload.size()));
    out.insert(out.end(), record.payload.begin(), record.payload.end());
    const std::uint32_t crc = protocol::crc32_ieee(out);
    put_u32(out, crc);
    return out;
}

std::optional<EventRecord> LogStore::deserialize(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < kFixedHeaderSize + 4U || get_u16(bytes.data()) != kRecordMagic || bytes[2] != kRecordVersion) {
        return std::nullopt;
    }
    const std::size_t payload_size = get_u16(bytes.data() + 32U);
    if (bytes.size() != kFixedHeaderSize + payload_size + 4U) {
        return std::nullopt;
    }
    const std::uint32_t expected = get_u32(bytes.data() + bytes.size() - 4U);
    const std::uint32_t actual = protocol::crc32_ieee(bytes.data(), bytes.size() - 4U);
    if (expected != actual) {
        return std::nullopt;
    }
    EventRecord record{};
    record.type = static_cast<RecordType>(bytes[3]);
    record.sequence = get_u32(bytes.data() + 4U);
    record.session_id = get_u64(bytes.data() + 8U);
    record.monotonic_ms = get_u64(bytes.data() + 16U);
    record.rtc_ms = get_u64(bytes.data() + 24U);
    record.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kFixedHeaderSize),
                          bytes.begin() + static_cast<std::ptrdiff_t>(kFixedHeaderSize + payload_size));
    return record;
}

bool LogStore::append(const EventRecord& input) {
    if (fail_after_.has_value() && recover().size() >= *fail_after_) {
        healthy_ = false;
        return false;
    }
    EventRecord record = input;
    if (record.sequence == 0U) {
        record.sequence = next_sequence_;
    }
    const std::vector<std::uint8_t> bytes = serialize(record);
    std::ofstream stream(path_, std::ios::binary | std::ios::app);
    if (!stream) {
        healthy_ = false;
        return false;
    }
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.flush();
    if (!stream.good()) {
        healthy_ = false;
        return false;
    }
    next_sequence_ = record.sequence + 1U;
    healthy_ = true;
    return true;
}

std::vector<EventRecord> LogStore::recover() const {
    std::vector<EventRecord> records;
    std::ifstream stream(path_, std::ios::binary);
    if (!stream) {
        return records;
    }
    while (true) {
        std::array<std::uint8_t, kFixedHeaderSize> header{};
        stream.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
        const std::streamsize got = stream.gcount();
        if (got == 0) {
            break;
        }
        if (got != static_cast<std::streamsize>(header.size())) {
            break;
        }
        const std::size_t payload_size = get_u16(header.data() + 32U);
        std::vector<std::uint8_t> bytes(header.begin(), header.end());
        std::vector<std::uint8_t> tail(payload_size + 4U);
        stream.read(reinterpret_cast<char*>(tail.data()), static_cast<std::streamsize>(tail.size()));
        if (stream.gcount() != static_cast<std::streamsize>(tail.size())) {
            break;
        }
        bytes.insert(bytes.end(), tail.begin(), tail.end());
        const std::optional<EventRecord> record = deserialize(bytes);
        if (!record.has_value()) {
            break;
        }
        records.push_back(*record);
    }
    return records;
}

bool LogStore::replay_from(std::uint32_t sequence, std::vector<EventRecord>& output) const {
    output.clear();
    for (const EventRecord& record : recover()) {
        if (record.sequence >= sequence) {
            output.push_back(record);
        }
    }
    return true;
}

void LogStore::set_fail_after_records(std::optional<std::size_t> count) {
    fail_after_ = count;
    healthy_ = true;
}

std::uint32_t record_crc(const EventRecord& record) {
    const std::vector<std::uint8_t> bytes = LogStore::serialize(record);
    return protocol::crc32_ieee(bytes.data(), bytes.size() - 4U);
}

}  // namespace ratsense::drivers

