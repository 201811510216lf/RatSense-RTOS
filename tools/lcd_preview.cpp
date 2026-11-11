#include "ratsense/drivers/display.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    const std::filesystem::path path = argc > 1 ? argv[1] : "out/lcd_preview.pbm";
    std::filesystem::create_directories(path.parent_path());
    ratsense::drivers::SharpP3502Frame frame;
    frame.clear(false);
    frame.draw_text(4U, 4U, "RATSENSE");
    frame.draw_text(4U, 20U, "STATE:ARMED");
    frame.fill_rect(4U, 40U, 80U, 12U, true);
    const auto data = frame.to_pbm();
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    std::cout << "wrote " << path.string() << " bytes=" << data.size() << '\n';
    return output.good() ? 0 : 1;
}

