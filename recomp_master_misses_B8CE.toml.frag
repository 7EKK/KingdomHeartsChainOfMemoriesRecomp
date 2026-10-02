# recomp_master_misses.toml.frag — AUTO-GENERATED proposal.
# These guest PCs were reached by runtime_dispatch with no generated
# function and were bridged through the interpreter this session.
# A HUMAN reviews these and merges the genuine ones into the binary's
# config; this file is NEVER auto-merged (PRINCIPLES.md "Never
# auto-write game.toml").
#
# Proposal shapes:
#   [[extra_func]]  — a standalone missed function entry.
#   [[jump_table]]  — flagged in a comment when a tight run of
#       consecutive same-mode misses looks like a computed-jump
#       switch's case targets. Sizing the table covers the whole
#       switch in ONE entry; prefer it over the per-case [[extra_func]].
# BIOS PCs (< 0x4000) belong in bios/gba_bios.toml; cart PCs in the
# game's game.toml.
#
# game:  KINGDOMHEART
# code:  B8CE
# sha1:  10729bd884f8fdca7a310b6d606c52e46657aa48

[[extra_func]]
addr = 0x0806D334
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x1"

[[extra_func]]
addr = 0x0806DA34
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x8"

[[extra_func]]
addr = 0x0806E14E
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x5"

[[extra_func]]
addr = 0x08073768
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x1"

[[extra_func]]
addr = 0x0807457C
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x1"

[[extra_func]]
addr = 0x08075F14
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x1"

[[extra_func]]
addr = 0x080A1C7C
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x1"

[[extra_func]]
addr = 0x080A1F18
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x1"

