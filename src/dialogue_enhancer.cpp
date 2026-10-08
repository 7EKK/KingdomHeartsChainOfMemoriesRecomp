#include "dialogue_enhancer.h"
#include "dialogue_backlog.h"
#include "font_resizer.h"
#include "dialogue_box_scaler.h"
#include "cutscene_dialogue_data.inl"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

extern "C" uint16_t bus_read_u16(uint32_t addr);

namespace khcom {

namespace {

static const DialogueLookupEntry* find_cutscene_entry(uint32_t addr) {
    if (addr < kCutsceneDialogues[0].text_ptr) return nullptr;
    auto it = std::upper_bound(std::begin(kCutsceneDialogues), std::end(kCutsceneDialogues), addr,
        [](uint32_t val, const DialogueLookupEntry& e) {
            return val < e.text_ptr;
        });
    if (it != std::begin(kCutsceneDialogues)) {
        --it;
        if (addr >= it->text_ptr && addr < it->text_ptr + it->length_bytes) {
            return &(*it);
        }
    }
    return nullptr;
}

// Standard GBA KH:CoM proportional width table for ASCII 32 (' ') to 126 ('~')
static const uint8_t kAsciiWidths[95] = {
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

static inline int get_char_px_width(uint16_t w) {
    if (w < 128) {
        return FontResizer::instance().get_char_advance_width(static_cast<char>(w));
    }
    if (w == 0x0092) return FontResizer::instance().get_char_advance_width('\'');
    if (w == 0x0085) return FontResizer::instance().get_char_advance_width('.') * 3;
    if (w == 0x0093 || w == 0x0094) return FontResizer::instance().get_char_advance_width('"');
    return FontResizer::instance().get_glyph_width();
}

static std::vector<uint16_t> rewrap_words_in_place(const std::vector<uint16_t>& in,
                                                   int max_line_len,
                                                   int max_line_px,
                                                   bool soft_wrap,
                                                   bool case_aware) {
    std::vector<uint16_t> out = in;
    if (!soft_wrap || out.empty() || max_line_len <= 0 || max_line_px <= 0) return out;

    int cur_line_len = 0;
    int cur_line_px = 0;
    int last_space_idx = -1;
    const int space_px = FontResizer::instance().get_char_advance_width(' ');

    for (size_t i = 0; i < out.size(); ++i) {
        uint16_t w = out[i];
        if (w == 0x000A) { // Newline
            uint16_t prev_w = (i > 0) ? out[i - 1] : 0;
            uint16_t next_w = (i + 1 < out.size()) ? out[i + 1] : 0;

            bool is_sentence_terminator = (prev_w == '.' || prev_w == '!' || prev_w == '?' ||
                                           prev_w == ':' || prev_w == ';');
            bool next_is_lower = (next_w >= 'a' && next_w <= 'z');
            bool prev_is_comma_or_word = (prev_w == ',' || (prev_w >= 'A' && prev_w <= 'Z') || (prev_w >= 'a' && prev_w <= 'z'));
            bool next_is_upper = (next_w >= 'A' && next_w <= 'Z');

            bool can_merge = false;
            if (!case_aware) {
                can_merge = !is_sentence_terminator;
            } else {
                if (!is_sentence_terminator && next_is_lower) {
                    can_merge = true;
                } else if (prev_is_comma_or_word && next_is_upper) {
                    can_merge = true;
                } else if (is_sentence_terminator && next_is_upper) {
                    can_merge = false;
                } else {
                    can_merge = !is_sentence_terminator;
                }
            }

            // Measure next word length and pixel width
            int next_word_len = 0;
            int next_word_px = 0;
            for (size_t j = i + 1; j < out.size(); ++j) {
                if (out[j] == 0x000A || out[j] == 0x0020 || out[j] == 0 || out[j] >= 0xE000) break;
                ++next_word_len;
                next_word_px += get_char_px_width(out[j]);
            }

            if (can_merge &&
                (cur_line_len + 1 + next_word_len <= max_line_len) &&
                (cur_line_px + space_px + next_word_px <= max_line_px)) {
                out[i] = 0x0020; // Merge newline into space!
                cur_line_len += 1;
                cur_line_px += space_px;
                last_space_idx = static_cast<int>(i);
            } else {
                cur_line_len = 0;
                cur_line_px = 0;
                last_space_idx = -1;
            }
        } else if (w == 0x0020) {
            last_space_idx = static_cast<int>(i);
            ++cur_line_len;
            cur_line_px += space_px;
        } else if (w == 0 || w >= 0xE000) {
            break;
        } else {
            const int c_px = get_char_px_width(w);
            ++cur_line_len;
            cur_line_px += c_px;
            if ((cur_line_len > max_line_len || cur_line_px > max_line_px) && last_space_idx != -1) {
                out[last_space_idx] = 0x000A; // Wrap at last space
                cur_line_len = 0;
                cur_line_px = 0;
                for (size_t k = last_space_idx + 1; k <= i; ++k) {
                    ++cur_line_len;
                    cur_line_px += get_char_px_width(out[k]);
                }
                last_space_idx = -1;
            }
        }
    }
    return out;
}

} // namespace

DialogueEnhancer& DialogueEnhancer::instance() {
    static DialogueEnhancer s_instance;
    return s_instance;
}

DialogueEnhancer::DialogueEnhancer() {
    recompute_line_width();
}

void DialogueEnhancer::recompute_line_width() {
    // 1. Calculate available text pixel width dynamically:
    // Base GBA dialogue box has 152px usable text area (28 tiles = 224px minus portrait & borders).
    // In widescreen or responsive modes, extra horizontal tiles widen the dialogue box.
    int extra_tiles = DialogueBoxScaler::instance().settings().extra_horizontal_tiles;
    int extra_px = extra_tiles * 8;
    settings_.max_line_width_px = 152 + extra_px;

    // 2. Calculate average character width dynamically based on active FontResizer metrics:
    float avg_char_w = FontResizer::instance().get_average_char_width();
    if (avg_char_w < 2.0f) avg_char_w = 2.0f;

    // 3. Dynamic max characters per line:
    int dynamic_chars = static_cast<int>(settings_.max_line_width_px / avg_char_w);

    // Fine-tune based on line capacity mode
    if (settings_.line_capacity == LineCapacityMode::Dense2Lines) {
        dynamic_chars += 2;
    } else if (settings_.line_capacity == LineCapacityMode::Expanded4Lines) {
        dynamic_chars += 4;
    }

    settings_.max_line_width_chars = std::clamp(dynamic_chars, 24, 80);
}

void DialogueEnhancer::set_density(FontDensity density) {
    settings_.density = density;
    FontResizer::instance().set_density(density);
    recompute_line_width();
    invalidate_cache();
}

void DialogueEnhancer::set_font_scale(FontScale scale) {
    settings_.scale = scale;
    FontResizer::instance().set_font_scale(scale);
    recompute_line_width();
    invalidate_cache();
}

void DialogueEnhancer::set_font_style(FontStyle style) {
    settings_.style = style;
    FontResizer::instance().set_font_style(style);
    recompute_line_width();
    invalidate_cache();
}

void DialogueEnhancer::set_line_capacity(LineCapacityMode mode) {
    settings_.line_capacity = mode;
    recompute_line_width();
    invalidate_cache();
}

void DialogueEnhancer::set_soft_word_wrap(bool enable) {
    settings_.soft_word_wrap = enable;
    invalidate_cache();
}

void DialogueEnhancer::set_case_aware_continuation(bool enable) {
    settings_.case_aware_continuation = enable;
    invalidate_cache();
}

void DialogueEnhancer::invalidate_cache() {
    cached_ptr_ = 0;
    cached_words_.clear();
}

bool DialogueEnhancer::intercept_bus_read(uint32_t addr, uint32_t width, uint32_t* out_val) {
    if (!out_val) return false;
    if (addr < 0x08FBD378u || addr > 0x08FFE504u + 1024) {
        return false;
    }

    if (settings_.density == FontDensity::Original &&
        settings_.scale == FontScale::Original100 &&
        settings_.style == FontStyle::Authentic &&
        settings_.line_capacity == LineCapacityMode::Authentic3Lines &&
        !settings_.soft_word_wrap) {
        return false;
    }

    static bool s_in_intercept = false;
    if (s_in_intercept) return false;

    // Check hit in active cached dialogue
    if (cached_ptr_ != 0 && addr >= cached_ptr_ && addr < cached_ptr_ + cached_words_.size() * 2) {
        size_t idx = (addr - cached_ptr_) / 2;
        if (width == 2) {
            *out_val = cached_words_[idx];
            return true;
        } else if (width == 1) {
            uint16_t w = cached_words_[idx];
            *out_val = (addr & 1) ? ((w >> 8) & 0xFF) : (w & 0xFF);
            return true;
        } else if (width == 4) {
            uint32_t w0 = cached_words_[idx];
            uint32_t w1 = (idx + 1 < cached_words_.size()) ? cached_words_[idx + 1] : 0;
            *out_val = (w1 << 16) | (w0 & 0xFFFF);
            return true;
        }
    }

    const DialogueLookupEntry* match = find_cutscene_entry(addr);
    if (!match) return false;

    s_in_intercept = true;
    cached_ptr_ = match->text_ptr;
    cached_words_.clear();
    cached_words_.reserve(128);

    {
        ScopedInternalBusRead guard;
        for (size_t i = 0; i < 512; ++i) {
            uint16_t w = bus_read_u16(cached_ptr_ + i * 2);
            cached_words_.push_back(w);
            if (w == 0 || w >= 0xE000) break;
        }
    }

    // Dynamic line wrapping based on current font size, style, density, and box width
    recompute_line_width();
    const int bus_max_chars = settings_.max_line_width_chars;
    const int bus_max_px = settings_.max_line_width_px;
    cached_words_ = rewrap_words_in_place(cached_words_, bus_max_chars, bus_max_px,
                                          settings_.soft_word_wrap, settings_.case_aware_continuation);
    s_in_intercept = false;

    if (addr >= cached_ptr_ && addr < cached_ptr_ + cached_words_.size() * 2) {
        size_t idx = (addr - cached_ptr_) / 2;
        if (width == 2) {
            *out_val = cached_words_[idx];
            return true;
        } else if (width == 1) {
            uint16_t w = cached_words_[idx];
            *out_val = (addr & 1) ? ((w >> 8) & 0xFF) : (w & 0xFF);
            return true;
        } else if (width == 4) {
            uint32_t w0 = cached_words_[idx];
            uint32_t w1 = (idx + 1 < cached_words_.size()) ? cached_words_[idx + 1] : 0;
            *out_val = (w1 << 16) | (w0 & 0xFFFF);
            return true;
        }
    }

    return false;
}

int DialogueEnhancer::get_glyph_advance_width(int base_width) const {
    switch (settings_.density) {
        case FontDensity::Compact:
            return (base_width * 4) / 5; // 80% spacing
        case FontDensity::HighDensity:
            return (base_width * 13) / 20; // 65% spacing
        case FontDensity::Original:
        default:
            return base_width;
    }
}

bool DialogueEnhancer::should_merge_newline(char prev_char, char next_char) const {
    if (!settings_.soft_word_wrap) {
        return false;
    }

    if (std::isspace(static_cast<unsigned char>(next_char))) {
        return false;
    }

    bool is_sentence_terminator = (prev_char == '.' || prev_char == '!' || prev_char == '?' ||
                                   prev_char == ':' || prev_char == ';');

    if (!settings_.case_aware_continuation) {
        return !is_sentence_terminator;
    }

    if (!is_sentence_terminator && std::islower(static_cast<unsigned char>(next_char))) {
        return true;
    }

    if ((prev_char == ',' || std::isalpha(static_cast<unsigned char>(prev_char))) &&
        std::isupper(static_cast<unsigned char>(next_char))) {
        return true;
    }

    if (is_sentence_terminator && std::isupper(static_cast<unsigned char>(next_char))) {
        return false;
    }

    return !is_sentence_terminator;
}

std::string DialogueEnhancer::process_dialogue_text(std::string_view raw_text) {
    if (!settings_.soft_word_wrap || raw_text.empty()) {
        return std::string(raw_text);
    }

    recompute_line_width();

    std::string result;
    result.reserve(raw_text.size());

    int current_line_len = 0;
    int current_line_px = 0;
    const int max_len = settings_.max_line_width_chars;
    const int max_px = settings_.max_line_width_px;
    const int space_px = FontResizer::instance().get_char_advance_width(' ');

    for (size_t i = 0; i < raw_text.size(); ++i) {
        char c = raw_text[i];

        if (c == '\n' || c == '\r') {
            char prev = (i > 0) ? raw_text[i - 1] : ' ';
            char next = (i + 1 < raw_text.size()) ? raw_text[i + 1] : ' ';

            // Measure next word length and px width so we don't merge if next word exceeds limits
            int next_word_len = 0;
            int next_word_px = 0;
            for (size_t j = i + 1; j < raw_text.size(); ++j) {
                if (raw_text[j] == '\n' || raw_text[j] == '\r' || raw_text[j] == ' ') break;
                ++next_word_len;
                next_word_px += get_char_px_width(static_cast<uint8_t>(raw_text[j]));
            }

            if (should_merge_newline(prev, next) &&
                (current_line_len + 1 + next_word_len <= max_len) &&
                (current_line_px + space_px + next_word_px <= max_px)) {
                if (!result.empty() && result.back() != ' ') {
                    result.push_back(' ');
                    ++current_line_len;
                    current_line_px += space_px;
                }
            } else {
                result.push_back('\n');
                current_line_len = 0;
                current_line_px = 0;
            }
        } else {
            result.push_back(c);
            const int c_px = get_char_px_width(static_cast<uint8_t>(c));
            ++current_line_len;
            current_line_px += c_px;

            if ((current_line_len >= max_len || current_line_px >= max_px) && c == ' ') {
                result.back() = '\n';
                current_line_len = 0;
                current_line_px = 0;
            }
        }
    }

    if (result.size() >= 3) {
        DialogueBacklog::instance().push_entry("Story", result);
    }

    return result;
}

std::string DialogueEnhancer::get_rewrapped_dialogue(uint32_t text_ptr) {
    if (text_ptr < 0x08FBD378u || text_ptr > 0x08FFE504u) {
        return "";
    }
    std::vector<uint16_t> words;
    words.reserve(128);
    {
        ScopedInternalBusRead guard;
        for (size_t i = 0; i < 512; ++i) {
            uint16_t w = bus_read_u16(text_ptr + i * 2);
            if (w == 0 || w >= 0xE000) break;
            words.push_back(w);
        }
    }
    recompute_line_width();
    const int bus_max_chars = settings_.max_line_width_chars;
    const int bus_max_px = settings_.max_line_width_px;
    std::vector<uint16_t> rewrapped = rewrap_words_in_place(words, bus_max_chars, bus_max_px,
                                                            settings_.soft_word_wrap, settings_.case_aware_continuation);
    std::string result;
    result.reserve(rewrapped.size() + 8);
    for (uint16_t w : rewrapped) {
        if (w == 0x000A) {
            result.push_back('\n');
        } else if (w >= 32 && w <= 126) {
            result.push_back(static_cast<char>(w));
        } else if (w == 0x85) {
            result += "...";
        } else if (w == 0x92) {
            result += "'";
        } else if (w == 0x93 || w == 0x94) {
            result += "\"";
        } else if (w == 0xE9) {
            result += "é";
        }
    }
    return result;
}

} // namespace khcom
