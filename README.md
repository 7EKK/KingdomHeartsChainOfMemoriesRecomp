# KHCOMRecomp — Kingdom Hearts: Chain of Memories (GBA) Static Recompilation

[![Project Status: Beta](https://img.shields.io/badge/Status-Beta%20%2F%20Work%20in%20Progress-orange?style=flat&logo=git)](https://github.com/rafadono/KingdomHeartsChainOfMemoriesRecomp)
[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Support%20Project-FF5E5B?style=flat&logo=kofi&logoColor=white)](https://ko-fi.com/rafadono)

> [!NOTE]
> **Project Status: Beta / Work-in-Progress (Not a Final Release)**
> This project is currently in **Beta / Active Development** and is **not a final finished product**. 
> While core gameplay, 16:9 widescreen battle arena rendering, audio DSP, dialogue backlog/enhancements, and the in-game configuration overlay are functional and backed by automated test suites, features and enhancements are actively being refined, tuned, and tested. Community feedback and bug reports are welcome!

Native static recompilation of **Kingdom Hearts: Chain of Memories** for Game Boy Advance to modern PC platforms, built on the `gbarecomp` framework.

---

## ROM and BIOS Requirements

### Game ROM
The official target version for this project is the **USA release**:

- **Game Title / ID:** `B8CE`
- **Region:** USA (NTSC-U, 60 Hz)
- **SHA-1 Hash:** `10729bd884f8fdca7a310b6d606c52e46657aa48`
- **Size:** 33,554,432 bytes (32 MB / 256 Mbit)
- **Save Type:** EEPROM (8 KB / 64 Kbit)
- **Location:** Place your ROM file in `roms/B8CE.gba` (or any `.gba` file inside `roms/`).

To verify your ROM file:
```powershell
.\build.ps1 -Target verify_rom_hash
.\build\Release\verify_rom_hash.exe "roms\Kingdom Hearts - Chain of Memories (U)(Venom).gba"
```

### GBA BIOS
For authentic hardware execution of BIOS SWI calls and interrupt vectors:

- **File:** `bios/gba_bios.bin` (or `bios/gba (W).gba`)
- **Size:** 16,384 bytes (16 KB)
- **SHA-1 Hash:** `300c20df6731a33952ded8c436f7f186d25d3492`

---

## In-Game Configuration Overlay

The application features a real-time in-game configuration overlay built using `recomp-ui`. Open or close the overlay at any time during gameplay without pausing or restarting:

- **Keyboard:** Press `Escape`
- **Gamepad / Controller:** Press `Guide` / `Home` / `Menu` / `Options`

All adjustments made in the overlay take effect immediately in real time and are persisted across sessions in `config/khcom_settings.ini`.
Available sections include **Display**, **Graphics**, **Audio**, **Dialogue & Story**, **Controller (Gamepad)**, **Keyboard Controls**, **Gameplay & Assist**, **Diagnostics**, and **Assist Tools**.

---

## Key Features and Enhancements

### 1. High Refresh Rate Presentation Engine
- **Target Framerates:** `60 FPS (Original)`, `120 FPS (2x High Rate)`, `144 FPS`, `Display Native (Auto)`, and `Uncapped`.
- **Decoupled Simulation:** Game simulation ticks remain locked to authentic 59.7275 Hz GBA hardware cadence, keeping audio pitch, battle card cooldowns, and physics 100% stable without game acceleration.
- **GPU Temporal Motion Smoothing:** GPU-accelerated blending between simulation frames, eliminating judder and strobing during camera scrolling and card combat.

### 2. Authentic MiSTer FPGA Screen Filters
- **Hardware Color Profiles:**
  - `Raw (Uncorrected)`: Pure digital saturated RGB.
  - `AGB-001 (Reflective TFT)`: Desaturated, gamma 1.45 curve tailored to correct GBA games designed for unlit reflective screens.
  - `AGS-001 (SP Frontlit)`: Frontlit reflective simulation with subtle cool white point.
  - `AGS-101 (SP Backlit)`: High-contrast, rich-black backlit model (gamma 2.2).
  - `MiSTer Gamma 1.6 & 2.2`: Exact mathematical gamma curves from the MiSTer GBA core.
- **Screen Masks (Direct3D11 Multiplicative Blending):**
  - `MiSTer LCD Grid`: Inactive matrix grid between TFT liquid crystals.
  - `MiSTer Subpixel RGB`: Vertical Red, Green, Blue subpixel stripes.
  - `MiSTer Subpixel BGR`: Alternate subpixel striping.
  - `MiSTer Diffusion (AGS-001)`: Frontlight light-guide diffusion pattern.
  - `Game Boy Player Scanlines`: 240p CRT television scanlines.
  - `CRT Trinitron Aperture Grille`: Vertical phosphor stripes and scanlines.
- **Mask Intensity Slider:** Live adjustment from 10% to 100% opacity.

### 3. xBRZ High-Precision Pixel Art Upscaler
- Geometric pattern-recognition edge interpolation engine.
- Preserves clean diagonal vectors and curves without bilinear blurring.
- Modes: `Off (Original Pixels)`, `2x xBRZ`, `3x xBRZ`, `4x xBRZ`, `5x xBRZ`.

### 4. True 360 Analog Movement & Velocity
- Circular deadzone (configurable, default 18%) with full 360-degree angle resolution.
- Walk/run speed modulation: Tilting the analog stick below 55% threshold cuts movement speed by 50% for realistic character walking, while full tilt engages running.
- Modes: `Digital (Original 8-Way)`, `Analog 8-Way (Enhanced)`, and `True 360 Walk & Run`.

### 5. HD Orchestrated Soundtrack & Independent Mixer
- Replaces compressed GBA chiptune tracks with high-fidelity orchestral arrangements from PlayStation 2 *Re:Chain of Memories*.
- Configurable track mapping via [config/music_tracks.ini](config/music_tracks.ini).
- Seamless loop playback and automatic resampling to engine 65,536 Hz 16-bit stereo.
- **Independent Volume Controls:** Separate sliders for BGM (Music) and SFX (Effects / Combat Audio) from 0% to 150%.

### 6. Dynamic Text Resizing & Dialogue Presentation Engine
- **Font Resizing & Variable-Width Spacing (VWF):** Real-time text rendering pipeline replacing fixed-width glyph advance with proportional variable-width spacing. Includes configurable font density (*Original 100%*, *Compact 80%*, and *High-Density 65%*) with glyph bounding-box caching to comfortably fit long lines without text overflow.
- **Nine-Slice Dialogue Box Scaler:** Dynamically expands dialogue frame borders and backgrounds horizontally while preserving authentic corner aesthetics and seamless transparency.
- **Grammar & Case-Aware Soft Word-Wrapping:** Intelligently analyzes punctuation, quotation marks, and casing to reflow artificial mid-sentence `\n` breaks into smooth, natural dialogue lines.
- **Preserved Typewriter Cadence:** Dialogue progresses character-by-character along with original scene bytecode events (mugshots/portraits, pauses, sound effects). No artificial full text dump.

### 7. Conversation Log Sidebar (Backlog)
- Press **`F2`** on keyboard, or Gamepad **`Back` / `Select` / `Share`** to toggle the story backlog sidebar.
- **Dynamic In-Game Capture:** Dialogue lines are recorded chronologically in real time as they appear in cutscenes, without preloaded mock entries.
- **Automatic Game Pause:** Simulation and audio pause while the backlog is open, allowing the player to review dialogue without missing story progression.
- **Left-Docked Glassmorphic Panel:** Translucent dark sidebar displaying chronological dialogue entries with speaker badges and timestamps.
- **Smooth Navigation:** Scroll up and down using the mouse wheel, keyboard arrow keys, or gamepad D-Pad.
- **Closing:** Press `Escape`, `F2`, or Gamepad `B` / `Circle` to resume gameplay.

### 8. Adaptive Widescreen Architecture & Dynamic Pillarboxing (Battle 16:9 / Overworld 3:2)
- **Event-Driven Aspect Presentation (Combat 16:9 / Non-Combat 3:2):**
  - **16:9 True Widescreen during Battles:** Combat arenas expand across modern 16:9 and 16:10 viewports without stretching or edge distortion. Background scenery layers (`BG2` and `BG3`) seamlessly render into the extended horizontal field of view, revealing the full arena width.
  - **Authentic 3:2 Pillarbox for Overworld, Menus & Cinematics:** Outside of battles—during room exploration, door synthesis, menu navigation (Deck Edit, Status, Jiminy's Journal), title screen, and story cutscenes—the game automatically displays the native 240×160 GBA presentation surrounded by clean, undistorted pillarbox black bars, preventing edge glitches or out-of-bounds tile repetition.
  - **Zero-Latency Dynamic Switching:** Subroutine entry hooks (`khcom_fn_entry_hook`) monitor game execution states in real time (`mode_battle_*`, `task_btl_*`, `task_fld_*`, `mode_status_*`, etc.) to automatically toggle between True Widescreen and 3:2 Pillarbox without frame stutter or manual user intervention.
- **PPU Rasterizer & HUD Anchoring Integration:**
  - `khcom_tilemap_provider`: Preserves authentic VRAM tilemap wrapping (`kWsTilemapKeepWrapped`) during combat for full-width arena background coverage.
  - `khcom_bg_x_provider`: Handles layer visibility. Ensures dialogue windows on BG0 remain centered over the 240px native play area without bleeding into margins, anchors BG1 HUD elements to screen edges when enabled, and blanks margin columns during non-battle sequences.
  - `khcom_obj_attr_x_provider`: Hooks OAM sprite attributes in real time. Displaces Sora's HP bar to the top-left margin (`X -= extra_left`) and Card Deck / reload counter to the bottom-right margin (`X += extra_right`) when Widescreen Anchoring is active, while unwrapping 9-bit signed coordinates so sprites do not pop on opposite screen edges.

#### Why True 16:9 is Exclusive to Combat Arenas (Technical & Design Rationale)
A frequent question is why widescreen expansion (284×160) is active exclusively during combat arenas rather than throughout the entire game. This deliberate architectural decision is grounded in the GBA hardware architecture, the internal engine design of *Kingdom Hearts: Chain of Memories*, and strict visual quality standards:

1. **Continuous Wraparound Cylindrical Arenas (Combat Only):**
   - In battle, the background environments (`BG2` and `BG3`) are authored as complete, 360-degree cylindrical scrolling tilemaps stored in VRAM (typically 512×256 pixels).
   - Because this backdrop art continually wraps around the arena, expanding the horizontal viewport from 240px to 284px reveals **genuine, pre-existing scenery** without exposing empty voids or unmapped geometry. The arena is an enclosed space where all combatants stay within natural bounds.

2. **Room Exploration & Culling Boundaries (Overworld / Field Maps):**
   - Outside of battle, rooms in Castle Oblivion are isometric environments strictly constructed to fit the original 240×160 screen boundaries.
   - The game engine relies on viewport-edge culling: Heartless enemies, destructible objects, and door synthesis portals spawn and despawn precisely at the 240px edge.
   - Expanding the horizontal viewport during field exploration exposes the **unrendered void beyond the room's walls** (uninitialized VRAM tile garbage, black voids, repeating edge seams).
   - Enemies would be seen frozen off-camera or popping into existence inside the visible margins, destroying immersion.

3. **Menu Layouts & Interface Distortion (UI / Menus):**
   - Interface screens—including the Title Screen, Jiminy's Journal, Deck Edit / Card Customization, World Map, Status screen, and Moogle Shop—are static, full-screen 240×160 pixel art compositions.
   - Forcing 16:9 would either:
     - Unevenly stretch the 240px UI horizontally (distorting square pixels, flattening circular card symbols, and producing blurry font scaling), or
     - Display awkward disjointed borders with missing edge frames.
   - Clean 3:2 pillarboxing ensures menus remain pixel-perfect, crisp, and authentic.

4. **Cinematic Scripting & Theatrical Blocking (Story Cutscenes):**
   - Story cutscenes (e.g. Castle Oblivion hallway encounters, Organization XIII dialogues, Disney world cinematics) are choreographed with fixed 240-pixel camera blocking.
   - Characters enter and exit scenes by walking just past the 240px boundary.
   - In 16:9, actors would be visible "waiting in the wings" off-stage, or abruptly vanishing once they cross the 240px threshold, breaking theatrical illusion.
   - Dialogue boxes (`BG0`) are framed to span 240px; expanding the screen would leave awkward empty side gaps or stretch character portraits.

5. **Hardware Sprite Coordinate Wrapping (9-Bit OAM Limits):**
   - The GBA PPU uses 9-bit unsigned coordinates ($0..511$) for OAM sprites.
   - In combat, `khcom_obj_attr_x_provider` specifically unwraps 9-bit coordinates for combat sprites and anchors the HP bar and Card Deck to 16:9 screen corners.
   - In the overworld, applying this would distort NPC dialogue icons, speech bubbles, and door icons.

6. **Preservation of Authentic Geometric Proportions:**
   - Unlike generic emulator widescreen hacks that stretch the 3:2 framebuffer and cause "fat Sora" and warped sprites, KHCOMRecomp adheres to **True Geometric Widescreen**: all sprites and models maintain their exact 1:1 square pixel aspect ratio at all times. Widescreen is applied only where real artwork exists to be revealed, and clean pillarboxing is applied elsewhere to preserve artistic integrity.

### 9. Native RAM Overlay Dispatch & Combat Performance Optimization
- **Binary-Search Dispatcher:** Routes all 168 dynamic IWRAM (`0x0300xxxx`) and EWRAM (`0x0203xxxx`) combat routines directly to their native recompiled implementations.
- **Full 60 FPS Combat:** Completely eliminates dynamic interpretation drops and on-the-fly JIT compilation stutters during battles, card sleights, and enemy encounters.

### 10. Performance HUD & Frametime Graph
- Real-time FPS counter with 60 FPS target indicator (`[LOCKED]` vs `[VAR]`).
- Precision frame duration in milliseconds and rolling average.
- 120-frame rolling pacing history graph with 16.67 ms (60 FPS) target line.
- Dynamic color coding (Emerald Green for 60 FPS lock, Amber Yellow for slight variance, Rose Red for frame drops).
- Movable positioning: Draggable with mouse anywhere on screen, including docking into the black letterbox / pillarbox bars.
- Hotkey **`F10`** to cycle HUD display modes.

### 11. Dual-Aspect Viewport & Scaling Pipeline
- **Selectable View Modes (via In-Game Overlay):**
  - `3:2 (Native)`: Preserves authentic 240×160 GBA pixel aspect ratio across all game scenes. Centered with clean, pure black letterbox or pillarbox bars on any modern monitor or handheld display without distortion.
  - `16:9 (Battles only)`: Fixed widescreen (284×160) active exclusively during combat arenas, revealing extended background scenery while maintaining correct sprite proportions. Outside combat (menus, world exploration, cutscenes), the display automatically falls back to 3:2 pillarboxing, preventing tile glitches or edge repetition (see detailed technical rationale in Section 8).
- **Strict Geometric Preservation:** Arbitrary adaptive stretching has been eliminated to ensure sprites, UI, and text never appear warped, stretched, or unevenly scaled.
- **Window Scaling & Presentation:**
  - Freely resizable window with real-time aspect-ratio-preserving viewport centering and letterboxing/pillarboxing.
  - Fullscreen toggle via `Alt + Enter`.
  - Fully compatible with high refresh rates (60/120/144/Uncapped), GPU temporal motion smoothing, xBRZ pattern upscaling, and CRT/LCD shader masks.

### 12. High-Fidelity Audio DSP Suite
- **MP2K Shadow Mixer:** High-sample-rate shadow voice mixer eliminating GBA hardware audio quantization noise.
- **DAC Anti-Aliasing Filter:** Biquad low-pass filter targeting ultrasonic PWM/DAC switching hiss.
- **Parametric Equalizer Profiles:** *Flat (Authentic)*, *Warm Retro*, *Crisp Modern*, *Bass Boost*.
- **Stereo Width Expansion:** Adjustable from 0% (mono) to 200% (expanded stereo).
- **Soft-Knee Peak Limiter:** Prevents digital clipping on multi-card sleights.

### 13. Assist Tools & Save State Thumbnails
- **Save States & Load States:** 10 independent slots with visual thumbnail capture (`saves/thumbnails/slot_X.bmp`) and timestamps.
- **Fast-Forward:** Uncaps framerate with a customizable multiplier from **2x to 10x** (default: 4x).
- **Rewind:** Real-time rewind buffer capturing up to 60 seconds of continuous gameplay.
- **Turbo Dialog & Skip:** Hold **`Tab`** or Gamepad **`Y` / `Triangle`** to auto-advance dialogue boxes at 60Hz.

---

## Default Controls

Key mappings can be configured in [keybinds.ini](keybinds.ini) or via the in-game overlay:

| Function | Keyboard | Gamepad (Xbox / PlayStation / Switch) |
|---|---|---|
| D-Pad / Movement | Arrow Keys | D-Pad / Left Analog Stick (True 360 Walk & Run) |
| A | X | A / Cross / B |
| B | Z | B / Circle / A |
| L | C | Left Bumper (LB / L1 / L) |
| R | V | Right Bumper (RB / R1 / R) |
| Start | Enter (Return) | Start / Options / + |
| Select | Right Shift | Back / Share / - |
| Conversation Backlog | F2 | Back / Select / Touchpad / Share |
| Turbo Dialog Skip | Tab | Y / Triangle / X |
| Fast-Forward | 2 | Right Trigger (RT / R2 / ZR) |
| Rewind | 1 | Left Trigger (LT / L2 / ZL) |
| Performance HUD | F10 | Via In-game Overlay |
| Overlay Menu | Escape | Guide / Home / Menu |

### Save / Load Hotkeys
- **Save State (Slots 1-9):** `Shift + F1` to `Shift + F9`
- **Load State (Slots 1-9):** `F1` to `F9`
- **Performance HUD Cycle:** `F10`
- **Conversation Backlog:** `F2`
- **Fullscreen:** `Alt + Enter`

### In-Game Controls Customization
Access the **Controller (Gamepad)** and **Keyboard Controls** tabs in the overlay menu (`Escape`) to customize your controls with an emulator-inspired interface:
- **Dedicated Device Tabs:** Separate sections for Gamepad and Keyboard eliminate redundant button repetition and declutter the UI.
- **Action-Oriented Naming:** Clear in-game action labels (*Attack / Card Action*, *Jump / Dodge Roll*, *Cycle Deck Left / Right*, *Pause / Camp Menu*, *Switch Deck / Reload*) instead of raw hardware button codes.
- **Emulator-Style Key Capture:** Click any action key to instantly bind via physical keypress (*Press any key... / Esc to Cancel*), with clean key badges rendered in the UI.
- **Keyboard Layout Presets:** One-click presets for *Classic Arrows + Z/X*, *Modern WASD + J/K*, *Retro Emulation*, *Ergonomic*, and *Custom Mappings*.
- **Gamepad Profiles & Buttons:** Presets for *Xbox / Standard*, *Nintendo Switch*, *PlayStation*, *Swapped Bumpers*, *Triggers for Deck*, or custom per-action button remapping.
- **Analog Movement & Deadzone:** Switch between *Original Digital (D-Pad)*, *8-Way Analog Directional*, and *Full 360 Walk & Run*, with custom stick deadzone percentage.
- **Granular Reset Actions:** Independent reset buttons for Gamepad and Keyboard defaults.

---

## Build Workflow

### Linux

#### 1. Build the Game Executable
```bash
./build.sh --config Release
```
Or with CMake directly:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build --config Release --target KHCOMRecomp
```
The resulting executable is automatically copied to `./KHCOMRecomp` (and `build/KHCOMRecomp`).

#### 2. Execution
```bash
./KHCOMRecomp
```

### Windows

#### 1. Static Recompilation
Generates native C++ sources from the ARM/Thumb instructions in the ROM:
```powershell
.\build.ps1 -Recompile
```

#### 2. Build the Game Executable
Compiles the generated C++ shards and links them with the runtime, overlay, enhancement modules, and SDL2:
```powershell
.\build.ps1 -Target KHCOMRecomp -Config Release
```
The resulting binary is generated at `build\Release\KHCOMRecomp.exe`.

#### 3. Execution
```powershell
.\build\Release\KHCOMRecomp.exe
```

---

## Support & Sponsorship

If you enjoy this project and wish to support its continued development, you can support me on Ko-fi:

[![Support on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/rafadono)

