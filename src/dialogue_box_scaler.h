#pragma once

#include <cstdint>
#include <cstddef>

namespace khcom {

enum class BoxWidthMode : int {
    Standard240        = 0, // Original 28-tile dialogue box (224px wide)
    WidescreenExpanded = 1, // 34-tile dialogue box (272px wide, matches 16:9 widescreen)
    DynamicResponsive  = 2  // Adapts automatically to current viewport aspect ratio
};

struct DialogueBoxSettings {
    BoxWidthMode width_mode = BoxWidthMode::WidescreenExpanded;
    int extra_horizontal_tiles = 6; // Extra center tiles added (0 to 12)
    int max_visible_lines = 4;      // Max dialogue lines visible (3 to 5)
    bool adjust_border_sprites = true;
};

class DialogueBoxScaler {
public:
    static DialogueBoxScaler& instance();

    const DialogueBoxSettings& settings() const { return settings_; }
    DialogueBoxSettings& settings() { return settings_; }

    void set_width_mode(BoxWidthMode mode);
    void set_extra_tiles(int extra_tiles);
    void set_max_lines(int lines);

    // Returns effective box width in tiles (standard is 28 tiles = 224px)
    int get_box_width_tiles() const;
    int get_box_width_pixels() const;

    // Nine-slice tilemap generator:
    // Transforms a standard 32x32 GBA background tilemap row to expand the dialogue window
    void expand_dialogue_tilemap_row(uint16_t* tilemap_row, int original_tile_count, int target_tile_count);

    // Repositions OAM border and portrait sprites for the expanded dialogue box
    void adjust_dialogue_oam(uint16_t& attr0, uint16_t& attr1, int current_view_width);

private:
    DialogueBoxScaler();

    DialogueBoxSettings settings_;
};

} // namespace khcom
