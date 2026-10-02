# Technical Specification: In-Game Dialogue Font Resizing & Dialogue Engine Overhaul

## Overview

This document details the completed implementation of dialogue font resizing, variable-width font (VWF) metric adjustments, nine-slice dialogue box scaling, and dialogue script re-wrapping for *Kingdom Hearts: Chain of Memories* (GBA Recompiled).

The original game renders dialogue within a fixed-width window (typically 3 to 4 lines of text) using 8x8 or 8x12 1bpp/2bpp tile-based glyphs dynamically uploaded into GBA VRAM BG Character blocks. This implementation provides end-to-end support for custom font density scaling, widescreen dialogue box expansion, and automated dialogue script re-wrapping.

---

## Architectural Implementation

### Phase 1: Dialogue Script Extraction, Decompilation & Re-encoding Toolchain
Implemented in [`tools/dialogue_text_tool.py`](../tools/dialogue_text_tool.py):
- **Bytecode Scanner & Decompiler (`extract`)**:
  - Scans ROM text tables and decompiles raw message bytecode into structured, human-readable representations preserving control codes:
    - `[NEWLINE]` (0x01): Tile cursor advance.
    - `[WAIT_KEY]` (0x02): Text pause waiting for user button press.
    - `[CLEAR_PAGE]` (0x03): Screen page clear.
    - `[COLOR:n]` (0x04): Dialogue palette shift.
    - `[SPEED:n]` (0x05): Text typewriter speed.
    - `[SPEAKER:n]` (0x06): Speaker name badge index.
- **Word-Wrap Recalculation Engine (`recalculate-wrap`)**:
  - Eliminates premature line-breaks and merges sentences based on target line capacity (26 cols original, 34 cols compact, 42 cols high-density).
  - Preserves user interaction pauses (`[WAIT_KEY]`) and pagination breaks (`[CLEAR_PAGE]`).
- **Binary Bytecode Compiler (`compile`)**:
  - Recompiles JSON formatted scripts back into native GBA binary bytecode tables for injection or dynamic ROM patch staging.

```bash
# Extract all dialogue tables from ROM:
python3 tools/dialogue_text_tool.py extract --rom "roms/Kingdom Hearts - Chain of Memories (U)(Venom).gba" --out dialogue_extracted.json

# Recalculate word wrapping for 34 columns (compact density):
python3 tools/dialogue_text_tool.py recalculate-wrap --in dialogue_extracted.json --out dialogue_compact.json --width 34

# Recompile back into binary bytecode:
python3 tools/dialogue_text_tool.py compile --in dialogue_compact.json --out dialogue_patched.bin
```

---

### Phase 2: Engine Variable-Width Font (VWF) Resizing
Implemented in [`src/font_resizer.h`](../src/font_resizer.h) and [`src/font_resizer.cpp`](../src/font_resizer.cpp):
- **Font Scale Modes**:
  - `FontScale::Original100`: 8x12 glyphs, 16px line pitch (standard GBA).
  - `FontScale::Medium85`: 7x10 glyphs, 13px line pitch (~85% size).
  - `FontScale::Compact70`: 6x8 glyphs, 10px line pitch (~70% size).
  - `FontScale::Micro55`: 5x7 glyphs, 8px line pitch (~55% size).
- **Proportional Advance Table**:
  - Full ASCII (32..126) proportional kerning width table dynamically scaled per font mode.
- **4bpp Tile Downsampler**:
  - Converts source 1bpp/2bpp font glyphs into 8x8 4bpp GBA VRAM tile blocks (32 bytes per tile) with optional edge smoothing.
- **VRAM Text Block Intercept**:
  - Intercepts tile upload routines to GBA Character Base Blocks (0x06000000 - 0x0600FFFF) to inject resized glyph tiles into the background layer.

---

### Phase 3: Dialogue Box Nine-Slice Scaling
Implemented in [`src/dialogue_box_scaler.h`](../src/dialogue_box_scaler.h) and [`src/dialogue_box_scaler.cpp`](../src/dialogue_box_scaler.cpp):
- **Nine-Slice Horizontal Tilemap Expansion**:
  - Extends standard 28-tile dialogue box (224px wide) to 34 tiles (272px) in 16:9 widescreen and up to 36+ tiles in dynamic wide layouts.
  - Automatically stretches the central repeating slice (`expand_dialogue_tilemap_row`) without distorting corner caps.
- **Dialogue OAM Border Sprite Adjustment**:
  - Dynamically shifts left/right dialogue box border sprites and text advance indicators to match the expanded horizontal width.

---

## Verification & Testing

Both automated Python verification and compiled C++ test runners validate this subsystem:

1. **Python Automated Verification**:
   ```bash
   python3 tests/test_runtime_functions.py
   # Validates: bytecode roundtrip, 26->34 col wrap recalculation, 9-slice box geometry (17/17 suites pass)
   ```

2. **Native C++ Unit Test Runner**:
   ```bash
   ./build/khcom_unit_tests
   # Validates: FontResizer glyph metrics, 4bpp tile packing, DialogueBoxScaler tilemap stretching (12/12 modules pass)
   ```
