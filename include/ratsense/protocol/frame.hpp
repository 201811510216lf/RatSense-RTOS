#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ratsense::protocol {

constexpr std::uint16_t kMagic = 0x5253U;
constexpr std::uint8_t kVersion = 1U;
constexpr std::size_t kHeaderSize = 24U;
constexpr std::size_t kCrcSize = 4U;
constexpr std::size_t kMaxPayload = 512U;
constexpr std::size_t kMaxEncodedFrame = 1024U;

enum class MessageType : std::uint8_t {
    Hello = 1,
    HelloAck = 2,
    Heartbeat = 3,
    Status = 4,
    ThermalSample = 5,
    PressEvent = 6,
    FeedAttempt = 7,
    PelletDetected = 8,
    ErrorEvent = 9,
    ConfigGet = 10,
    ConfigSet = 11,
    ConfigAck = 12,
    ConfigNack = 13,
    StartSession = 14,
    PauseSession = 15,
    ResumeSession = 16,
    StopSession = 17,
    TimeSync = 18,
    LogQuery = 19,
    LogRecord = 20,
    LogAck = 21,
    FwUpdateRequest = 22,
    FwUpdateStatus = 23,
};

enum FrameFlags : std::uint8_t {
    AckRequested = 1U << 0U,
    Response = 1U << 1U,
    Error = 1U << 2U,
    Retried = 1U << 3U,
};

struct Frame {
    std::uint8_t version{kVersion};
    MessageType message_type{MessageType::Heartbeat};
    std::uint8_t flags{0};
    std::uint32_t sequence{0};
    std::uint32_t ack_sequence{0};
    std::uint64_t device_time_ms{0};
    std::vector<std::uint8_t> payload;
};

enum class CodecError {
    None,
    Empty,
    CobsMalformed,
    TooLong,
    HeaderTooShort,
    BadMagic,
    UnsupportedVersion,
    PayloadTooLarge,
    LengthMismatch,
    CrcMismatch,
};

struct DecodeResult {
    CodecError error{CodecError::None};
    Frame frame{};
    explicit operator bool() const { return error == CodecError::None; }
};

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size);
std::uint32_t crc32_ieee(const std::vector<std::uint8_t>& data);

std::vector<std::uint8_t> cobs_encode(const std::vector<std::uint8_t>& input);
bool cobs_decode(const std::vector<std::uint8_t>& encoded, std::vector<std::uint8_t>& output);

std::vector<std::uint8_t> encode_frame(const Frame& frame, CodecError* error = nullptr);
DecodeResult decode_frame(const std::vector<std::uint8_t>& encoded_without_delimiter);

class StreamParser {
public:
    explicit StreamParser(std::size_t max_encoded = kMaxEncodedFrame);

    // Feed arbitrary UART chunks. A callback is invoked for each valid frame.
    template <typename Callback>
    void feed(const std::uint8_t* bytes, std::size_t size, Callback&& callback) {
        for (std::size_t i = 0; i < size; ++i) {
            const std::uint8_t byte = bytes[i];
            if (byte == 0U) {
                if (!encoded_.empty()) {
                    const DecodeResult result = decode_frame(encoded_);
                    if (result) {
                        callback(result.frame);
                        ++frames_ok_;
                    } else {
                        ++frames_bad_;
                        last_error_ = result.error;
                    }
                    encoded_.clear();
                }
                continue;
            }
            if (encoded_.size() >= max_encoded_) {
                encoded_.clear();
                ++frames_bad_;
                last_error_ = CodecError::TooLong;
                continue;
            }
            encoded_.push_back(byte);
        }
    }

    std::size_t frames_ok() const { return frames_ok_; }
    std::size_t frames_bad() const { return frames_bad_; }
    CodecError last_error() const { return last_error_; }
    void reset();

private:
    std::vector<std::uint8_t> encoded_;
    std::size_t max_encoded_;
    std::size_t frames_ok_{0};
    std::size_t frames_bad_{0};
    CodecError last_error_{CodecError::None};
};

const char* codec_error_name(CodecError error);
const char* message_type_name(MessageType type);

}  // namespace ratsense::protocol

