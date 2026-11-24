# PPSSPP MultiSplit

An experimental Windows x64 libretro core for local split-screen PSP multiplayer. Unofficial companion to [PPSSPP](https://github.com/hrydgard/ppsspp), not affiliated with the PPSSPP or RetroArch projects.

## Features

- Two, three or four PSP sessions in one RetroArch window, including horizontal and vertical two-player layouts.
- Native Vulkan rendering per session using separate hidden worker processes; frames are composed in system memory.
- Local Ad Hoc multiplayer using loopback, automatic MAC addresses and a server hosted by player 1.
- Independent controllers and in-game save directories; audio from player 1 only.
- Shared PPSSPP core options, with necessary multiplayer and graphics settings fixed.
- Reset returns to the player selector.

Tested by the project maintainer on a Lenovo Legion Go S (Ryzen Z2 Go), with four PSP sessions at 60 FPS in Virtua Tennis 3, independent controllers and local Ad Hoc. This is a specific test result, not a performance guarantee for every game or system.

## Installation

Install the Windows x64 release by copying the DLL to RetroArch's `cores` directory and the matching `.info` file to `info`. The regular PPSSPP system assets must be available in `system/PPSSPP`. Load a legally obtained PSP game and select the layout. No additional RetroArch windows are launched.

The helper executable is extracted to `%LOCALAPPDATA%\PPSSPP-MultiSplit\psp_worker.exe` and retained so Windows Firewall can remember its authorization. Windows may ask on first launch. The optional firewall configuration script limits its rule to loopback; it is not needed when the remembered permission works. Do not disable Windows Firewall.

## Known limitations

Windows x64 only for this build. Libretro savestates, rewind and cheats are not implemented. Native in-game saves are supported. Some options require restarting content. The output to RetroArch is a CPU-composited frame, despite each worker rendering with Vulkan. Other games and host configurations have not been comprehensively tested.

## Source and building

The bridge (`bridge.c`) and worker (`worker.c`) are included, with the common ABI definitions, Vulkan declarations, native option definitions, import definitions and layout test. `BUILD.py` reproduces the bridge with the exact released payloads; `BUILD_FULL.py` compiles both the bridge and worker using Clang and LLD and packs an upstream PPSSPP DLL. See `UPSTREAM.md` for the exact upstream revision and provenance. The original engine is bundled unmodified; its upstream source is separate from this repository.

## License and credits

PPSSPP is developed by the PPSSPP contributors and is licensed under GPL-2.0-or-later. This companion is prepared for distribution under the same license (see `COPYING`). PPSSPP and libretro are credited for the underlying engine and interfaces. No ROMs or game assets are included.

## v1.0.1 input fix

The selector accepts input on all four RetroArch ports and keyboard arrows/Enter. If port 1 has not navigated the selector, the confirming pad controls PSP 1. Other ports are assigned in ascending order. The engine and worker are unchanged.
