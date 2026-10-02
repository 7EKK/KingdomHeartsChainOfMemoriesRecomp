#include "audio_dsp.h"
#include "screen_filters.h"
#include "frame_interpolator.h"
#include "input_enhancements.h"
#include "hud_anchoring.h"
#include "dialog_turbo.h"
#include "xbrz_filter.h"
#include "savestate_thumbnails.h"
#include "perf_hud.h"
#include "dialogue_enhancer.h"
#include "dialogue_backlog.h"
#include "font_resizer.h"
#include "dialogue_box_scaler.h"
#include "widescreen_adapter.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>

using namespace khcom;

static void test_audio_dsp() {
    auto& dsp = AudioDsp::instance();
    
    dsp.apply_preset(EqPreset::BassBoost);
    assert(dsp.settings().preset == EqPreset::BassBoost);
    assert(dsp.settings().bass_gain_db == 5.0f);
    assert(dsp.settings().mid_gain_db == -1.0f);
    assert(dsp.settings().treble_gain_db == 0.5f);

    dsp.apply_preset(EqPreset::Flat);
    assert(dsp.settings().preset == EqPreset::Flat);
    assert(dsp.settings().bass_gain_db == 0.0f);
    assert(dsp.settings().mid_gain_db == 0.0f);
    assert(dsp.settings().treble_gain_db == 0.0f);

    dsp.set_stereo_width(1.5f);
    assert(dsp.settings().stereo_width == 1.5f);

    dsp.set_anti_aliasing(true);
    assert(dsp.settings().anti_aliasing_filter == true);

    dsp.set_limiter(true);
    assert(dsp.settings().limiter_enabled == true);

    // Buffer processing test with extreme audio values (soft limiter saturation test)
    int16_t samples[8] = { 32000, 32000, 32700, 32700, -32000, -32000, -32700, -32700 };
    dsp.process_stereo(samples, 4, 32768);
    for (int i = 0; i < 8; ++i) {
        assert(samples[i] >= -32768 && samples[i] <= 32767);
    }

    std::cout << "[PASS] C++ AudioDsp unit test passed" << std::endl;
}

static void test_screen_filters() {
    auto& filters = ScreenFilters::instance();

    filters.set_color_profile(ColorProfile::Ags101);
    assert(filters.settings().color_profile == ColorProfile::Ags101);

    filters.set_mask_type(ScreenMaskType::CrtScanlines);
    assert(filters.settings().mask_type == ScreenMaskType::CrtScanlines);

    filters.set_mask_intensity(0.01f);
    assert(filters.settings().mask_intensity == 0.05f); // clamped min

    filters.set_mask_intensity(2.5f);
    assert(filters.settings().mask_intensity == 1.0f); // clamped max

    filters.set_mask_intensity(0.45f);
    assert(filters.settings().mask_intensity == 0.45f);

    // Test raw color correction identity
    filters.set_color_profile(ColorProfile::Raw);
    uint8_t test_pixels[6] = { 10, 20, 30, 200, 210, 220 };
    filters.apply_color_correction(test_pixels, 2, 1);
    assert(test_pixels[0] == 10 && test_pixels[1] == 20 && test_pixels[2] == 30);
    assert(test_pixels[3] == 200 && test_pixels[4] == 210 && test_pixels[5] == 220);

    std::cout << "[PASS] C++ ScreenFilters unit test passed" << std::endl;
}

static void test_frame_interpolator() {
    auto& interp = FrameInterpolator::instance();

    interp.set_mode(FramerateMode::Fps60);
    assert(interp.target_fps() == 60.0);
    assert(std::abs(interp.target_frametime_ms() - 16.6666) < 0.01);

    interp.set_mode(FramerateMode::Fps120);
    assert(interp.target_fps() == 120.0);
    assert(std::abs(interp.target_frametime_ms() - 8.3333) < 0.01);

    interp.set_mode(FramerateMode::Fps144);
    assert(interp.target_fps() == 144.0);
    assert(std::abs(interp.target_frametime_ms() - 6.9444) < 0.01);

    interp.set_mode(FramerateMode::Uncapped);
    assert(interp.target_fps() == 240.0);
    assert(std::abs(interp.target_frametime_ms() - 4.1666) < 0.01);

    interp.set_smoothing(MotionSmoothingMode::MotionAdaptive);
    assert(interp.settings().smoothing == MotionSmoothingMode::MotionAdaptive);

    std::cout << "[PASS] C++ FrameInterpolator unit test passed" << std::endl;
}

static void test_input_enhancements() {
    auto& input = InputEnhancements::instance();

    input.set_deadzone(0.01f);
    assert(input.settings().deadzone == 0.05f); // clamped min

    input.set_deadzone(0.80f);
    assert(input.settings().deadzone == 0.50f); // clamped max

    input.set_deadzone(0.18f);
    assert(input.settings().deadzone == 0.18f);

    input.set_analog_mode(AnalogMode::Analog8Way);
    assert(input.settings().analog_mode == AnalogMode::Analog8Way);

    // Full tilt Right
    uint16_t key = 0xFFFF;
    input.process_axis_input(32767, 0, key);
    assert((key & 0x0010) == 0); // Right bit active low
    assert((key & 0x0020) != 0); // Left not pressed

    // Full tilt Down-Right
    key = 0xFFFF;
    input.process_axis_input(32767, 32767, key);
    assert((key & 0x0010) == 0); // Right
    assert((key & 0x0080) == 0); // Down

    // Full tilt Left
    key = 0xFFFF;
    input.process_axis_input(-32767, 0, key);
    assert((key & 0x0020) == 0); // Left

    // Full tilt Up
    key = 0xFFFF;
    input.process_axis_input(0, -32767, key);
    assert((key & 0x0040) == 0); // Up

    // Tilt inside deadzone
    key = 0xFFFF;
    input.process_axis_input(100, 100, key);
    assert(key == 0xFFFF); // No directional change

    std::cout << "[PASS] C++ InputEnhancements unit test passed" << std::endl;
}

static void test_hud_anchoring() {
    auto& hud = HudAnchoring::instance();

    hud.set_mode(HudAnchorMode::WidescreenAnchored);
    hud.set_horizontal_offset(32);
    assert(hud.settings().horizontal_offset == 32);

    // Top-left Health Bar (x=24, y=10)
    int hp_x = 24, hp_y = 10;
    hud.adjust_sprite_coordinate(hp_x, hp_y, 16, 16, true);
    assert(hp_x == 24 - 32);

    // Bottom-right Card Deck (x=200, y=120)
    int deck_x = 200, deck_y = 120;
    hud.adjust_sprite_coordinate(deck_x, deck_y, 16, 16, true);
    assert(deck_x == 200 + 32);

    // Center sprite (x=100, y=50) -> must not shift
    int char_x = 100, char_y = 50;
    hud.adjust_sprite_coordinate(char_x, char_y, 16, 16, false);
    assert(char_x == 100 && char_y == 50);

    // OAM processing
    uint16_t attr0 = 10;
    uint16_t attr1 = 24;
    hud.process_oam_entry(attr0, attr1);
    int shifted_oam_x = static_cast<int16_t>(attr1 & 0x01FF);
    if (shifted_oam_x >= 256) shifted_oam_x -= 512;
    assert(shifted_oam_x == -8);

    std::cout << "[PASS] C++ HudAnchoring unit test passed" << std::endl;
}

static void test_dialog_turbo() {
    auto& turbo = DialogTurbo::instance();

    turbo.set_enabled(true);
    turbo.set_held(true);
    turbo.set_skip_speed(2);
    assert(turbo.skip_speed() == 2);

    // Even frames pulse A and B buttons (clearing bits 0 and 1)
    uint16_t k0 = turbo.process_keyinput(0xFFFF, 0);
    assert((k0 & 0x0001) == 0); // A pressed
    assert((k0 & 0x0002) == 0); // B pressed

    // Odd frames release A and B buttons
    uint16_t k1 = turbo.process_keyinput(0xFFFF, 1);
    assert((k1 & 0x0001) != 0); // A released
    assert((k1 & 0x0002) != 0); // B released

    // When released (not held), input remains unchanged
    turbo.set_held(false);
    uint16_t k_held = turbo.process_keyinput(0xFFFF, 0);
    assert(k_held == 0xFFFF);

    std::cout << "[PASS] C++ DialogTurbo unit test passed" << std::endl;
}

static void test_xbrz_filter() {
    auto& xbrz = XbrzUpscaler::instance();

    xbrz.set_scale(XbrzScale::Xbrz2x);
    assert(xbrz.scale() == XbrzScale::Xbrz2x);

    xbrz.set_scale(XbrzScale::Xbrz3x);
    assert(xbrz.scale() == XbrzScale::Xbrz3x);

    xbrz.set_scale(XbrzScale::Xbrz4x);
    assert(xbrz.scale() == XbrzScale::Xbrz4x);

    xbrz.set_scale(XbrzScale::None);
    assert(xbrz.scale() == XbrzScale::None);

    std::cout << "[PASS] C++ XbrzUpscaler unit test passed" << std::endl;
}

static void test_savestate_thumbnails() {
    auto& sm = SavestateThumbnailManager::instance();

    std::string path_slot_1 = sm.get_thumbnail_path(1);
    assert(path_slot_1.find("slot_1") != std::string::npos);

    std::string path_slot_9 = sm.get_thumbnail_path(9);
    assert(path_slot_9.find("slot_9") != std::string::npos);

    SavestateMetadata meta = sm.get_slot_metadata(1);
    assert(meta.slot == 1);

    std::cout << "[PASS] C++ SavestateThumbnailManager unit test passed" << std::endl;
}

static void test_perf_hud() {
    auto& hud = PerfHud::instance();

    hud.set_mode(PerfHudMode::FpsAndFrametime);
    assert(hud.settings().mode == PerfHudMode::FpsAndFrametime);

    hud.set_position(PerfHudPosition::TopRight);
    assert(hud.settings().position == PerfHudPosition::TopRight);

    hud.set_theme(PerfHudTheme::GlassDark);
    assert(hud.settings().theme == PerfHudTheme::GlassDark);

    std::cout << "[PASS] C++ PerfHud unit test passed" << std::endl;
}

static void test_dialogue_components() {
    auto& enhancer = DialogueEnhancer::instance();
    auto& backlog = DialogueBacklog::instance();

    // Enhancer tests
    std::string t1 = enhancer.process_dialogue_text("Where are we?\nDonald? Goofy?");
    assert(t1.find('\n') != std::string::npos);

    std::string t2 = enhancer.process_dialogue_text("Ahead lies what you seek,\nbut to claim it, you must lose.");
    assert(t2.find("seek, but") != std::string::npos);

    // Backlog tests
    backlog.set_open(false);
    assert(!backlog.is_open());

    backlog.set_open(true);
    assert(backlog.is_open());

    backlog.toggle_open();
    assert(!backlog.is_open());

    backlog.clear();
    backlog.push_entry("Naminé", "I'm sorry, Sora... I altered your memories.");
    assert(!backlog.entries().empty());
    assert(backlog.entries().back().speaker == "Naminé");
    assert(backlog.entries().back().text.find("altered your memories") != std::string::npos);

    std::cout << "[PASS] C++ DialogueEnhancer and DialogueBacklog unit tests passed" << std::endl;
}

static void test_font_resizer() {
    auto& fr = FontResizer::instance();

    fr.set_font_scale(FontScale::Original100);
    assert(fr.get_glyph_width() == 8);
    assert(fr.get_glyph_height() == 12);
    assert(fr.settings().line_spacing == 16);

    fr.set_font_scale(FontScale::Medium85);
    assert(fr.get_glyph_width() == 7);
    assert(fr.get_glyph_height() == 10);
    assert(fr.settings().line_spacing == 13);

    fr.set_font_scale(FontScale::Compact70);
    assert(fr.get_glyph_width() == 6);
    assert(fr.get_glyph_height() == 8);
    assert(fr.settings().line_spacing == 10);

    fr.set_font_scale(FontScale::Micro55);
    assert(fr.get_glyph_width() == 5);
    assert(fr.get_glyph_height() == 7);
    assert(fr.settings().line_spacing == 8);

    // Test proportional advance widths
    int w_space = fr.get_char_advance_width(' ');
    int w_m = fr.get_char_advance_width('m');
    assert(w_m >= w_space);

    // Test 4bpp tile downsampling
    uint8_t src_1bpp[12] = { 0x3C, 0x42, 0x81, 0x81, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x00, 0x00, 0x00 };
    uint8_t dst_4bpp[32];
    fr.render_scaled_glyph_4bpp(src_1bpp, dst_4bpp, 2);
    bool has_pixels = false;
    for (int i = 0; i < 32; ++i) {
        if (dst_4bpp[i] != 0) has_pixels = true;
    }
    assert(has_pixels);

    std::cout << "[PASS] C++ FontResizer unit test passed" << std::endl;
}

static void test_dialogue_box_scaler() {
    auto& dbs = DialogueBoxScaler::instance();

    dbs.set_width_mode(BoxWidthMode::Standard240);
    assert(dbs.get_box_width_tiles() == 28);
    assert(dbs.get_box_width_pixels() == 224);

    dbs.set_width_mode(BoxWidthMode::WidescreenExpanded);
    assert(dbs.get_box_width_tiles() == 34);
    assert(dbs.get_box_width_pixels() == 272);

    dbs.set_width_mode(BoxWidthMode::DynamicResponsive);
    assert(dbs.get_box_width_tiles() == 36);
    assert(dbs.get_box_width_pixels() == 288);

    // Test nine-slice tilemap row horizontal stretching
    uint16_t row[40];
    row[0] = 0x0100; // Left cap
    row[1] = 0x0101; // Center repeating tile
    row[27] = 0x0102; // Right cap
    dbs.expand_dialogue_tilemap_row(row, 28, 34);
    assert(row[0] == 0x0100);
    for (int i = 1; i < 33; ++i) {
        assert(row[i] == 0x0101);
    }
    assert(row[33] == 0x0102);

    // Test dialogue OAM adjustment
    uint16_t a0 = 120; // lower third dialogue region
    uint16_t a1 = 200; // right border corner
    dbs.adjust_dialogue_oam(a0, a1, 284);
    int shifted_x = a1 & 0x01FF;
    if (shifted_x >= 256) shifted_x -= 512;
    assert(shifted_x > 200); // Shifted right for expanded box

    std::cout << "[PASS] C++ DialogueBoxScaler unit test passed" << std::endl;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "           KHCOMRecomp Native C++ Unit Test Runner             " << std::endl;
    std::cout << "================================================================" << std::endl;

    test_audio_dsp();
    test_screen_filters();
    test_frame_interpolator();
    test_input_enhancements();
    test_hud_anchoring();
    test_dialog_turbo();
    test_xbrz_filter();
    test_savestate_thumbnails();
    test_perf_hud();
    test_dialogue_components();
    test_font_resizer();
    test_dialogue_box_scaler();

    std::cout << "================================================================" << std::endl;
    std::cout << "ALL NATIVE C++ UNIT TESTS PASSED SUCCESSFULLY (12/12 MODULES)." << std::endl;
    std::cout << "================================================================" << std::endl;
    return 0;
}
