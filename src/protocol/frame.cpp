#include "ratsense/protocol/frame.hpp"

#include <algorithm>
#include <array>

namespace ratsense::protocol {
namespace {

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
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
    return static_cast<std::uint16_t>(p[0]) | static_cast<std::uint16_t>(p[1] << 8U);
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

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            const std::uint32_t mask = static_cast<std::uint32_t>(-(static_cast<std::int32_t>(crc & 1U)));
            crc = (crc >> 1U) ^ (0xedb88320U & mask);
        }
    }
    return ~crc;
}

std::uint32_t crc32_ieee(const std::vector<std::uint8_t>& data) {
    return crc32_ieee(data.data(), data.size());
}

std::vector<std::uint8_t> cobs_encode(const std::vector<std::uint8_t>& input) {
    std::vector<std::uint8_t> output;
    output.reserve(input.size() + 2U);
    std::size_t code_index = 0;
    output.push_back(0);
    std::uint8_t code = 1U;
    for (const std::uint8_t byte : input) {
        if (byte == 0U) {
            output[code_index] = code;
            code_index = output.size();
            output.push_back(0U);
            code = 1U;
        } else {
            output.push_back(byte);
            ++code;
            if (code == 0xffU) {
                output[code_index] = code;
                code_index = output.size();
                output.push_back(0U);
                code = 1U;
            }
        }
    }
    output[code_index] = code;
    return output;
}

bool cobs_decode(const std::vector<std::uint8_t>& encoded, std::vector<std::uint8_t>& output) {
    output.clear();
    if (encoded.empty()) {
        return false;
    }
    std::size_t index = 0;
    while (index < encoded.size()) {
        const std::uint8_t code = encoded[index];
        if (code == 0U) {
            return false;
        }
        ++index;
        const std::size_t end = index + static_cast<std::size_t>(code) - 1U;
        if (end > encoded.size()) {
            return false;
        }
        output.insert(output.end(), encoded.begin() + static_cast<std::ptrdiff_t>(index),
                      encoded.begin() + static_cast<std::ptrdiff_t>(end));
        index = end;
        if (code != 0xffU && index < encoded.size()) {
            output.push_back(0U);
        }
    }
    return true;
}

std::vector<std::uint8_t> encode_frame(const Frame& frame, CodecError* error) {
    if (error != nullptr) {
        *error = CodecError::None;
    }
    if (frame.payload.size() > kMaxPayload) {
        if (error != nullptr) {
            *error = CodecError::PayloadTooLarge;
        }
        return {};
    }
    std::vector<std::uint8_t> raw;
    raw.reserve(kHeaderSize + frame.payload.size() + kCrcSize);
    put_u16(raw, kMagic);
    raw.push_back(frame.version);
    raw.push_back(static_cast<std::uint8_t>(frame.message_type));
    raw.push_back(frame.flags);
    raw.push_back(static_cast<std::uint8_t>(kHeaderSize));
    put_u32(raw, frame.sequence);
    put_u32(raw, frame.ack_sequence);
    put_u16(raw, static_cast<std::uint16_t>(frame.payload.size()));
    put_u64(raw, frame.device_time_ms);
    raw.insert(raw.end(), frame.payload.begin(), frame.payload.end());
    put_u32(raw, crc32_ieee(raw));
    return cobs_encode(raw);
}

DecodeResult decode_frame(const std::vector<std::uint8_t>& encoded_without_delimiter) {
    DecodeResult result;
    if (encoded_without_delimiter.empty()) {
        result.error = CodecError::Empty;
        return result;
    }
    std::vector<std::uint8_t> raw;
    if (!cobs_decode(encoded_without_delimiter, raw)) {
        result.error = CodecError::CobsMalformed;
        return result;
    }
    if (raw.size() < kHeaderSize + kCrcSize) {
        result.error = CodecError::HeaderTooShort;
        return result;
    }
    if (get_u16(raw.data()) != kMagic) {
        result.error = CodecError::BadMagic;
        return result;
    }
    if (raw[2] != kVersion) {
        result.error = CodecError::UnsupportedVersion;
        return result;
    }
    const std::size_t header_len = raw[5];
    if (header_len < kHeaderSize || header_len > raw.size() - kCrcSize) {
        result.error = CodecError::HeaderTooShort;
        return result;
    }
    const std::size_t payload_len = get_u16(raw.data() + 14U);
    if (payload_len > kMaxPayload) {
        result.error = CodecError::PayloadTooLarge;
        return result;
    }
    if (raw.size() != header_len + payload_len + kCrcSize) {
        result.error = CodecError::LengthMismatch;
        return result;
    }
    const std::uint32_t expected_crc = get_u32(raw.data() + raw.size() - kCrcSize);
    const std::uint32_t actual_crc = crc32_ieee(raw.data(), raw.size() - kCrcSize);
    if (expected_crc != actual_crc) {
        result.error = CodecError::CrcMismatch;
        return result;
    }
    result.frame.version = raw[2];
    result.frame.message_type = static_cast<MessageType>(raw[3]);
    result.frame.flags = raw[4];
    result.frame.sequence = get_u32(raw.data() + 6U);
    result.frame.ack_sequence = get_u32(raw.data() + 10U);
    result.frame.device_time_ms = get_u64(raw.data() + 16U);
    result.frame.payload.assign(raw.begin() + static_cast<std::ptrdiff_t>(header_len),
                                raw.begin() + static_cast<std::ptrdiff_t>(header_len + payload_len));
    return result;
}

StreamParser::StreamParser(std::size_t max_encoded) : max_encoded_(std::max<std::size_t>(32U, max_encoded)) {
    encoded_.reserve(max_encoded_);
}

void StreamParser::reset() {
    encoded_.clear();
    frames_ok_ = 0;
    frames_bad_ = 0;
    last_error_ = CodecError::None;
}

const char* codec_error_name(CodecError error) {
    switch (error) {
        case CodecError::None:
            return "none";
        case CodecError::Empty:
            return "empty";
        case CodecError::CobsMalformed:
            return "cobs_malformed";
        case CodecError::TooLong:
            return "too_long";
        case CodecError::HeaderTooShort:
            return "header_too_short";
        case CodecError::BadMagic:
            return "bad_magic";
        case CodecError::UnsupportedVersion:
            return "unsupported_version";
        case CodecError::PayloadTooLarge:
            return "payload_too_large";
        case CodecError::LengthMismatch:
            return "length_mismatch";
        case CodecError::CrcMismatch:
            return "crc_mismatch";
    }
    return "unknown";
}

const char* message_type_name(MessageType type) {
    switch (type) {
        case MessageType::Hello:
            return "HELLO";
        case MessageType::HelloAck:
            return "HELLO_ACK";
        case MessageType::Heartbeat:
            return "HEARTBEAT";
        case MessageType::Status:
            return "STATUS";
        case MessageType::ThermalSample:
            return "THERMAL_SAMPLE";
        case MessageType::PressEvent:
            return "PRESS_EVENT";
        case MessageType::FeedAttempt:
            return "FEED_ATTEMPT";
        case MessageType::PelletDetected:
            return "PELLET_DETECTED";
        case MessageType::ErrorEvent:
            return "ERROR_EVENT";
        case MessageType::ConfigGet:
            return "CONFIG_GET";
        case MessageType::ConfigSet:
            return "CONFIG_SET";
        case MessageType::ConfigAck:
            return "CONFIG_ACK";
        case MessageType::ConfigNack:
            return "CONFIG_NACK";
        case MessageType::StartSession:
            return "START_SESSION";
        case MessageType::PauseSession:
            return "PAUSE_SESSION";
        case MessageType::ResumeSession:
            return "RESUME_SESSION";
        case MessageType::StopSession:
            return "STOP_SESSION";
        case MessageType::TimeSync:
            return "TIME_SYNC";
        case MessageType::LogQuery:
            return "LOG_QUERY";
        case MessageType::LogRecord:
            return "LOG_RECORD";
        case MessageType::LogAck:
            return "LOG_ACK";
        case MessageType::FwUpdateRequest:
            return "FW_UPDATE_REQUEST";
        case MessageType::FwUpdateStatus:
            return "FW_UPDATE_STATUS";
    }
    return "UNKNOWN";
}

}  // namespace ratsense::protocol

