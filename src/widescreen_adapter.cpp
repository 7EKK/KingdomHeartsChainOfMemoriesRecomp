#include "widescreen_adapter.h"
#include "hud_anchoring.h"
#include <algorithm>

extern "C" {
extern int (*g_ws_tilemap_provider)(int bg, int hw_x, int screen_y, uint16_t* out_entry);
extern int (*g_ws_bg_x_provider)(int bg, int output_x, int screen_y, int* out_hw_x);
extern unsigned g_ws_bg_x_provider_layers;
extern int (*g_ws_obj_attr_x_provider)(int oam_index, uint16_t attr0, uint16_t attr1, uint16_t attr2, int* out_x);
extern int g_ws_authored_margin_layers;
extern int g_ws_obj_native_clip;
extern int g_ws_pillarbox;
extern int g_ws_pillarbox_left;
extern int g_ws_pillarbox_right;
extern unsigned g_ws_active;
extern unsigned g_ws_extra;
extern unsigned g_ws_extra_left;
extern unsigned g_ws_extra_right;
extern unsigned g_ws_view_width;
extern void (*g_runtime_fn_entry_hook)(uint32_t entry_pc);
}

namespace {
constexpr int kWsTilemapKeepWrapped = 2;
bool s_is_menu = true;
bool s_is_battle = false;
void (*s_prev_fn_entry_hook)(uint32_t entry_pc) = nullptr;

void khcom_fn_entry_hook(uint32_t pc) {
    if (s_prev_fn_entry_hook) {
        s_prev_fn_entry_hook(pc);
    }

    // Battle / combat modes: 16:9 True Widescreen
    const bool is_battle_entry =
        (pc >= 0x080099D0u && pc <= 0x0800A5A3u) || // mode_battle_0, mode_battle_1
        (pc >= 0x0800A608u && pc <= 0x0800AB7Fu) || // mode_chkbtl_0, mode_chkbtl_1
        (pc >= 0x0800C428u && pc <= 0x0800C677u) || // mode_vsbattle_0, mode_vsbattle_1
        (pc >= 0x0801D228u && pc <= 0x08031FFFu) || // task_btl_* routines
        (pc >= 0x0803FE74u && pc <= 0x0805DA34u) || // task_btl_*, task_smn_*, task_frd_*, task_hum_*
        (pc >= 0x02038738u && pc <= 0x0203875Au) || // RAM overlay battle routines
        (pc >= 0x03000000u && pc <= 0x03007000u);   // RAM overlay battle routines in IWRAM

    // Non-battle modes: Authentic 3:2 view (Pillarboxed)
    const bool is_non_battle_entry =
        (pc == 0x0800A5A4u) ||                       // mode_battle_2 (battle cleanup/exit)
        (pc == 0x0800AB80u) ||                       // mode_chkbtl_2 (battle check exit)
        (pc == 0x0800C678u) ||                       // mode_vsbattle_2 (vs battle exit)
        (pc >= 0x08032070u && pc <= 0x08036BEEu) || // task_fld_* (Field Sora, Riku, Overworld/Rooms)
        (pc >= 0x0803F254u && pc <= 0x0803FDFBu) || // task_roomcreate_*, task_romcri_eff_* (World Room creation)
        (pc >= 0x0805ACE4u && pc <= 0x0805C910u) || // mode_jiminy_* (Jiminy's Journal)
        (pc >= 0x0805E8B4u && pc <= 0x0805F0D8u) || // mode_movie_* (Cutscenes / FMV)
        (pc >= 0x080D31C4u && pc <= 0x080D5B6Fu) || // mode_allmap_*, task_allmap_* (World Map)
        (pc >= 0x080D5B70u && pc <= 0x080D7013u) || // mode_title_* (Title Screen)
        (pc >= 0x080D7014u && pc <= 0x080D740Fu) || // mode_copyright1_*, mode_copyright2_* (Copyright screens)
        (pc >= 0x080D7410u && pc <= 0x080DAC00u) || // mode_status_* (Pause / Status / Deck Menu)
        (pc >= 0x080FFC0Cu && pc <= 0x08109BF0u);   // World inspection & menu modes

    if (is_battle_entry) {
        s_is_battle = true;
        s_is_menu = false;
        if (g_ws_active && g_ws_view_width > 240) {
            g_ws_pillarbox = 0; // True widescreen in battle arena
        }
    } else if (is_non_battle_entry) {
        s_is_battle = false;
        s_is_menu = true;
        g_ws_pillarbox = 1; // Authentic 3:2 pillarboxed view for field, menus, cutscenes
    }
}
} // namespace

int khcom_tilemap_provider(int bg, int hw_x, int screen_y, uint16_t* out_entry) {
    (void)bg;
    (void)hw_x;
    (void)screen_y;
    (void)out_entry;
    if (!s_is_battle) {
        return 0;
    }
    // Accept authentic VRAM tilemap data beyond 240px for true widescreen in battle
    return kWsTilemapKeepWrapped;
}

int khcom_bg_x_provider(int bg, int output_x, int screen_y, int* out_hw_x) {
    if (!out_hw_x || !g_ws_active) {
        return 0;
    }

    const int left = static_cast<int>(g_ws_extra_left);
    const int right = static_cast<int>(g_ws_extra_right);
    if (left == 0 && right == 0) {
        return 0;
    }

    const int extra = left + right;
    const bool in_left_margin = (output_x < left);
    const bool in_right_margin = (output_x >= left + 240);
    const bool in_margin = in_left_margin || in_right_margin;

    // When outside of battle, all margin columns are pillarboxed in 3:2
    if (!s_is_battle) {
        if (in_margin) {
            return -1;
        }
        return 0;
    }

    if (bg == 0) {
        // Dialogue and message text boxes stay centered in 240px; never bleed into margins
        if (in_margin) {
            return -1;
        }
        return 0;
    }

    if (bg == 1) {
        // UI layer: in battle only, if WidescreenAnchored is active, sample corners into margins
        const auto anchor_mode = khcom::HudAnchoring::instance().settings().mode;
        if (!s_is_menu && s_is_battle && anchor_mode == khcom::HudAnchorMode::WidescreenAnchored) {
            if (in_left_margin && screen_y < 48) {
                *out_hw_x = std::clamp(output_x, 0, 80);
                return 1;
            }
            if (in_right_margin && screen_y >= 96) {
                *out_hw_x = std::clamp(output_x - extra, 140, 239);
                return 1;
            }
        }

        // Never bleed dialogue or HUD text into margins, but never punch holes inside native screen
        if (in_margin) {
            return -1;
        }
        return 0;
    }

    if (bg == 2 || bg == 3) {
        // Authentic battle arena background layers render across the full widescreen view
        return 0;
    }

    return 0;
}

int khcom_obj_attr_x_provider(int oam_index, uint16_t attr0, uint16_t attr1, uint16_t attr2, int* out_x) {
    (void)oam_index;
    (void)attr2;
    if (!out_x || !g_ws_active) {
        return 0;
    }

    const int raw_x = static_cast<int>(attr1 & 0x1FFu);
    const int sx = (raw_x >= 256) ? (raw_x - 512) : raw_x;

    // Never displace sprites outside of battle
    if (!s_is_battle) {
        if (raw_x >= 256) {
            *out_x = sx;
            return 1;
        }
        return 0;
    }

    // Never displace sprites in menus, cutscenes, or when HUD anchoring is disabled
    const auto anchor_mode = khcom::HudAnchoring::instance().settings().mode;
    if (!s_is_menu && s_is_battle && anchor_mode == khcom::HudAnchorMode::WidescreenAnchored) {
        const int raw_y = static_cast<int>(attr0 & 0xFFu);
        const int sy = (raw_y >= 160) ? (raw_y - 256) : raw_y;
        if (sx >= -16 && sx <= 85 && sy >= 0 && sy <= 45) {
            *out_x = sx - static_cast<int>(g_ws_extra_left);
            return 1;
        }
        if (sx >= 130 && sx <= 250 && sy >= 90 && sy <= 160) {
            *out_x = sx + static_cast<int>(g_ws_extra_right);
            return 1;
        }
    }

    if (raw_x >= 256) {
        *out_x = sx;
        return 1;
    }

    return 0;
}

void khcom_install_widescreen_adapter(uint32_t extra_left, uint32_t extra_right) {
    (void)extra_left;
    (void)extra_right;
    g_ws_tilemap_provider = &khcom_tilemap_provider;
    g_ws_bg_x_provider = &khcom_bg_x_provider;
    g_ws_bg_x_provider_layers = 0xFu;
    g_ws_obj_attr_x_provider = &khcom_obj_attr_x_provider;
    g_ws_authored_margin_layers = 1;
    g_ws_obj_native_clip = 0;
    g_ws_pillarbox = (s_is_battle && g_ws_active && g_ws_view_width > 240) ? 0 : 1;

    if (g_runtime_fn_entry_hook != &khcom_fn_entry_hook) {
        s_prev_fn_entry_hook = g_runtime_fn_entry_hook;
        g_runtime_fn_entry_hook = &khcom_fn_entry_hook;
    }
}

void khcom_update_widescreen_state() {
    g_ws_tilemap_provider = &khcom_tilemap_provider;
    g_ws_bg_x_provider = &khcom_bg_x_provider;
    g_ws_bg_x_provider_layers = 0xFu;
    g_ws_obj_attr_x_provider = &khcom_obj_attr_x_provider;
    g_ws_authored_margin_layers = 1;
    g_ws_obj_native_clip = 0;
    g_ws_pillarbox = (s_is_battle && g_ws_active && g_ws_view_width > 240) ? 0 : 1;
}
