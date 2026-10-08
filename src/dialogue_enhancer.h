#pragma once

#include "font_resizer.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace khcom {

enum class FontDensity : int {
    Original    = 0, // 100% standard GBA glyph spacing (~26 chars)
    Compact     = 1, // 80% compact glyph spacing (~34 chars)
    HighDensity = 2  // 65% high-density glyph spacing (~42 chars)
};

enum class LineCapacityMode : int {
    Authentic3Lines = 0, // Authentic 3 lines (standard GBA)
    Dense2Lines     = 1, // Prioritize 2 dense lines (fewer boxes)
    Expanded4Lines  = 2  // Expanded 4 lines (widescreen capacity)
};

struct DialogueEnhancerSettings {
    FontDensity density = FontDensity::Compact;
    FontScale scale = FontScale::Compact70;
    FontStyle style = FontStyle::CleanModern;
    LineCapacityMode line_capacity = LineCapacityMode::Dense2Lines;
    bool soft_word_wrap = true;
    bool case_aware_continuation = true;
    int max_line_width_chars = 34; // Default Compact is 34 chars
    int max_line_width_px = 152;   // Safe GBA text box line pixel width (fits within 168px margin)
};

class DialogueEnhancer {
public:
    static DialogueEnhancer& instance();

    const DialogueEnhancerSettings& settings() const { return settings_; }
    DialogueEnhancerSettings& settings() { return settings_; }

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

    void recompute_line_width();

    DialogueEnhancerSettings settings_;

    uint32_t cached_ptr_ = 0;
    std::vector<uint16_t> cached_words_;
};

} // namespace khcom
