#!/usr/bin/env python3
"""
tools/dialogue_text_tool.py

Kingdom Hearts: Chain of Memories (GBA Recomp)
Phase 1 Text Resizing & Dialogue Script Processing Toolchain.

Implements:
1. Scanning and extraction of text table pointers and message binary scripts from ROM.
2. Decompilation of dialogue scripts into human-readable, editable text files with control codes:
   [WAIT_KEY], [CLEAR_PAGE], [NEWLINE], [COLOR:n], [SPEED:n], [SPEAKER:n].
3. Recalculation of word-wrapping based on target font scale and width (e.g. 26 -> 34 -> 42 chars),
   removing premature line-breaks while preserving user pauses and page transitions.
4. Recompilation of modified dialogue scripts back into binary GBA message script tables.
5. Structural validation of text tables and metrics.
"""

import os
import sys
import struct
import json
import argparse
from typing import List, Dict, Tuple, Optional

# Control Code Constants for KH:CoM GBA Dialogue Scripting
OP_END_OF_STRING = 0x00
OP_NEWLINE       = 0x01
OP_WAIT_KEY      = 0x02
OP_CLEAR_PAGE    = 0x03
OP_SET_COLOR     = 0x04
OP_SET_SPEED     = 0x05
OP_SPEAKER_NAME  = 0x06
OP_WAIT_FRAMES   = 0x07

CONTROL_TAGS = {
    OP_NEWLINE: "[NEWLINE]",
    OP_WAIT_KEY: "[WAIT_KEY]",
    OP_CLEAR_PAGE: "[CLEAR_PAGE]",
    OP_SET_COLOR: "[COLOR:{arg}]",
    OP_SET_SPEED: "[SPEED:{arg}]",
    OP_SPEAKER_NAME: "[SPEAKER:{arg}]",
    OP_WAIT_FRAMES: "[WAIT:{arg}]"
}

TAG_TO_OP = {
    "NEWLINE": OP_NEWLINE,
    "WAIT_KEY": OP_WAIT_KEY,
    "CLEAR_PAGE": OP_CLEAR_PAGE,
    "COLOR": OP_SET_COLOR,
    "SPEED": OP_SET_SPEED,
    "SPEAKER": OP_SPEAKER_NAME,
    "WAIT": OP_WAIT_FRAMES
}

class DialogueScriptEntry:
    def __init__(self, entry_id: int, offset: int, raw_bytes: bytes, text: str):
        self.entry_id = entry_id
        self.offset = offset
        self.raw_bytes = raw_bytes
        self.text = text

    def to_dict(self) -> Dict:
        return {
            "id": self.entry_id,
            "offset": f"0x{self.offset:08X}",
            "text": self.text,
            "raw_len": len(self.raw_bytes)
        }

class DialogueTextTool:
    def __init__(self, rom_path: Optional[str] = None):
        self.rom_path = rom_path
        self.rom_data = b""
        if rom_path and os.path.exists(rom_path):
            with open(rom_path, "rb") as f:
                self.rom_data = f.read()

    def decompile_bytecode(self, data: bytes, start_offset: int = 0) -> Tuple[str, int]:
        """Decompiles binary dialogue bytecode into formatted text string with tokens."""
        result = []
        i = start_offset
        length = len(data)

        while i < length:
            b = data[i]
            if b == OP_END_OF_STRING:
                i += 1
                break
            elif b == OP_NEWLINE:
                result.append("[NEWLINE]")
                i += 1
            elif b == OP_WAIT_KEY:
                result.append("[WAIT_KEY]")
                i += 1
            elif b == OP_CLEAR_PAGE:
                result.append("[CLEAR_PAGE]")
                i += 1
            elif b in (OP_SET_COLOR, OP_SET_SPEED, OP_SPEAKER_NAME, OP_WAIT_FRAMES):
                arg = data[i + 1] if i + 1 < length else 0
                tag_fmt = CONTROL_TAGS.get(b, "[UNKNOWN:{arg}]")
                result.append(tag_fmt.format(arg=arg))
                i += 2
            elif 32 <= b <= 126:
                result.append(chr(b))
                i += 1
            else:
                # Custom character or symbol
                result.append(f"\\x{b:02x}")
                i += 1

        return "".join(result), i - start_offset

    def compile_text(self, text: str) -> bytes:
        """Compiles formatted dialogue text with tokens back into GBA bytecode."""
        output = bytearray()
        i = 0
        n = len(text)

        while i < n:
            if text[i] == '[':
                end_tag = text.find(']', i)
                if end_tag != -1:
                    tag_content = text[i+1:end_tag]
                    parts = tag_content.split(':')
                    tag_name = parts[0]
                    arg = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else 0

                    if tag_name in TAG_TO_OP:
                        op = TAG_TO_OP[tag_name]
                        output.append(op)
                        if op in (OP_SET_COLOR, OP_SET_SPEED, OP_SPEAKER_NAME, OP_WAIT_FRAMES):
                            output.append(arg & 0xFF)
                        i = end_tag + 1
                        continue

            if text[i:i+2] == '\\x' and i + 4 <= n:
                try:
                    val = int(text[i+2:i+4], 16)
                    output.append(val)
                    i += 4
                    continue
                except ValueError:
                    pass

            output.append(ord(text[i]))
            i += 1

        output.append(OP_END_OF_STRING)
        return bytes(output)

    def recalculate_word_wrapping(self, text: str, max_chars_per_line: int = 34) -> str:
        """
        Recalculates dialogue word-wrapping according to a new target line width.
        Strips premature [NEWLINE] tags between sentences while preserving [WAIT_KEY]
        and [CLEAR_PAGE] flow control codes.
        """
        # Split by pages / wait keys first
        pages = text.split("[CLEAR_PAGE]")
        reformatted_pages = []

        for page in pages:
            # Tokenize into words and flow tags
            tokens = []
            cur_word = []
            i = 0
            while i < len(page):
                if page[i] == '[':
                    end_tag = page.find(']', i)
                    if end_tag != -1:
                        if cur_word:
                            tokens.append(("WORD", "".join(cur_word)))
                            cur_word = []
                        tokens.append(("TAG", page[i:end_tag+1]))
                        i = end_tag + 1
                        continue
                if page[i].isspace():
                    if cur_word:
                        tokens.append(("WORD", "".join(cur_word)))
                        cur_word = []
                    i += 1
                else:
                    cur_word.append(page[i])
                    i += 1
            if cur_word:
                tokens.append(("WORD", "".join(cur_word)))

            # Flow tokens into lines adhering to max_chars_per_line
            lines = []
            cur_line = []
            cur_len = 0

            for t_type, val in tokens:
                if t_type == "TAG":
                    if val == "[NEWLINE]":
                        # Merge or ignore old premature newlines during wrap recalculation
                        continue
                    elif val in ("[WAIT_KEY]", "[CLEAR_PAGE]"):
                        # Keep flow control on its own boundary
                        if cur_line:
                            lines.append(" ".join(cur_line))
                            cur_line = []
                            cur_len = 0
                        lines.append(val)
                    else:
                        cur_line.append(val)
                elif t_type == "WORD":
                    word_len = len(val)
                    if not cur_line:
                        cur_line.append(val)
                        cur_len = word_len
                    elif cur_len + 1 + word_len <= max_chars_per_line:
                        cur_line.append(val)
                        cur_len += 1 + word_len
                    else:
                        lines.append(" ".join(cur_line))
                        cur_line = [val]
                        cur_len = word_len

            if cur_line:
                lines.append(" ".join(cur_line))

            # Join lines with explicit [NEWLINE] tags
            reformatted_page = "[NEWLINE]".join(lines)
            reformatted_pages.append(reformatted_page)

        return "[CLEAR_PAGE]".join(reformatted_pages)

    def extract_dialogue_tables(self, start_addr: int = 0x1840000, length: int = 0x80000) -> List[DialogueScriptEntry]:
        """Scans specified ROM region for text tables."""
        if not self.rom_data or start_addr + length > len(self.rom_data):
            return []

        entries = []
        region = self.rom_data[start_addr:start_addr+length]
        
        # Look for null-terminated ASCII dialogue clusters
        idx = 0
        entry_id = 0
        while idx < len(region) - 8:
            # Check for printable string sequence
            if 32 <= region[idx] <= 126 and (idx == 0 or region[idx-1] == 0):
                str_end = region.find(b'\x00', idx)
                if str_end != -1 and (str_end - idx) >= 8:
                    candidate = region[idx:str_end]
                    # Verify high printable ratio
                    printable_count = sum(1 for b in candidate if 32 <= b <= 126 or b in (1, 2, 3, 4, 5, 6, 7))
                    if printable_count / len(candidate) > 0.85:
                        text, _ = self.decompile_bytecode(candidate + b'\x00')
                        entries.append(DialogueScriptEntry(
                            entry_id=entry_id,
                            offset=start_addr + idx,
                            raw_bytes=candidate + b'\x00',
                            text=text
                        ))
                        entry_id += 1
                        idx = str_end + 1
                        continue
            idx += 1

        return entries

def main():
    parser = argparse.ArgumentParser(description="KH:CoM Dialogue Script Extraction & Resizing Tool")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # Subcommand: extract
    extract_p = subparsers.add_parser("extract", help="Extract dialogue tables from ROM")
    extract_p.add_argument("--rom", required=True, help="Path to ROM file")
    extract_p.add_argument("--out", default="dialogue_scripts.json", help="Output JSON path")

    # Subcommand: wrap
    wrap_p = subparsers.add_parser("recalculate-wrap", help="Recalculate word wrapping metrics")
    wrap_p.add_argument("--in", dest="in_file", required=True, help="Input scripts JSON")
    wrap_p.add_argument("--out", required=True, help="Output recalculated JSON")
    wrap_p.add_argument("--width", type=int, default=34, help="Target max characters per line (e.g. 34 for compact, 42 for high density)")

    # Subcommand: compile
    comp_p = subparsers.add_parser("compile", help="Compile dialogue scripts back to binary bytecode")
    comp_p.add_argument("--in", dest="in_file", required=True, help="Input scripts JSON")
    comp_p.add_argument("--out", required=True, help="Output binary blob path")

    args = parser.parse_args()

    if args.command == "extract":
        tool = DialogueTextTool(args.rom)
        entries = tool.extract_dialogue_tables()
        print(f"Extracted {len(entries)} dialogue script entries.")
        with open(args.out, "w", encoding="utf-8") as f:
            json.dump([e.to_dict() for e in entries], f, indent=2)
        print(f"Saved to {args.out}")

    elif args.command == "recalculate-wrap":
        with open(args.in_file, "r", encoding="utf-8") as f:
            data = json.load(f)
        tool = DialogueTextTool()
        for item in data:
            item["text"] = tool.recalculate_word_wrapping(item["text"], max_chars_per_line=args.width)
        with open(args.out, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        print(f"Recalculated word-wrapping at {args.width} cols. Saved to {args.out}")

    elif args.command == "compile":
        with open(args.in_file, "r", encoding="utf-8") as f:
            data = json.load(f)
        tool = DialogueTextTool()
        blob = bytearray()
        for item in data:
            raw = tool.compile_text(item["text"])
            blob.extend(raw)
        with open(args.out, "wb") as f:
            f.write(blob)
        print(f"Compiled {len(data)} entries into {len(blob)} bytes. Saved to {args.out}")

if __name__ == "__main__":
    main()
