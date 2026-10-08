#pragma once

#include "font_resizer.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace khcom {

enum class LineCapacityMode : int {
    Authentic3Lines = 0, // Authentic 3 lines (standard GBA)
    Dense2Lines     = 1, // Prioritize 2 dense lines (fewer boxes)
    Expanded4Lines  = 2  // Expanded 4 lines (widescreen capacity)
};

struct DialogueEnhancerSettings {
    FontDensity density = FontDensity::Original;
    FontScale scale = FontScale::Original100;
    FontStyle style = FontStyle::Authentic;
    LineCapacityMode line_capacity = LineCapacityMode::Authentic3Lines;
    bool soft_word_wrap = true;
    bool case_aware_continuation = true;
    int max_line_width_chars = 26; // Dynamically computed from box width & font size
    int max_line_width_px = 152;   // Dynamically computed from dialogue box horizontal tiles
};

class DialogueEnhancer {
public:
    static DialogueEnhancer& instance();

    const DialogueEnhancerSettings& settings() const { return settings_; }
    DialogueEnhancerSettings& settings() { return settings_; }

    void recompute_line_width();

    void set_density(FontDensity density);
    void set_font_scale(FontScale scale);
    void set_font_style(FontStyle style);
    void set_line_capacity(LineCapacityMode mode);
    void set_soft_word_wrap(bool enable);
    void set_case_aware_continuation(bool enable);

    void invalidate_cache();

    // Intercepts bus reads for cutscene dialogue text from ROM and dynamically
    // supplies rewrapped character words, eliminating premature GBA line breaks
    bool intercept_bus_read(uint32_t addr, uint32_t width, uint32_t* out_val);

    // Calculates glyph advance width in pixels according to density setting
    int get_glyph_advance_width(int base_width = 8) const;

    // Evaluates whether a newline token should be merged into a space (soft wrap)
    // based on grammar, punctuation, and casing
    bool should_merge_newline(char prev_char, char next_char) const;

    // Formats a dialogue buffer applying soft word-wrapping and casing heuristics
    std::string process_dialogue_text(std::string_view raw_text);

    // Formats and returns the rewrapped dialogue text for a cutscene pointer
    std::string get_rewrapped_dialogue(uint32_t text_ptr);

private:
    DialogueEnhancer();

    DialogueEnhancerSettings settings_;

    uint32_t cached_ptr_ = 0;
    std::vector<uint16_t> cached_words_;
};

} // namespace khcom
