#include "ratsense/protocol/frame.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
bool parse_hex(const std::string& text, std::vector<std::uint8_t>& bytes) {
    std::istringstream input(text);
    std::string token;
    while (input >> token) {
        unsigned value = 0;
        std::stringstream parser;
        parser << std::hex << token;
        parser >> value;
        if (parser.fail() || value > 255U) {
            return false;
        }
        bytes.push_back(static_cast<std::uint8_t>(value));
    }
    return !bytes.empty();
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: uart_frame_tool \"12 34 ...\" (encoded COBS bytes without 00 delimiter)\n";
        return 0;
    }
    std::vector<std::uint8_t> bytes;
    if (!parse_hex(argv[1], bytes)) {
        std::cerr << "invalid hex input\n";
        return 2;
    }
    const auto result = ratsense::protocol::decode_frame(bytes);
    if (!result) {
        std::cout << "error=" << ratsense::protocol::codec_error_name(result.error) << '\n';
        return 1;
    }
    std::cout << "type=" << ratsense::protocol::message_type_name(result.frame.message_type)
              << " sequence=" << result.frame.sequence << " ack=" << result.frame.ack_sequence
              << " payload_len=" << result.frame.payload.size() << '\n';
    for (const auto byte : result.frame.payload) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte) << ' ';
    }
    std::cout << '\n';
    return 0;
}

