#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace khcom {

enum class FontScale : int {
    Original100 = 0, // 8x12 glyphs, 16px line spacing (original GBA)
    Medium85    = 1, // 7x10 glyphs, 13px line spacing
    Compact70   = 2, // 6x8 glyphs, 10px line spacing
    Micro55     = 3  // 5x7 glyphs, 8px line spacing
};

struct FontResizerSettings {
    FontScale scale = FontScale::Original100;
    int line_spacing = 16;       // Vertical line pitch in pixels (8 to 20)
    int kerning_adjustment = 0;   // Kerning offset in pixels (-2 to +2)
    bool edge_smoothing = true;   // Anti-aliased font downsampling
};

class FontResizer {
public:
    static FontResizer& instance();

    const FontResizerSettings& settings() const { return settings_; }
    FontResizerSettings& settings() { return settings_; }

    void set_font_scale(FontScale scale);
    void set_line_spacing(int spacing_px);
    void set_kerning_adjustment(int kerning_px);
    void set_edge_smoothing(bool enable);

    // Returns effective glyph bounding box width and height in pixels
    int get_glyph_width() const;
    int get_glyph_height() const;

    // Calculates proportional advance width for a given ASCII character
    int get_char_advance_width(char c) const;

    // Downsamples an 8x12 1bpp glyph into an 8x8 4bpp GBA VRAM tile
    // 4bpp GBA tile: 32 bytes (2 pixels per byte, 8 rows of 4 bytes)
    void render_scaled_glyph_4bpp(const uint8_t* src_glyph_1bpp, uint8_t* dst_tile_4bpp, uint8_t fg_palette_idx = 1);

    // Hook to intercept and resize text tile uploads into GBA VRAM BG Character Blocks
    bool intercept_text_tile_upload(uint32_t vram_dest_addr, const uint8_t* tile_data, size_t size, uint8_t* vram_base);

private:
    FontResizer();

    void update_metrics_for_scale(FontScale scale);

    FontResizerSettings settings_;
    int glyph_w_ = 8;
    int glyph_h_ = 12;
    uint8_t advance_table_[128];
};

} // namespace khcom
