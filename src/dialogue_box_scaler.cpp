#include "dialogue_box_scaler.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace khcom {

DialogueBoxScaler& DialogueBoxScaler::instance() {
    static DialogueBoxScaler s_instance;
    return s_instance;
}

DialogueBoxScaler::DialogueBoxScaler() = default;

void DialogueBoxScaler::set_width_mode(BoxWidthMode mode) {
    settings_.width_mode = mode;
    switch (mode) {
        case BoxWidthMode::Standard240:
            settings_.extra_horizontal_tiles = 0;
            break;
        case BoxWidthMode::WidescreenExpanded:
            settings_.extra_horizontal_tiles = 6; // 28 + 6 = 34 tiles (272px)
            break;
        case BoxWidthMode::DynamicResponsive:
            settings_.extra_horizontal_tiles = 8; // 28 + 8 = 36 tiles (288px)
            break;
    }
}

void DialogueBoxScaler::set_extra_tiles(int extra_tiles) {
    settings_.extra_horizontal_tiles = std::clamp(extra_tiles, 0, 12);
}

void DialogueBoxScaler::set_max_lines(int lines) {
    settings_.max_visible_lines = std::clamp(lines, 3, 6);
}

int DialogueBoxScaler::get_box_width_tiles() const {
    return 28 + settings_.extra_horizontal_tiles;
}

int DialogueBoxScaler::get_box_width_pixels() const {
    return get_box_width_tiles() * 8;
}

void DialogueBoxScaler::expand_dialogue_tilemap_row(uint16_t* tilemap_row, int original_tile_count, int target_tile_count) {
    if (!tilemap_row || target_tile_count <= original_tile_count || original_tile_count < 3) {
        return;
    }

    // A nine-slice tilemap row consists of:
    // Left border tile (index 0)
    // Center repeating tiles (index 1 to original_tile_count - 2)
    // Right border tile (index original_tile_count - 1)
    uint16_t left_cap = tilemap_row[0];
    uint16_t center_tile = tilemap_row[1];
    uint16_t right_cap = tilemap_row[original_tile_count - 1];

    tilemap_row[0] = left_cap;
    for (int i = 1; i < target_tile_count - 1; ++i) {
        tilemap_row[i] = center_tile;
    }
    tilemap_row[target_tile_count - 1] = right_cap;
}

void DialogueBoxScaler::adjust_dialogue_oam(uint16_t& attr0, uint16_t& attr1, int current_view_width) {
    if (settings_.width_mode == BoxWidthMode::Standard240 || !settings_.adjust_border_sprites) {
        return;
    }

    int y = attr0 & 0x00FF;
    int x = attr1 & 0x01FF;

    // GBA OAM signed coordinates
    if (x >= 256) x -= 512;
    if (y >= 160) y -= 256;

    // Dialogue window is located in lower third: y >= 110 and y <= 155
    if (y >= 110 && y <= 155) {
        int extra_shift = (settings_.extra_horizontal_tiles * 8) / 2;

        // Left border corner
        if (x <= 40) {
            x = std::max(-32, x - extra_shift);
            attr1 = (attr1 & ~0x01FF) | (static_cast<uint16_t>(x) & 0x01FF);
        }
        // Right border corner / advance arrow icon
        else if (x >= 180) {
            x = std::min(current_view_width + 16, x + extra_shift);
            attr1 = (attr1 & ~0x01FF) | (static_cast<uint16_t>(x) & 0x01FF);
        }
    }
}

} // namespace khcom
