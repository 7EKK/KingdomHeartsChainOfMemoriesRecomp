#include "game_config.h"
#include "audio_dsp.h"
#include "perf_hud.h"
#include "frame_interpolator.h"
#include "screen_filters.h"
#include "input_enhancements.h"
#include "hd_audio_player.h"
#include "xbrz_filter.h"
#include "dialog_turbo.h"
#include "hud_anchoring.h"
#include "dialogue_enhancer.h"
#include "dialogue_backlog.h"
#include "widescreen_adapter.h"
#include "runtime_bus_bridge.h"
#include "gba/gba_bus.h"
#include "gba/gba_io.h"
#include <cstring>
#include <fstream>
#include <string>

#if defined(GBARECOMP_RUNTIME_UI)
#include "recomp_runtime_ui.h"
#endif

namespace khcom {

namespace {

const char* const kAspectLabels[] = {
    "3:2 (Native)",
    "16:9 (Battles only)"
};

const std::uint16_t kAspectWidths[] = {
    240,
    284
};

#if defined(GBARECOMP_RUNTIME_UI)
const char* const kFpsTargetChoices[] = {
    "60 FPS (Original GBA)",
    "120 FPS (2x High Rate)",
    "144 FPS",
    "Display Native (Auto)",
    "Uncapped"
};

const char* const kMotionSmoothingChoices[] = {
    "Off (Direct Presentation)",
    "Smooth Blend (50% Lerp)",
    "Motion-Adaptive"
};

const char* const kXbrzChoices[] = {
    "Off (Original Pixels)",
    "2x xBRZ (Enhanced Edges)",
    "3x xBRZ (Crisp Vectors)",
    "4x xBRZ (Ultra Detail)",
    "5x xBRZ (Retina Smoothing)"
};

const char* const kColorProfileChoices[] = {
    "Raw (Uncorrected)",
    "AGB-001 (Reflective TFT)",
    "AGS-001 (SP Frontlit)",
    "AGS-101 (SP Backlit)",
    "MiSTer Gamma 1.6",
    "MiSTer Gamma 2.2"
};

const char* const kScreenMaskChoices[] = {
    "Off (Clean Pixels)",
    "MiSTer LCD Grid (TFT Matrix)",
    "MiSTer Subpixel RGB (Vertical)",
    "MiSTer Subpixel BGR",
    "MiSTer Diffusion (AGS-001)",
    "Game Boy Player Scanlines",
    "CRT Trinitron Aperture Grille"
};

const char* const kAnalogChoices[] = {
    "Digital (Original 8-Way)",
    "Analog 8-Way (Smooth Stick)",
    "True 360 Walk & Run"
};

const char* const kGamepadProfileChoices[] = {
    "Xbox / Standard (A: Atk, B: Jump, LB/RB: Deck)",
    "Nintendo Style (B: Atk, A: Jump, LB/RB: Deck)",
    "PlayStation Style (Square/X: Atk, Cross/A: Jump)",
    "Swapped Shoulders (RB: Deck L, LB: Deck R)",
    "Triggers for Deck (LT: Deck L, RT: Deck R)",
    "Custom Profile"
};

const char* const kPadButtonChoices[] = {
    "Button A (Bottom / Cross)",
    "Button B (Right / Circle)",
    "Button X (Left / Square)",
    "Button Y (Top / Triangle)",
    "Left Bumper (LB / L1)",
    "Right Bumper (RB / R1)",
    "Left Trigger (LT / L2)",
    "Right Trigger (RT / R2)"
};

const char* const kKeyboardPresetChoices[] = {
    "Default (X, Z, C, V, Return, RShift)",
    "WASD + J/K/U/I (Modern PC)",
    "Classic Emulation (Z, X, A, S)",
    "Arrows + Z/X/Q/W",
    "Custom"
};

const char* const kHudAnchorChoices[] = {
    "Original Centered (GBA 3:2)",
    "Widescreen Anchored (Corners)"
};

const char* const kFontDensityChoices[] = {
    "Original (100% GBA Spacing)",
    "Compact (80% Spacing)",
    "High-Density (65% Spacing)"
};

const char* const kFontScaleChoices[] = {
    "Authentic (100% - 8x12 Standard)",
    "Medium (85% - 7x10)",
    "Compact (70% - 6x8)",
    "Micro (55% - 5x7)"
};

const char* const kFontStyleChoices[] = {
    "Authentic GBA Proportional",
    "Clean Modern Sans",
    "Condensed Space-Saver"
};

const char* const kLineCapacityChoices[] = {
    "Authentic (3 Lines GBA)",
    "Dense (2 Lines Compact)",
    "Expanded (4 Lines Widescreen)"
};

const char* const kEqChoices[] = {
    "Flat (Authentic)",
    "Warm Retro",
    "Crisp Modern",
    "Bass Boost"
};

const char* const kPerfModeChoices[] = {
    "Disabled",
    "FPS Counter Only",
    "FPS + Frametime ms",
    "Full HUD + Frametime Graph"
};

const char* const kPerfPositionChoices[] = {
    "Top-Left",
    "Top-Right",
    "Bottom-Left",
    "Bottom-Right",
    "Bottom Black Bar (Letterbox)",
    "Custom (Drag with Mouse)"
};

const char* const kPerfThemeChoices[] = {
    "Glassmorphism Dark",
    "Neon Cyberpunk",
    "Solid Dark",
    "Minimal (Transparent)"
};

const RecompRuntimeUiItem kExtraItems[] = {
    {
        "video.fps_target",
        "Graphics",
        "Target Framerate",
        "Target refresh rate without altering speed",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 4, 1,
        kFpsTargetChoices, 5, nullptr
    },
    {
        "video.motion_smoothing",
        "Graphics",
        "Motion Smoothing",
        "Temporal blending between simulation frames",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 2, 1,
        kMotionSmoothingChoices, 3, nullptr
    },
    {
        "video.xbrz_scale",
        "Graphics",
        "xBRZ Pixel Scaler",
        "Geometric pixel art upscaler (2x to 5x)",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 4, 1,
        kXbrzChoices, 5, nullptr
    },
    {
        "video.color_profile",
        "Graphics",
        "Color Profile / Gamma",
        "Hardware color grading and gamma curve",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 5, 1,
        kColorProfileChoices, 6, nullptr
    },
    {
        "video.screen_mask",
        "Graphics",
        "Screen Mask Type",
        "LCD grid, RGB subpixel stripes, scanlines",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 6, 1,
        kScreenMaskChoices, 7, nullptr
    },
    {
        "video.mask_intensity",
        "Graphics",
        "Mask Intensity (%)",
        "Grid darkness: 10% (subtle) to 100% (dense)",
        RECOMP_RUNTIME_UI_INT,
        10, 100, 5,
        nullptr, 0, nullptr
    },
    {
        "video.hud_anchoring",
        "Display",
        "HUD Dynamic Anchoring",
        "Shift health and cards to edges in 16:9",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 1, 1,
        kHudAnchorChoices, 2, nullptr
    },
    {
        "input.gamepad_profile",
        "Controller (Gamepad)",
        "Controller Preset",
        "Quick layout profile preset",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 5, 1,
        kGamepadProfileChoices, 6, nullptr
    },
    {
        "input.pad_attack",
        "Controller (Gamepad)",
        "Attack / Card Action",
        "Play card, attack, menu confirm",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 7, 1,
        kPadButtonChoices, 8, nullptr
    },
    {
        "input.pad_jump",
        "Controller (Gamepad)",
        "Jump / Dodge Roll",
        "Jump, dodge roll, menu cancel",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 7, 1,
        kPadButtonChoices, 8, nullptr
    },
    {
        "input.pad_deck_l",
        "Controller (Gamepad)",
        "Cycle Deck Left",
        "Scroll deck cards to the left",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 7, 1,
        kPadButtonChoices, 8, nullptr
    },
    {
        "input.pad_deck_r",
        "Controller (Gamepad)",
        "Cycle Deck Right / Lock-on",
        "Scroll deck right and lock-on",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 7, 1,
        kPadButtonChoices, 8, nullptr
    },
    {
        "input.analog_mode",
        "Controller (Gamepad)",
        "Stick Movement Mode",
        "360 walk/run or 8-way digital",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 2, 1,
        kAnalogChoices, 3, nullptr
    },
    {
        "input.deadzone",
        "Controller (Gamepad)",
        "Stick Deadzone (%)",
        "Stick drift margin (5% to 50%)",
        RECOMP_RUNTIME_UI_INT,
        5, 50, 5,
        nullptr, 0, nullptr
    },
    {
        "input.reset_pad",
        "Controller (Gamepad)",
        "Reset Gamepad to Defaults",
        "Restore default gamepad mapping",
        RECOMP_RUNTIME_UI_ACTION,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.kb_preset",
        "Keyboard Controls",
        "Keyboard Preset",
        "Keyboard layout preset",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 4, 1,
        kKeyboardPresetChoices, 5, nullptr
    },
    {
        "input.key_a",
        "Keyboard Controls",
        "Attack / Card Action",
        "Play card, attack, menu confirm",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_b",
        "Keyboard Controls",
        "Jump / Dodge Roll",
        "Jump, dodge roll, menu cancel",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_l",
        "Keyboard Controls",
        "Cycle Deck Left",
        "Scroll deck cards to the left",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_r",
        "Keyboard Controls",
        "Cycle Deck Right / Lock-on",
        "Scroll deck right and lock-on",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_start",
        "Keyboard Controls",
        "Pause / Camp Menu",
        "Open journal and pause game",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_select",
        "Keyboard Controls",
        "Switch Deck / Reload",
        "Toggle enemy cards and reload",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_up",
        "Keyboard Controls",
        "Move Up",
        "Move character upwards",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_down",
        "Keyboard Controls",
        "Move Down",
        "Move character downwards",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_left",
        "Keyboard Controls",
        "Move Left",
        "Move character to the left",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.key_right",
        "Keyboard Controls",
        "Move Right",
        "Move character to the right",
        RECOMP_RUNTIME_UI_TEXT,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "input.reset_kb",
        "Keyboard Controls",
        "Reset Keyboard to Defaults",
        "Restore default keyboard mapping",
        RECOMP_RUNTIME_UI_ACTION,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "gameplay.turbo_dialog",
        "Gameplay & Assist",
        "Turbo Dialog & Cutscene Skip",
        "Hold Tab or Y to advance text at 60Hz",
        RECOMP_RUNTIME_UI_BOOL,
        0, 1, 1,
        nullptr, 0, nullptr
    },
    {
        "dialogue.font_density",
        "Dialogue & Story",
        "Font Density",
        "Compact spacing to fit more text per line",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 2, 1,
        kFontDensityChoices, 3, nullptr
    },
    {
        "dialogue.font_scale",
        "Dialogue & Story",
        "Font Size",
        "Glyph dimensions and line spacing",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 3, 2,
        kFontScaleChoices, 4, nullptr
    },
    {
        "dialogue.font_style",
        "Dialogue & Story",
        "Font Style",
        "Pixel art font style and kerning",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 2, 1,
        kFontStyleChoices, 3, nullptr
    },
    {
        "dialogue.line_capacity",
        "Dialogue & Story",
        "Box Line Target",
        "Target box lines: 2 dense, 3 or 4 lines",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 2, 1,
        kLineCapacityChoices, 3, nullptr
    },
    {
        "dialogue.soft_word_wrap",
        "Dialogue & Story",
        "Grammar-Aware Word-Wrap",
        "Joins mid-sentence cutscene lines cleanly",
        RECOMP_RUNTIME_UI_BOOL,
        0, 1, 1,
        nullptr, 0, nullptr
    },
    {
        "dialogue.backlog_enable",
        "Dialogue & Story",
        "Conversation Log (Backlog)",
        "Translucent story history (press L or F2)",
        RECOMP_RUNTIME_UI_BOOL,
        0, 1, 1,
        nullptr, 0, nullptr
    },
    {
        "dialogue.open_backlog",
        "Dialogue & Story",
        "Open Conversation Log",
        "View transcript and story history",
        RECOMP_RUNTIME_UI_ACTION,
        0, 0, 0,
        nullptr, 0, nullptr
    },
    {
        "audio.hd_enabled",
        "Audio",
        "HD Re:CoM Soundtrack",
        "High-fidelity PS2 Re:CoM soundtrack",
        RECOMP_RUNTIME_UI_BOOL,
        0, 1, 1,
        nullptr, 0, nullptr
    },
    {
        "audio.bgm_volume",
        "Audio",
        "Music (BGM) Volume (%)",
        "Independent music volume (0% to 150%)",
        RECOMP_RUNTIME_UI_INT,
        0, 150, 5,
        nullptr, 0, nullptr
    },
    {
        "audio.sfx_volume",
        "Audio",
        "Effects (SFX) Volume (%)",
        "Independent sound effects volume (0-150%)",
        RECOMP_RUNTIME_UI_INT,
        0, 150, 5,
        nullptr, 0, nullptr
    },
    {
        "perf.mode",
        "Diagnostics & HUD",
        "Display Mode",
        "FPS and frametime overlay (or press F10)",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 3, 1,
        kPerfModeChoices, 4, nullptr
    },
    {
        "perf.position",
        "Diagnostics & HUD",
        "Screen Position",
        "Screen corner or drag anywhere with mouse",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 5, 1,
        kPerfPositionChoices, 6, nullptr
    },
    {
        "perf.theme",
        "Diagnostics & HUD",
        "Visual Theme",
        "Color palette and background opacity",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 3, 1,
        kPerfThemeChoices, 4, nullptr
    },
    {
        "audio.eq_preset",
        "Audio",
        "Equalizer Profile",
        "Real-time DSP sound equalizer profile",
        RECOMP_RUNTIME_UI_CHOICE,
        0, 3, 1,
        kEqChoices, 4, nullptr
    },
    {
        "audio.stereo_width",
        "Audio",
        "Stereo Width (%)",
        "Stereo width: 0% mono to 200% expanded",
        RECOMP_RUNTIME_UI_INT,
        0, 200, 10,
        nullptr, 0, nullptr
    },
    {
        "audio.anti_aliasing",
        "Audio",
        "DAC Anti-Aliasing",
        "Biquad filter to cut GBA audio hiss",
        RECOMP_RUNTIME_UI_BOOL,
        0, 1, 1,
        nullptr, 0, nullptr
    },
    {
        "audio.limiter",
        "Audio",
        "Dynamic Peak Limiter",
        "Dynamic limiter preventing audio clipping",
        RECOMP_RUNTIME_UI_BOOL,
        0, 1, 1,
        nullptr, 0, nullptr
    }
};

int ui_get_callback(const char* key, int* value_out) {
    if (!key || !value_out) return 0;
    auto& dsp = AudioDsp::instance();
    auto& hud = PerfHud::instance();
    auto& interp = FrameInterpolator::instance();
    auto& filters = ScreenFilters::instance();
    auto& xbrz = XbrzUpscaler::instance();
    auto& input = InputEnhancements::instance();
    auto& hd_audio = HdAudioPlayer::instance();
    auto& turbo = DialogTurbo::instance();
    auto& anchoring = HudAnchoring::instance();
    auto& enhancer = DialogueEnhancer::instance();
    auto& backlog = DialogueBacklog::instance();

    if (std::strcmp(key, "dialogue.font_density") == 0) {
        *value_out = static_cast<int>(enhancer.settings().density);
        return 1;
    }
    if (std::strcmp(key, "dialogue.font_scale") == 0) {
        *value_out = static_cast<int>(enhancer.settings().scale);
        return 1;
    }
    if (std::strcmp(key, "dialogue.font_style") == 0) {
        *value_out = static_cast<int>(enhancer.settings().style);
        return 1;
    }
    if (std::strcmp(key, "dialogue.line_capacity") == 0) {
        *value_out = static_cast<int>(enhancer.settings().line_capacity);
        return 1;
    }
    if (std::strcmp(key, "dialogue.soft_word_wrap") == 0) {
        *value_out = enhancer.settings().soft_word_wrap ? 1 : 0;
        return 1;
    }
    if (std::strcmp(key, "dialogue.backlog_enable") == 0) {
        *value_out = backlog.is_enabled() ? 1 : 0;
        return 1;
    }

    if (std::strcmp(key, "video.fps_target") == 0) {
        *value_out = static_cast<int>(interp.settings().mode);
        return 1;
    }
    if (std::strcmp(key, "video.motion_smoothing") == 0) {
        *value_out = static_cast<int>(interp.settings().smoothing);
        return 1;
    }
    if (std::strcmp(key, "video.xbrz_scale") == 0) {
        int idx = (xbrz.scale() == XbrzScale::None) ? 0 : (static_cast<int>(xbrz.scale()) - 1);
        *value_out = idx;
        return 1;
    }
    if (std::strcmp(key, "video.color_profile") == 0) {
        *value_out = static_cast<int>(filters.settings().color_profile);
        return 1;
    }
    if (std::strcmp(key, "video.screen_mask") == 0) {
        *value_out = static_cast<int>(filters.settings().mask_type);
        return 1;
    }
    if (std::strcmp(key, "video.mask_intensity") == 0) {
        *value_out = static_cast<int>(filters.settings().mask_intensity * 100.0f + 0.5f);
        return 1;
    }
    if (std::strcmp(key, "video.hud_anchoring") == 0) {
        *value_out = static_cast<int>(anchoring.settings().mode);
        return 1;
    }
    if (std::strcmp(key, "input.analog_mode") == 0) {
        *value_out = static_cast<int>(input.settings().analog_mode);
        return 1;
    }
    if (std::strcmp(key, "input.deadzone") == 0) {
        *value_out = static_cast<int>(input.settings().deadzone * 100.0f + 0.5f);
        return 1;
    }
    if (std::strcmp(key, "input.gamepad_profile") == 0) {
        *value_out = static_cast<int>(input.settings().gamepad_profile);
        return 1;
    }
    if (std::strcmp(key, "input.pad_attack") == 0) {
        *value_out = static_cast<int>(input.settings().pad_attack);
        return 1;
    }
    if (std::strcmp(key, "input.pad_jump") == 0) {
        *value_out = static_cast<int>(input.settings().pad_jump);
        return 1;
    }
    if (std::strcmp(key, "input.pad_deck_l") == 0) {
        *value_out = static_cast<int>(input.settings().pad_deck_l);
        return 1;
    }
    if (std::strcmp(key, "input.pad_deck_r") == 0) {
        *value_out = static_cast<int>(input.settings().pad_deck_r);
        return 1;
    }
    if (std::strcmp(key, "input.kb_preset") == 0) {
        *value_out = static_cast<int>(input.settings().kb_preset);
        return 1;
    }
    if (std::strcmp(key, "gameplay.turbo_dialog") == 0) {
        *value_out = turbo.is_enabled() ? 1 : 0;
        return 1;
    }
    if (std::strcmp(key, "audio.hd_enabled") == 0) {
        *value_out = hd_audio.settings().enabled ? 1 : 0;
        return 1;
    }
    if (std::strcmp(key, "audio.bgm_volume") == 0) {
        *value_out = static_cast<int>(hd_audio.settings().bgm_volume * 100.0f + 0.5f);
        return 1;
    }
    if (std::strcmp(key, "audio.sfx_volume") == 0) {
        *value_out = static_cast<int>(hd_audio.settings().sfx_volume * 100.0f + 0.5f);
        return 1;
    }
    if (std::strcmp(key, "perf.mode") == 0) {
        *value_out = static_cast<int>(hud.settings().mode);
        return 1;
    }
    if (std::strcmp(key, "perf.position") == 0) {
        *value_out = static_cast<int>(hud.settings().position);
        return 1;
    }
    if (std::strcmp(key, "perf.theme") == 0) {
        *value_out = static_cast<int>(hud.settings().theme);
        return 1;
    }
    if (std::strcmp(key, "audio.eq_preset") == 0) {
        *value_out = static_cast<int>(dsp.settings().preset);
        return 1;
    }
    if (std::strcmp(key, "audio.stereo_width") == 0) {
        *value_out = static_cast<int>(dsp.settings().stereo_width * 100.0f + 0.5f);
        return 1;
    }
    if (std::strcmp(key, "audio.anti_aliasing") == 0) {
        *value_out = dsp.settings().anti_aliasing_filter ? 1 : 0;
        return 1;
    }
    if (std::strcmp(key, "audio.limiter") == 0) {
        *value_out = dsp.settings().limiter_enabled ? 1 : 0;
        return 1;
    }
    return 0;
}

static int ui_action_callback(const char* key) {
    if (!key) return 0;
    if (std::strcmp(key, "dialogue.open_backlog") == 0) {
        DialogueBacklog::instance().set_open(true);
        return 1;
    }
    if (std::strcmp(key, "input.reset_controls") == 0) {
        InputEnhancements::instance().reset_to_defaults();
        save_khcom_config();
        return 1;
    }
    if (std::strcmp(key, "input.reset_pad") == 0) {
        InputEnhancements::instance().reset_gamepad_defaults();
        save_khcom_config();
        return 1;
    }
    if (std::strcmp(key, "input.reset_kb") == 0) {
        InputEnhancements::instance().reset_keyboard_defaults();
        save_khcom_config();
        return 1;
    }
    return 0;
}

static int ui_get_text_callback(const char* key, char* buf, std::size_t buf_len) {
    if (!key || !buf || buf_len == 0) return 0;
    auto& input = InputEnhancements::instance();
    if (std::strncmp(key, "input.key_", 10) == 0) {
        const char* name = key + 10;
        GbaKeyIndex idx = input.key_index_from_name(name);
        if (idx != KEY_COUNT) {
            const char* kname = input.get_key_name(idx);
            std::snprintf(buf, buf_len, "%s", kname);
            return 1;
        }
    }
    return 0;
}

static int ui_set_text_callback(const char* key, const char* value) {
    if (!key || !value) return 0;
    auto& input = InputEnhancements::instance();
    if (std::strncmp(key, "input.key_", 10) == 0) {
        const char* name = key + 10;
        GbaKeyIndex idx = input.key_index_from_name(name);
        if (idx != KEY_COUNT) {
            bool ok = input.set_key_from_name(idx, value);
            if (ok) {
                input.save_keybinds();
            }
            return ok ? 1 : 0;
        }
    }
    return 0;
}

static int ui_enabled_callback(const char* key) {
    if (!key) return 1;
    if (std::strcmp(key, "system.resume") == 0) {
        return 0;
    }
    return 1;
}

static bool s_is_loading_config = false;

static void save_khcom_config_impl() {
    std::ofstream file("khcom_config.ini");
    if (!file.is_open()) return;

    auto& dsp = AudioDsp::instance();
    auto& hud = PerfHud::instance();
    auto& interp = FrameInterpolator::instance();
    auto& filters = ScreenFilters::instance();
    auto& xbrz = XbrzUpscaler::instance();
    auto& input = InputEnhancements::instance();
    auto& hd_audio = HdAudioPlayer::instance();
    auto& turbo = DialogTurbo::instance();
    auto& anchoring = HudAnchoring::instance();
    auto& enhancer = DialogueEnhancer::instance();
    auto& backlog = DialogueBacklog::instance();

    file << "[Video]\n";
    file << "fps_target=" << static_cast<int>(interp.settings().mode) << "\n";
    file << "motion_smoothing=" << static_cast<int>(interp.settings().smoothing) << "\n";
    int xbrz_idx = (xbrz.scale() == XbrzScale::None) ? 0 : (static_cast<int>(xbrz.scale()) - 1);
    file << "xbrz_scale=" << xbrz_idx << "\n";
    file << "color_profile=" << static_cast<int>(filters.settings().color_profile) << "\n";
    file << "screen_mask=" << static_cast<int>(filters.settings().mask_type) << "\n";
    file << "mask_intensity=" << static_cast<int>(filters.settings().mask_intensity * 100.0f + 0.5f) << "\n";
    file << "hud_anchoring=" << static_cast<int>(anchoring.settings().mode) << "\n\n";

    file << "[Audio]\n";
    file << "hd_enabled=" << (hd_audio.settings().enabled ? 1 : 0) << "\n";
    file << "bgm_volume=" << static_cast<int>(hd_audio.settings().bgm_volume * 100.0f + 0.5f) << "\n";
    file << "sfx_volume=" << static_cast<int>(hd_audio.settings().sfx_volume * 100.0f + 0.5f) << "\n";
    file << "eq_preset=" << static_cast<int>(dsp.settings().preset) << "\n";
    file << "stereo_width=" << static_cast<int>(dsp.settings().stereo_width * 100.0f + 0.5f) << "\n";
    file << "anti_aliasing=" << (dsp.settings().anti_aliasing_filter ? 1 : 0) << "\n";
    file << "limiter=" << (dsp.settings().limiter_enabled ? 1 : 0) << "\n\n";

    file << "[Dialogue]\n";
    file << "font_density=" << static_cast<int>(enhancer.settings().density) << "\n";
    file << "font_scale=" << static_cast<int>(enhancer.settings().scale) << "\n";
    file << "font_style=" << static_cast<int>(enhancer.settings().style) << "\n";
    file << "line_capacity=" << static_cast<int>(enhancer.settings().line_capacity) << "\n";
    file << "soft_word_wrap=" << (enhancer.settings().soft_word_wrap ? 1 : 0) << "\n";
    file << "backlog_enable=" << (backlog.is_enabled() ? 1 : 0) << "\n\n";

    file << "[Controls]\n";
    file << "analog_mode=" << static_cast<int>(input.settings().analog_mode) << "\n";
    file << "analog_deadzone=" << static_cast<int>(input.settings().deadzone * 100.0f + 0.5f) << "\n";
    file << "gamepad_profile=" << static_cast<int>(input.settings().gamepad_profile) << "\n";
    file << "pad_attack=" << static_cast<int>(input.settings().pad_attack) << "\n";
    file << "pad_jump=" << static_cast<int>(input.settings().pad_jump) << "\n";
    file << "pad_deck_l=" << static_cast<int>(input.settings().pad_deck_l) << "\n";
    file << "pad_deck_r=" << static_cast<int>(input.settings().pad_deck_r) << "\n";
    file << "kb_preset=" << static_cast<int>(input.settings().kb_preset) << "\n\n";

    file << "[Gameplay]\n";
    file << "turbo_dialog=" << (turbo.is_enabled() ? 1 : 0) << "\n\n";

    file << "[Diagnostics]\n";
    file << "perf_mode=" << static_cast<int>(hud.settings().mode) << "\n";
    file << "perf_position=" << static_cast<int>(hud.settings().position) << "\n";
    file << "perf_theme=" << static_cast<int>(hud.settings().theme) << "\n";
    file << "hud_custom_x=" << static_cast<int>(hud.settings().custom_x) << "\n";
    file << "hud_custom_y=" << static_cast<int>(hud.settings().custom_y) << "\n";
}

int ui_set_callback(const char* key, int value) {
    if (!key) return 0;
    auto& dsp = AudioDsp::instance();
    auto& hud = PerfHud::instance();
    auto& interp = FrameInterpolator::instance();
    auto& filters = ScreenFilters::instance();
    auto& xbrz = XbrzUpscaler::instance();
    auto& input = InputEnhancements::instance();
    auto& hd_audio = HdAudioPlayer::instance();
    auto& turbo = DialogTurbo::instance();
    auto& anchoring = HudAnchoring::instance();
    auto& enhancer = DialogueEnhancer::instance();
    auto& backlog = DialogueBacklog::instance();

    bool handled = false;
    if (std::strcmp(key, "dialogue.font_density") == 0) {
        enhancer.set_density(static_cast<FontDensity>(value));
        handled = true;
    } else if (std::strcmp(key, "dialogue.font_scale") == 0) {
        enhancer.set_font_scale(static_cast<FontScale>(value));
        handled = true;
    } else if (std::strcmp(key, "dialogue.font_style") == 0) {
        enhancer.set_font_style(static_cast<FontStyle>(value));
        handled = true;
    } else if (std::strcmp(key, "dialogue.line_capacity") == 0) {
        enhancer.set_line_capacity(static_cast<LineCapacityMode>(value));
        handled = true;
    } else if (std::strcmp(key, "dialogue.soft_word_wrap") == 0) {
        enhancer.set_soft_word_wrap(value != 0);
        handled = true;
    } else if (std::strcmp(key, "dialogue.backlog_enable") == 0) {
        backlog.set_enabled(value != 0);
        handled = true;
    } else if (std::strcmp(key, "video.fps_target") == 0) {
        interp.set_mode(static_cast<FramerateMode>(value));
        handled = true;
    } else if (std::strcmp(key, "video.motion_smoothing") == 0) {
        interp.set_smoothing(static_cast<MotionSmoothingMode>(value));
        handled = true;
    } else if (std::strcmp(key, "video.xbrz_scale") == 0) {
        XbrzScale scale = (value <= 0) ? XbrzScale::None : static_cast<XbrzScale>(value + 1);
        xbrz.set_scale(scale);
        handled = true;
    } else if (std::strcmp(key, "video.color_profile") == 0) {
        filters.set_color_profile(static_cast<ColorProfile>(value));
        handled = true;
    } else if (std::strcmp(key, "video.screen_mask") == 0) {
        filters.set_mask_type(static_cast<ScreenMaskType>(value));
        handled = true;
    } else if (std::strcmp(key, "video.mask_intensity") == 0) {
        filters.set_mask_intensity(static_cast<float>(value) / 100.0f);
        handled = true;
    } else if (std::strcmp(key, "video.hud_anchoring") == 0) {
        anchoring.set_mode(static_cast<HudAnchorMode>(value));
        handled = true;
    } else if (std::strcmp(key, "input.analog_mode") == 0) {
        input.set_analog_mode(static_cast<AnalogMode>(value));
        handled = true;
    } else if (std::strcmp(key, "input.deadzone") == 0) {
        input.set_deadzone(static_cast<float>(value) / 100.0f);
        handled = true;
    } else if (std::strcmp(key, "input.gamepad_profile") == 0) {
        input.apply_gamepad_profile(static_cast<GamepadProfile>(value));
        handled = true;
    } else if (std::strcmp(key, "input.pad_attack") == 0) {
        input.set_pad_attack(value);
        handled = true;
    } else if (std::strcmp(key, "input.pad_jump") == 0) {
        input.set_pad_jump(value);
        handled = true;
    } else if (std::strcmp(key, "input.pad_deck_l") == 0) {
        input.set_pad_deck_l(value);
        handled = true;
    } else if (std::strcmp(key, "input.pad_deck_r") == 0) {
        input.set_pad_deck_r(value);
        handled = true;
    } else if (std::strcmp(key, "input.kb_preset") == 0) {
        input.apply_keyboard_preset(static_cast<KeyboardPreset>(value));
        handled = true;
    } else if (std::strcmp(key, "gameplay.turbo_dialog") == 0) {
        turbo.set_enabled(value != 0);
        handled = true;
    } else if (std::strcmp(key, "audio.hd_enabled") == 0) {
        hd_audio.set_enabled(value != 0);
        handled = true;
    } else if (std::strcmp(key, "audio.bgm_volume") == 0) {
        hd_audio.set_bgm_volume(static_cast<float>(value) / 100.0f);
        handled = true;
    } else if (std::strcmp(key, "audio.sfx_volume") == 0) {
        hd_audio.set_sfx_volume(static_cast<float>(value) / 100.0f);
        handled = true;
    } else if (std::strcmp(key, "perf.mode") == 0) {
        hud.set_mode(static_cast<PerfHudMode>(value));
        handled = true;
    } else if (std::strcmp(key, "perf.position") == 0) {
        hud.set_position(static_cast<PerfHudPosition>(value));
        handled = true;
    } else if (std::strcmp(key, "perf.theme") == 0) {
        hud.set_theme(static_cast<PerfHudTheme>(value));
        handled = true;
    } else if (std::strcmp(key, "audio.eq_preset") == 0) {
        dsp.apply_preset(static_cast<EqPreset>(value));
        handled = true;
    } else if (std::strcmp(key, "audio.stereo_width") == 0) {
        dsp.set_stereo_width(static_cast<float>(value) / 100.0f);
        handled = true;
    } else if (std::strcmp(key, "audio.anti_aliasing") == 0) {
        dsp.set_anti_aliasing(value != 0);
        handled = true;
    } else if (std::strcmp(key, "audio.limiter") == 0) {
        dsp.set_limiter(value != 0);
        handled = true;
    }

    if (handled) {
        if (!s_is_loading_config) {
            save_khcom_config();
        }
        return 1;
    }
    return 0;
}

static void load_khcom_config_impl() {
    auto& input = InputEnhancements::instance();
    input.load_keybinds();

    std::ifstream file("khcom_config.ini");
    if (!file.is_open()) return;

    s_is_loading_config = true;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == '#' || line[0] == ';') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        std::string val_str = line.substr(eq + 1);
        int val = 0;
        try {
            val = std::stoi(val_str);
        } catch (...) {
            continue;
        }

        if (key == "fps_target") ui_set_callback("video.fps_target", val);
        else if (key == "motion_smoothing") ui_set_callback("video.motion_smoothing", val);
        else if (key == "xbrz_scale") ui_set_callback("video.xbrz_scale", val);
        else if (key == "color_profile") ui_set_callback("video.color_profile", val);
        else if (key == "screen_mask") ui_set_callback("video.screen_mask", val);
        else if (key == "mask_intensity") ui_set_callback("video.mask_intensity", val);
        else if (key == "hud_anchoring") ui_set_callback("video.hud_anchoring", val);
        else if (key == "hd_enabled") ui_set_callback("audio.hd_enabled", val);
        else if (key == "bgm_volume") ui_set_callback("audio.bgm_volume", val);
        else if (key == "sfx_volume") ui_set_callback("audio.sfx_volume", val);
        else if (key == "eq_preset") ui_set_callback("audio.eq_preset", val);
        else if (key == "stereo_width") ui_set_callback("audio.stereo_width", val);
        else if (key == "anti_aliasing") ui_set_callback("audio.anti_aliasing", val);
        else if (key == "limiter") ui_set_callback("audio.limiter", val);
        else if (key == "font_density") ui_set_callback("dialogue.font_density", val);
        else if (key == "font_scale") ui_set_callback("dialogue.font_scale", val);
        else if (key == "font_style") ui_set_callback("dialogue.font_style", val);
        else if (key == "line_capacity") ui_set_callback("dialogue.line_capacity", val);
        else if (key == "soft_word_wrap") ui_set_callback("dialogue.soft_word_wrap", val);
        else if (key == "backlog_enable") ui_set_callback("dialogue.backlog_enable", val);
        else if (key == "analog_mode") ui_set_callback("input.analog_mode", val);
        else if (key == "analog_deadzone") ui_set_callback("input.deadzone", val);
        else if (key == "gamepad_profile") ui_set_callback("input.gamepad_profile", val);
        else if (key == "pad_attack") ui_set_callback("input.pad_attack", val);
        else if (key == "pad_jump") ui_set_callback("input.pad_jump", val);
        else if (key == "pad_deck_l") ui_set_callback("input.pad_deck_l", val);
        else if (key == "pad_deck_r") ui_set_callback("input.pad_deck_r", val);
        else if (key == "kb_preset") ui_set_callback("input.kb_preset", val);
        else if (key == "turbo_dialog") ui_set_callback("gameplay.turbo_dialog", val);
        else if (key == "perf_mode") ui_set_callback("perf.mode", val);
        else if (key == "perf_position") ui_set_callback("perf.position", val);
        else if (key == "perf_theme") ui_set_callback("perf.theme", val);
        else if (key == "hud_custom_x") PerfHud::instance().settings().custom_x = static_cast<float>(val);
        else if (key == "hud_custom_y") PerfHud::instance().settings().custom_y = static_cast<float>(val);
    }
    s_is_loading_config = false;
}
#endif

} // namespace

void save_khcom_config() {
#if defined(GBARECOMP_RUNTIME_UI)
    save_khcom_config_impl();
#endif
}

void load_khcom_config() {
#if defined(GBARECOMP_RUNTIME_UI)
    load_khcom_config_impl();
#endif
}

#if defined(GBARECOMP_RUNTIME_UI)
static RecompRuntimeUi* s_active_runtime_ui = nullptr;

bool is_runtime_menu_open() {
    return s_active_runtime_ui && recomp_runtime_ui_is_open(s_active_runtime_ui);
}

void open_runtime_menu() {
    if (s_active_runtime_ui && !recomp_runtime_ui_is_open(s_active_runtime_ui)) {
        recomp_runtime_ui_open(s_active_runtime_ui);
    }
}

void close_runtime_menu() {
    if (s_active_runtime_ui && recomp_runtime_ui_is_open(s_active_runtime_ui)) {
        recomp_runtime_ui_close(s_active_runtime_ui);
    }
}

void toggle_runtime_menu() {
    if (s_active_runtime_ui) {
        if (recomp_runtime_ui_is_open(s_active_runtime_ui)) {
            recomp_runtime_ui_close(s_active_runtime_ui);
        } else {
            recomp_runtime_ui_open(s_active_runtime_ui);
        }
    }
}
#else
bool is_runtime_menu_open() {
    return false;
}
void open_runtime_menu() {}
void close_runtime_menu() {}
void toggle_runtime_menu() {}
#endif

gbarecomp::RunOptions create_run_options() {
    gbarecomp::RunOptions opts;
    opts.builtin_game_name = GAME_TITLE.data();
    opts.builtin_rom_sha1 = ROM_SHA1_USA.data();
    opts.launcher_region = "USA";
    opts.launcher_game_config = "game.toml";

    // Assist tools: save states, fast-forward, and rewind
    opts.expose_assist_tools = true;
    opts.assist_tools_enabled_by_default = true;
    opts.assist_fast_forward_multiplier_default = 4;
    opts.save_state_slot_count = 10;
    opts.rewind_history_seconds = 60;
    opts.rewind_capture_interval_frames = 15;

    // Video display and viewport configuration:
    // Fixed 16:9 widescreen active ONLY during battle arenas; authentic 3:2 elsewhere
    opts.freely_resizable_window = true;
    opts.resize_driven_view = false;
    opts.max_view_width = 284;
    opts.max_resize_view_width = 284;
    opts.max_resize_view_height = 160;
    opts.launcher_expose_widescreen = true;
    opts.launcher_expose_adaptive_view = false;
    opts.widescreen_view_width = 284;
    opts.launcher_aspect_labels = kAspectLabels;
    opts.launcher_aspect_view_widths = kAspectWidths;
    opts.launcher_num_aspects = sizeof(kAspectWidths) / sizeof(kAspectWidths[0]);
    opts.launcher_expose_sharp_filter = true;
    opts.launcher_default_sharp_filter = true;
    opts.extended_view_init = &khcom_install_widescreen_adapter;
    opts.extended_view_frame = [](const gbarecomp::ExtendedViewFrameInfo*) {
        khcom_update_widescreen_state();
        auto* bus = gbarecomp::active_bus();
        if (bus) {
            uint16_t host = bus->io().host_keyinput();
            uint16_t modified = khcom::InputEnhancements::instance().process_keyinput(host);
            bus->io().set_keyinput(modified);
        }
    };

    // Immediately install widescreen hooks so battle state tracking is active from frame 0
    khcom_install_widescreen_adapter(22, 22);

#if defined(GBARECOMP_RUNTIME_UI)
    opts.ui_extra_items = kExtraItems;
    opts.ui_extra_item_count = sizeof(kExtraItems) / sizeof(kExtraItems[0]);
    opts.ui_get = ui_get_callback;
    opts.ui_set = ui_set_callback;
    opts.ui_action = ui_action_callback;
    opts.ui_enabled = ui_enabled_callback;
    opts.ui_get_text = ui_get_text_callback;
    opts.ui_set_text = ui_set_text_callback;

    // Restore saved user configurations across game sessions
    load_khcom_config();
#endif

    return opts;
}

} // namespace khcom

#if defined(GBARECOMP_RUNTIME_UI) && (defined(__GNUC__) || defined(__clang__))
extern "C" {
RecompRuntimeUi* __real_recomp_runtime_ui_create_standard(const RecompRuntimeUiStandardConfig* standard);
RecompRuntimeUi* __wrap_recomp_runtime_ui_create_standard(const RecompRuntimeUiStandardConfig* standard) {
    if (!standard) return nullptr;
    RecompRuntimeUiStandardConfig cfg = *standard;
    cfg.features &= ~static_cast<uint64_t>(RECOMP_RUNTIME_UI_STANDARD_RESUME);
    cfg.view_modes = RECOMP_RUNTIME_UI_VIEW_MODE_NATIVE | RECOMP_RUNTIME_UI_VIEW_MODE_FIXED_16_9;
    cfg.native_view_label = "3:2 (Native)";
    cfg.fixed_view_label = "16:9 (Battles only)";
    khcom::s_active_runtime_ui = __real_recomp_runtime_ui_create_standard(&cfg);
    return khcom::s_active_runtime_ui;
}
}
#endif
