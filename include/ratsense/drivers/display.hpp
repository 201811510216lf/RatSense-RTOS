#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ratsense::drivers {

constexpr std::size_t kLcdWidth = 168U;
constexpr std::size_t kLcdHeight = 144U;
constexpr std::size_t kLcdRowBytes = kLcdWidth / 8U;

class SharpP3502Frame {
public:
    SharpP3502Frame();
    void clear(bool on = false);
    void set_pixel(std::size_t x, std::size_t y, bool on = true);
    void fill_rect(std::size_t x, std::size_t y, std::size_t width, std::size_t height, bool on = true);
    void draw_text(std::size_t x, std::size_t y, const std::string& text);
    const std::array<std::uint8_t, kLcdHeight * kLcdRowBytes>& pixels() const { return pixels_; }
    std::vector<std::uint8_t> dirty_rows() const;
    void mark_clean();
    std::vector<std::uint8_t> make_row_command(std::size_t row, bool vcom) const;
    std::vector<std::uint8_t> to_pbm() const;
    bool vcom() const { return vcom_; }
    void toggle_vcom() { vcom_ = !vcom_; }

private:
    std::array<std::uint8_t, kLcdHeight * kLcdRowBytes> pixels_{};
    std::array<bool, kLcdHeight> dirty_{};
    bool vcom_{false};
};

}  // namespace ratsense::drivers

