#include "ratsense/drivers/storage.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    const std::filesystem::path path = argc > 1 ? argv[1] : "out/sim/normal.log";
    ratsense::drivers::LogStore store(path);
    const auto records = store.recover();
    std::cout << "path=" << path.string() << " records=" << records.size() << '\n';
    for (const auto& record : records) {
        std::cout << "seq=" << record.sequence << " type=" << static_cast<unsigned>(record.type)
                  << " session=" << record.session_id << " monotonic_ms=" << record.monotonic_ms
                  << " payload_len=" << record.payload.size() << '\n';
    }
    return 0;
}

