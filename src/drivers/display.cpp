#include "ratsense/drivers/display.hpp"

#include <algorithm>
#include <array>

namespace ratsense::drivers {
namespace {

std::array<std::uint8_t, 5> glyph(char c) {
    switch (c) {
        case 'A':
            return {0x7e, 0x11, 0x11, 0x7e, 0x00};
        case 'E':
            return {0x7f, 0x49, 0x49, 0x41, 0x00};
        case 'N':
            return {0x7f, 0x06, 0x18, 0x7f, 0x00};
        case 'O':
            return {0x3e, 0x41, 0x41, 0x3e, 0x00};
        case 'R':
            return {0x7f, 0x19, 0x29, 0x46, 0x00};
        case 'S':
            return {0x46, 0x49, 0x49, 0x31, 0x00};
        case 'T':
            return {0x01, 0x7f, 0x01, 0x01, 0x00};
        case '0':
            return {0x3e, 0x45, 0x49, 0x3e, 0x00};
        case '1':
            return {0x00, 0x42, 0x7f, 0x40, 0x00};
        case '2':
            return {0x62, 0x51, 0x49, 0x46, 0x00};
        case '3':
            return {0x22, 0x41, 0x49, 0x36, 0x00};
        case '4':
            return {0x18, 0x14, 0x12, 0x7f, 0x00};
        case '5':
            return {0x2f, 0x49, 0x49, 0x31, 0x00};
        case '6':
            return {0x3e, 0x49, 0x49, 0x32, 0x00};
        case '7':
            return {0x01, 0x71, 0x09, 0x07, 0x00};
        case '8':
            return {0x36, 0x49, 0x49, 0x36, 0x00};
        case '9':
            return {0x26, 0x49, 0x49, 0x3e, 0x00};
        case ':':
            return {0x00, 0x36, 0x36, 0x00, 0x00};
        case '-':
            return {0x08, 0x08, 0x08, 0x08, 0x00};
        default:
            return {0, 0, 0, 0, 0};
    }
}

}  // namespace

SharpP3502Frame::SharpP3502Frame() {
    clear(false);
}

void SharpP3502Frame::clear(bool on) {
    pixels_.fill(on ? 0xffU : 0x00U);
    dirty_.fill(true);
}

void SharpP3502Frame::set_pixel(std::size_t x, std::size_t y, bool on) {
    if (x >= kLcdWidth || y >= kLcdHeight) {
        return;
    }
    const std::size_t index = y * kLcdRowBytes + x / 8U;
    const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
    const std::uint8_t before = pixels_[index];
    if (on) {
        pixels_[index] = static_cast<std::uint8_t>(pixels_[index] | mask);
    } else {
        pixels_[index] = static_cast<std::uint8_t>(pixels_[index] & static_cast<std::uint8_t>(~mask));
    }
    if (before != pixels_[index]) {
        dirty_[y] = true;
    }
}

void SharpP3502Frame::fill_rect(std::size_t x, std::size_t y, std::size_t width, std::size_t height, bool on) {
    for (std::size_t yy = y; yy < std::min(kLcdHeight, y + height); ++yy) {
        for (std::size_t xx = x; xx < std::min(kLcdWidth, x + width); ++xx) {
            set_pixel(xx, yy, on);
        }
    }
}

void SharpP3502Frame::draw_text(std::size_t x, std::size_t y, const std::string& text) {
    std::size_t cursor = x;
    for (char c : text) {
        const auto columns = glyph(c);
        for (std::size_t col = 0; col < columns.size(); ++col) {
            for (std::size_t row = 0; row < 7U; ++row) {
                const bool on = (columns[col] & static_cast<std::uint8_t>(1U << row)) != 0U;
                set_pixel(cursor + col, y + row, on);
            }
        }
        cursor += 6U;
        if (cursor >= kLcdWidth) {
            break;
        }
    }
}

std::vector<std::uint8_t> SharpP3502Frame::dirty_rows() const {
    std::vector<std::uint8_t> rows;
    for (std::size_t i = 0; i < dirty_.size(); ++i) {
        if (dirty_[i]) {
            rows.push_back(static_cast<std::uint8_t>(i + 1U));
        }
    }
    return rows;
}

void SharpP3502Frame::mark_clean() {
    dirty_.fill(false);
}

std::vector<std::uint8_t> SharpP3502Frame::make_row_command(std::size_t row, bool vcom) const {
    if (row == 0U || row > kLcdHeight) {
        return {};
    }
    std::vector<std::uint8_t> command;
    command.reserve(2U + kLcdRowBytes + 1U);
    command.push_back(static_cast<std::uint8_t>(0x80U | (vcom ? 0x40U : 0x00U)));
    command.push_back(static_cast<std::uint8_t>(row));
    const std::size_t offset = (row - 1U) * kLcdRowBytes;
    command.insert(command.end(), pixels_.begin() + static_cast<std::ptrdiff_t>(offset),
                   pixels_.begin() + static_cast<std::ptrdiff_t>(offset + kLcdRowBytes));
    command.push_back(0U);
    return command;
}

std::vector<std::uint8_t> SharpP3502Frame::to_pbm() const {
    const std::string header = "P4\n168 144\n";
    std::vector<std::uint8_t> output(header.begin(), header.end());
    output.insert(output.end(), pixels_.begin(), pixels_.end());
    return output;
}

}  // namespace ratsense::drivers

