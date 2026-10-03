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
uint64_t s_frame_counter = 0;
uint64_t s_last_battle_frame = 0;
uint64_t s_last_non_battle_frame = 0;
void (*s_prev_fn_entry_hook)(uint32_t entry_pc) = nullptr;

void khcom_fn_entry_hook(uint32_t pc) {
    if (s_prev_fn_entry_hook) {
        s_prev_fn_entry_hook(pc);
    }

    // Battle / combat modes: 16:9 True Widescreen
    const bool is_battle_entry =
        (pc >= 0x080099D0u && pc < 0x0800A5A4u) ||  // mode_battle_0, mode_battle_1
        (pc >= 0x0800A608u && pc < 0x0800AB80u) ||  // mode_chkbtl_0, mode_chkbtl_1
        (pc >= 0x0800C428u && pc < 0x0800C678u) ||  // mode_vsbattle_0, mode_vsbattle_1
        (pc >= 0x0801D228u && pc <= 0x08031B64u) || // task_btl_* routines
        (pc >= 0x0803FDFCu && pc <= 0x08040C70u) || // task_btl_form_*, btl_born, btl_raid, btl_badstatus
        (pc >= 0x08040E28u && pc <= 0x08045404u) || // task_smn_* (Battle summons: Cloud, Simba, etc.)
        (pc >= 0x08045550u && pc <= 0x08049D88u) || // task_frd_* (Battle friend cards: Donald, Goofy, etc.)
        (pc >= 0x0804A014u && pc <= 0x08059DD0u) || // task_hum_* (Battle humanoid bosses: Axel, Larxene, etc.)
        (pc >= 0x0805CC80u && pc <= 0x0805DA34u) || // task_btl_pop_cb_*, btl_exp, btl_vslockon, btl_hpoth
        (pc >= 0x080B9660u && pc <= 0x080BCE78u) || // task_bos_tm_* (Trickmaster boss)
        (pc >= 0x080BCE8Cu && pc <= 0x080C1A30u) || // task_bos_jf_* (Jafar boss)
        (pc >= 0x080C1B60u && pc <= 0x080C51C0u) || // task_bos_dsd_* (Darkside boss)
        (pc >= 0x080D9B90u && pc <= 0x080DB954u) || // task_bos_boogie_* (Oogie Boogie boss)
        (pc >= 0x080DC640u && pc <= 0x080DDDD8u) || // task_bos_ursula_* (Ursula boss)
        (pc >= 0x080FB608u && pc <= 0x080FB890u) || // task_bos_ga_* (Guard Armor boss)
        (pc >= 0x080FC444u && pc <= 0x080FD9A0u) || // task_bos_md_* (Maleficent Dragon boss)
        (pc >= 0x0810A580u && pc <= 0x0810C29Cu) || // task_bos_pc_* (Parasite Cage boss)
        (pc >= 0x0810FF7Cu && pc <= 0x0811259Cu) || // task_bos_lst_* (Marluxia final boss)
        (pc >= 0x080AEBA0u && pc <= 0x080B1844u) || // mode_sio_btl_* (Multiplayer battle)
        (pc >= 0x02038738u && pc <= 0x0203875Au) || // RAM overlay battle routines
        (pc >= 0x03000000u && pc <= 0x03007000u);   // RAM overlay battle routines in IWRAM

    // Non-battle modes: Authentic 3:2 view (Pillarboxed)
    const bool is_non_battle_entry =
        (pc == 0x0800A5A4u) ||                       // mode_battle_2 (battle cleanup/exit)
        (pc == 0x0800AB80u) ||                       // mode_chkbtl_2 (battle check exit)
        (pc == 0x0800C678u) ||                       // mode_vsbattle_2 (vs battle exit)
        (pc >= 0x08032070u && pc <= 0x08036BE4u) || // task_fld_* (Field Sora, Riku, Overworld/Rooms)
        (pc >= 0x08036BFCu && pc <= 0x0803F008u) || // task_emy_* (Field enemies wandering)
        (pc >= 0x0803F254u && pc <= 0x0803FDBCu) || // task_roomcreate_*, task_romcri_eff_* (World Room creation)
        (pc >= 0x0805ACE4u && pc <= 0x0805C910u) || // mode_jiminy_* (Jiminy's Journal)
        (pc >= 0x0805DB38u && pc <= 0x0805E824u) || // task_tutorial_* (Tutorial prompts)
        (pc >= 0x0805E8B4u && pc <= 0x0805F0D8u) || // mode_movie_* (Cutscenes / FMV)
        (pc >= 0x080750D4u && pc <= 0x08075354u) || // mode_eventselect_*
        (pc >= 0x080B3F80u && pc <= 0x080B7E50u) || // mode_wLogo_*, task_wLogo_* (World Logo intro)
        (pc >= 0x080C85C4u && pc <= 0x080D2BC4u) || // task_poo_* (100 Acre Wood mini-games)
        (pc >= 0x080D31C4u && pc <= 0x080D4CF8u) || // mode_allmap_*, task_allmap_* (World Map)
        (pc >= 0x080D5B70u && pc <= 0x080D6FFCu) || // mode_title_*, task_title_* (Title Screen)
        (pc >= 0x080D7014u && pc <= 0x080D7288u) || // mode_copyright1_*, mode_copyright2_* (Copyright screens)
        (pc >= 0x080D7410u && pc <= 0x080D8B54u) || // mode_status_*, task_status_* (Pause / Status / Deck Menu)
        (pc >= 0x080F7BF8u && pc <= 0x080F7DB0u) || // task_room_name_* (Room name display)
        (pc >= 0x080FFC0Cu && pc <= 0x08109AA0u) || // mode_worldinspect_*, mode_ms_* (Card shop & inspection)
        (pc >= 0x08112B3Cu && pc <= 0x08115484u);   // mode_StaffRoll_*, task_sroll_* (Credits)

    if (is_battle_entry) {
        s_is_battle = true;
        s_is_menu = false;
        s_last_battle_frame = s_frame_counter;
        if (g_ws_active && g_ws_view_width > 240) {
            g_ws_pillarbox = 0; // True widescreen in battle arena
        }
    } else if (is_non_battle_entry) {
        s_is_battle = false;
        s_is_menu = true;
        s_last_non_battle_frame = s_frame_counter;
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

bool khcom_is_battle_active() {
    // If battle state is true, but no battle PC has been observed for over 30 frames
    // while non-battle PCs have been observed, auto-fall back to false (clean heartbeat).
    if (s_is_battle && s_frame_counter > s_last_battle_frame + 30 &&
        s_last_non_battle_frame > s_last_battle_frame) {
        s_is_battle = false;
        s_is_menu = true;
        g_ws_pillarbox = 1;
    }
    return s_is_battle;
}

void khcom_set_battle_active(bool active) {
    s_is_battle = active;
    s_is_menu = !active;
    if (active) {
        s_last_battle_frame = s_frame_counter;
        if (g_ws_active && g_ws_view_width > 240) {
            g_ws_pillarbox = 0;
        }
    } else {
        s_last_non_battle_frame = s_frame_counter;
        g_ws_pillarbox = 1;
    }
}

void khcom_widescreen_notify_present() {
    ++s_frame_counter;
}

void khcom_compute_effective_viewport(int tex_w, int tex_h, int win_w, int win_h,
                                      SDL_Rect* out_src, SDL_Rect* out_dst,
                                      int* out_base_w, int* out_base_h) {
    if (!out_src || !out_dst || tex_w <= 0 || tex_h <= 0 || win_w <= 0 || win_h <= 0) return;

    const bool is_284 = (tex_w % 284 == 0) && (tex_h % 160 == 0) && (tex_w / 284 == tex_h / 160);
    const int factor = is_284 ? (tex_w / 284) : (tex_h / 160 > 0 ? (tex_h / 160) : 1);
    const bool in_battle = khcom_is_battle_active();

    if (is_284 && in_battle) {
        // Mode 16:9 during battle arena: render full wide texture into 16:9 layout
        out_src->x = 0;
        out_src->y = 0;
        out_src->w = tex_w;
        out_src->h = tex_h;

        if (out_base_w) *out_base_w = 284;
        if (out_base_h) *out_base_h = 160;

        float s = std::min(static_cast<float>(win_w) / 284.0f, static_cast<float>(win_h) / 160.0f);
        int dw = static_cast<int>(284.0f * s);
        int dh = static_cast<int>(160.0f * s);
        out_dst->x = (win_w - dw) / 2;
        out_dst->y = (win_h - dh) / 2;
        out_dst->w = dw;
        out_dst->h = dh;
    } else if (is_284 && !in_battle) {
        // Mode 16:9 outside battle: crop central authentic 240x160 area and fit to 3:2 layout
        out_src->x = 22 * factor;
        out_src->y = 0;
        out_src->w = 240 * factor;
        out_src->h = 160 * factor;

        if (out_base_w) *out_base_w = 240;
        if (out_base_h) *out_base_h = 160;

        float s = std::min(static_cast<float>(win_w) / 240.0f, static_cast<float>(win_h) / 160.0f);
        int dw = static_cast<int>(240.0f * s);
        int dh = static_cast<int>(160.0f * s);
        out_dst->x = (win_w - dw) / 2;
        out_dst->y = (win_h - dh) / 2;
        out_dst->w = dw;
        out_dst->h = dh;
    } else {
        // Mode Native (3:2) everywhere
        out_src->x = 0;
        out_src->y = 0;
        out_src->w = tex_w;
        out_src->h = tex_h;

        if (out_base_w) *out_base_w = 240;
        if (out_base_h) *out_base_h = 160;

        float s = std::min(static_cast<float>(win_w) / 240.0f, static_cast<float>(win_h) / 160.0f);
        int dw = static_cast<int>(240.0f * s);
        int dh = static_cast<int>(160.0f * s);
        out_dst->x = (win_w - dw) / 2;
        out_dst->y = (win_h - dh) / 2;
        out_dst->w = dw;
        out_dst->h = dh;
    }
}

