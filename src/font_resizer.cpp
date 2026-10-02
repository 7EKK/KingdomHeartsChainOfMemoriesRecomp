#include "font_resizer.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace khcom {

namespace {

// Standard GBA KH:CoM proportional width table for ASCII 32 (' ') to 126 ('~')
const uint8_t kBaseAsciiWidths[95] = {
    4, // ' '
    3, 4, 7, 6, 7, 7, 3, 4, 4, 5, 6, 3, 5, 3, 5, // '!' through '/'
    6, 5, 6, 6, 6, 6, 6, 6, 6, 6,                 // '0' through '9'
    3, 3, 5, 6, 5, 5, 7,                          // ':' through '@'
    7, 6, 6, 6, 6, 6, 7, 6, 4, 6, 6, 6, 8, 7, 7, // 'A' through 'O'
    6, 7, 6, 6, 6, 6, 7, 8, 7, 6, 6,              // 'P' through 'Z'
    4, 5, 4, 6, 6, 4,                             // '[' through '`'
    6, 6, 5, 6, 5, 5, 6, 6, 3, 4, 5, 3, 8, 6, 6, // 'a' through 'o'
    6, 6, 5, 5, 5, 6, 6, 8, 6, 6, 5,              // 'p' through 'z'
    4, 3, 4, 7                                    // '{' through '~'
};

} // namespace

FontResizer& FontResizer::instance() {
    static FontResizer s_instance;
    return s_instance;
}

FontResizer::FontResizer() {
    update_metrics_for_scale(settings_.scale);
}

void FontResizer::set_font_scale(FontScale scale) {
    settings_.scale = scale;
    update_metrics_for_scale(scale);
}

void FontResizer::set_line_spacing(int spacing_px) {
    settings_.line_spacing = std::clamp(spacing_px, 8, 24);
}

void FontResizer::set_kerning_adjustment(int kerning_px) {
    settings_.kerning_adjustment = std::clamp(kerning_px, -2, 2);
    update_metrics_for_scale(settings_.scale);
}

void FontResizer::set_edge_smoothing(bool enable) {
    settings_.edge_smoothing = enable;
}

int FontResizer::get_glyph_width() const {
    return glyph_w_;
}

int FontResizer::get_glyph_height() const {
    return glyph_h_;
}

int FontResizer::get_char_advance_width(char c) const {
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 128) {
        return advance_table_[uc];
    }
    return glyph_w_;
}

void FontResizer::update_metrics_for_scale(FontScale scale) {
    float scale_factor = 1.0f;
    switch (scale) {
        case FontScale::Medium85:
            glyph_w_ = 7;
            glyph_h_ = 10;
            settings_.line_spacing = 13;
            scale_factor = 0.85f;
            break;
        case FontScale::Compact70:
            glyph_w_ = 6;
            glyph_h_ = 8;
            settings_.line_spacing = 10;
            scale_factor = 0.70f;
            break;
        case FontScale::Micro55:
            glyph_w_ = 5;
            glyph_h_ = 7;
            settings_.line_spacing = 8;
            scale_factor = 0.55f;
            break;
        case FontScale::Original100:
        default:
            glyph_w_ = 8;
            glyph_h_ = 12;
            settings_.line_spacing = 16;
            scale_factor = 1.0f;
            break;
    }

    // Populate proportional advance widths
    for (int i = 0; i < 128; ++i) {
        if (i >= 32 && (i - 32) < 95) {
            int base_w = kBaseAsciiWidths[i - 32];
            int scaled_w = static_cast<int>(std::round(base_w * scale_factor)) + settings_.kerning_adjustment;
            advance_table_[i] = static_cast<uint8_t>(std::clamp(scaled_w, 2, glyph_w_));
        } else {
            advance_table_[i] = static_cast<uint8_t>(glyph_w_);
        }
    }
}

void FontResizer::render_scaled_glyph_4bpp(const uint8_t* src_glyph_1bpp, uint8_t* dst_tile_4bpp, uint8_t fg_palette_idx) {
    if (!src_glyph_1bpp || !dst_tile_4bpp) return;

    // Clear destination 8x8 4bpp tile (32 bytes)
    std::memset(dst_tile_4bpp, 0, 32);

    const int target_w = glyph_w_;
    const int target_h = glyph_h_;

    // Sample from original 8x12 1bpp grid to target_w x target_h
    // and center within the 8x8 tile
    int offset_x = (8 - target_w) / 2;
    int offset_y = (8 - std::min(8, target_h)) / 2;

    for (int ty = 0; ty < std::min(8, target_h); ++ty) {
        int dst_y = offset_y + ty;
        if (dst_y < 0 || dst_y >= 8) continue;

        // Map target y to source y in 8x12 grid
        int sy = (ty * 12) / target_h;
        sy = std::clamp(sy, 0, 11);
        uint8_t src_row_byte = src_glyph_1bpp[sy];

        for (int tx = 0; tx < target_w; ++tx) {
            int dst_x = offset_x + tx;
            if (dst_x < 0 || dst_x >= 8) continue;

            int sx = (tx * 8) / target_w;
            sx = std::clamp(sx, 0, 7);

            bool pixel_on = (src_row_byte & (0x80 >> sx)) != 0;
            if (pixel_on) {
                // In GBA 4bpp tile:
                // byte_index = (dst_y * 4) + (dst_x / 2)
                // pixel 0 is low nibble, pixel 1 is high nibble
                int byte_idx = (dst_y * 4) + (dst_x / 2);
                if (dst_x % 2 == 0) {
                    dst_tile_4bpp[byte_idx] = (dst_tile_4bpp[byte_idx] & 0xF0) | (fg_palette_idx & 0x0F);
                } else {
                    dst_tile_4bpp[byte_idx] = (dst_tile_4bpp[byte_idx] & 0x0F) | ((fg_palette_idx & 0x0F) << 4);
                }
            }
        }
    }
}

bool FontResizer::intercept_text_tile_upload(uint32_t vram_dest_addr, const uint8_t* tile_data, size_t size, uint8_t* vram_base) {
    if (settings_.scale == FontScale::Original100 || !tile_data || !vram_base || size < 32) {
        return false;
    }

    // Check if destination is within GBA BG Character Base Blocks (VRAM 0x06000000 - 0x0600FFFF)
    if (vram_dest_addr < 0x06000000 || vram_dest_addr >= 0x06010000) {
        return false;
    }

    uint32_t offset = vram_dest_addr - 0x06000000;
    
    // Scale glyph into temporary 4bpp tile buffer
    uint8_t scaled_tile[32];
    // In GBA font VRAM upload, tile_data contains 1bpp or 2bpp glyph rows
    render_scaled_glyph_4bpp(tile_data, scaled_tile);

    std::memcpy(vram_base + offset, scaled_tile, 32);
    return true;
}

} // namespace khcom
