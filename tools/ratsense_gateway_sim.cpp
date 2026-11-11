#include "ratsense/protocol/frame.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    const std::string device_id = argc > 1 ? argv[1] : "host-sim-001";
    ratsense::protocol::Frame frame{};
    frame.message_type = ratsense::protocol::MessageType::Status;
    frame.flags = ratsense::protocol::AckRequested;
    frame.sequence = 1U;
    frame.device_time_ms = 1000U;
    frame.payload = {1U, 0U, 0U, 0U};  // online=true, fault=0
    const std::vector<std::uint8_t> encoded = ratsense::protocol::encode_frame(frame);
    std::cout << "device_id=" << device_id << " topic=ratsense/v1/" << device_id
              << "/telemetry/status bytes=" << encoded.size() + 1U << " mqtt_qos=1\n";
    return encoded.empty() ? 1 : 0;
}

