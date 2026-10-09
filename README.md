# Touhou 15: Legacy of Lunatic Kingdom — Nintendo Switch Port
*(東方紺珠伝　〜 Legacy of Lunatic Kingdom)*

![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![Status](https://img.shields.io/badge/Status-Fully%20Playable-brightgreen?style=for-the-badge)
![License](https://img.shields.io/badge/Port%20Code-CC0%201.0-blue?style=for-the-badge)

A native homebrew port of ZUN's 2015 bullet hell **Touhou 15: Legacy of Lunatic Kingdom** for the **Nintendo Switch** (Horizon OS).

This build runs the C++ reimplementation by [SteinsGateON](https://space.bilibili.com/34714121) from [YomotsuHisami/th15](https://github.com/YomotsuHisami/th15), compiled **natively for ARM64** (no wasm, no emulation) and driven by an SDL2 host on an OpenGL ES 3 context — no Linux, Box64 or Wine involved.

Companion to the [Touhou 6](https://github.com/saekaze/th06-switch), [Touhou 7](https://github.com/saekaze/th07-switch), [Touhou 8](https://github.com/saekaze/th08-switch), [Touhou 9](https://github.com/saekaze/th09-switch), [Touhou 10](https://github.com/saekaze/th10-switch) and [Touhou 11](https://github.com/saekaze/th11-switch) Switch ports, with the same `touhou15.nro` + `sd:/switch/touhou/touhou15/` layout.

---

## 🆕 What's New — 1.00b-r2

* 🧊 **No more freeze when a spell card ends.** TH15 saves a checkpoint file at every chapter change; the port used to compress and write it on the game thread, which froze the game right as the bullets disappeared. It now runs in the background on another CPU core, like in the original game.

## First release (1.00b-r1)

* 🌙 The full game: Reimu, Marisa, Sanae and Reisen, Legacy and Pointdevice modes, every difficulty, all 6 stages and Extra, chapter checkpoints, endings, music room, manual and replays.
* 🎮 Remappable controls through **Option → Key Config** (the D-Pad and sticks only move).
* ⏳ **The first launch is long** — see [First launch](#-first-launch-is-slow--only-once) below. Every launch after it is quick.
* 📁 Recommended folder: `sd:/switch/touhou/touhou15/`, next to the other Touhou ports.

---

## ✨ Key Features

* ⬛ **OLED-Friendly Pillarboxing:** the original 640×480 picture is centred with pure black (`#000000`) bars and an aspect-correct upscale.
* 🔊 **Full Audio:** sound effects plus BGM streamed straight out of your `thbgm.dat` (original 16-bit PCM and loop points — no conversion step).
* 🎮 **Sane, Remappable Controls:** Joy-Con (handheld, grip, detached) and Pro Controller with the same default layout as every other Touhou Switch port (B shoot, A bomb, L/ZL focus, R/ZR skip, + pause); the in-game **Key Config** can rebind them.
* 👆 **Touch Screen:** in handheld mode, drag to move and tap menus — upstream's touch controller, mapped through the pillarbox.
* 🈂️ **Japanese Text Without Windows:** text glyphs are baked with FreeType from `msgothic.ttc` (or the Switch's built-in Japanese font) on first launch and cached.
* 🌏 **Language-Aware Title:** hbmenu shows `東方紺珠伝　～ Legacy of Lunatic Kingdom` on consoles set to 日本語 and the romanised title everywhere else.
* 💾 **Saves Next to the Data:** `th15.cfg`, `scoreth15.dat`, replays (`replay/th15_01.rpy` …), chapter checkpoints (`autosave/`) and snapshots are written into the SD folder the game loaded from.

---

## 📥 Installation Guide

> ⚠️ **Disclaimer:** In compliance with ZUN's guidelines and copyright law, this repository contains **ONLY the homebrew engine code**. No game assets are distributed. You must legally own a copy of *Touhou 15: Legacy of Lunatic Kingdom v1.00b*.

### 1. SD Card File Structure

1. Ensure your Nintendo Switch is running custom firmware (Atmosphère CFW).
2. Download the latest `touhou15.nro` from the [Releases](../../releases) tab (or build from source).
3. Create the folder `sd:/switch/touhou/touhou15/` and copy the following into it:

```text
sd:/switch/touhou/touhou15/
    ├── touhou15.nro          # Nintendo Switch homebrew executable
    ├── th15.dat              # Main game archive (v1.00b)
    ├── thbgm.dat             # Background music archive
    └── msgothic.ttc          # MS Gothic (C:\Windows\Fonts) - recommended
```

`thbgm.dat` is optional (the game runs silent without it). Without `msgothic.ttc` the console's built-in Japanese font is used.

**Recommended place: `sd:/switch/touhou/touhou15/`.** Keeping every Touhou port in one `sd:/switch/touhou/` folder (`touhou6`, `touhou7`, `touhou8` …) is much tidier than a separate folder per game. Other places still work: the port first looks in its own folder (wherever the NRO is), then for a `th15` / `touhou15` folder (any capitalisation) directly on the SD card, in `switch/`, `touhou/`, `switch/touhou/`, `games/` or `roms/`.

### ⏳ First launch is slow — only once

**The very first start takes much longer than usual. This is normal — let the loading bar finish and don't close the game.**

Why: TH15 draws all of its text (menus, dialogue, spell card names, the manual) with Windows fonts, and the game files don't contain them. So on the first start the port builds the text font itself from `msgothic.ttc` (or the Switch's Japanese font): about 7,500 characters in 8 sizes and weights, and then saves the result to the SD card as `fontcache/` (about 55 MB). It uses three of the Switch's CPU cores to do it, but it still takes a while, and writing 55 MB to a slow SD card adds to that.

**After that it's fine:** later launches just load the finished `fontcache/` and start normally. The font is only built again if you delete `fontcache/` or change the font file. If you have the GDI-baked tables from an upstream web build (`th15_web/assets/sdl-native/fonts/`), copy that folder to `sd:/switch/touhou/touhou15/fonts/` for pixel-exact Windows text.

> 💡 TH15 keeps the whole `th15.dat` and the font tables in memory. If it runs out of memory from the album, launch hbmenu through a game (hold **R** while starting any game) for full RAM.

### 2. Launching

Run `touhou15.nro` from the **Homebrew Menu (hbmenu)**, **Sphaira launcher**, or a home screen forwarder. Title-menu **Quit** returns to hbmenu. If something is missing, the port shows what and where it looked, and `th15-switch.log` is written next to the saves.

---

## 🕹 Controls

| Nintendo Switch Button | Action |
| :--- | :--- |
| **Left Stick / D-Pad** | Character Movement |
| **B** | Shoot / Confirm |
| **A** | Bomb / Cancel |
| **L / ZL** | Focus (Precision Slow-Motion Movement) |
| **R / ZR** | Skip Dialogue (hold, Ctrl) |
| **+ (Plus)** | Pause / In-Game Menu |
| **X** | Enter (while not bound in Key Config) |
| **Y** | R key (Retry shortcut, while not bound in Key Config) |
| **− (Minus)** | Snapshot (saved to `snapshot/` next to the game data) |
| **Touch** (handheld) | Drag to move, tap menus |

**The same default layout in every Touhou Switch port:** B shoots, A bombs, L/ZL focuses, R/ZR skips dialogue, + pauses. These are only defaults — the Switch buttons act as the game's own gamepad, so the in-game **Option → Key Config** can rebind shot, bomb, focus and pause. The D-Pad and sticks only move — they can never be picked as a button. Key Config numbers: 0 B, 1 A, 2 L/ZL, 3 R/ZR, 4 +, 5 X, 6 Y.

---

## 🛠 Building from Source

### Automated Build (GitHub Actions)

`.github/workflows/build-switch.yml` (a copy also lives in `scripts/`) compiles `touhou15.nro` inside the official `devkitpro/devkita64` container and uploads it as an artifact; tagging `v*` also publishes it as a release asset. A second job builds the same sources for Linux and runs the host tests.

### Local Build (Linux / macOS / WSL)

1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with `devkitA64` and `libnx`.
2. Install the required Switch portlibs:

   ```bash
   sudo dkp-pacman -Syu
   sudo dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau switch-sdl2 switch-freetype switch-libpng switch-bzip2 switch-zlib switch-harfbuzz
   ```

3. Build:

   ```bash
   export DEVKITPRO=/opt/devkitpro
   ./scripts/build_switch.sh
   ```

   The result is `build-switch/touhou15.nro`.

Desktop Linux test build and host tests (same sources; needs `libsdl2-dev libfreetype-dev libgles-dev`, and `xvfb` + a CJK font for the full test set):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
xvfb-run -a ./build/th15_host_tests
./build/touhou15 /path/to/folder/with/th15.dat
```

---

## 📂 How It Works

The game logic is the architecture-independent C++ from upstream (`game/`), compiled natively for AArch64 with the upstream's own code-generation contract (`-ffp-contract=off -fno-strict-aliasing -fno-exceptions -fno-rtti`). It is already 64-bit clean; the host in `src/` is upstream's SDL host with the browser parts replaced — see [`upstream/UPSTREAM.md`](upstream/UPSTREAM.md) and the exact [`switch-port.patch`](upstream/switch-port.patch).

| File | Purpose |
| :--- | :--- |
| `game/` | upstream TH15 game logic (ECL/ANM VMs, player, bullets, lasers, chapters, menus, replays, …) |
| `portable/sdl/Renderer.*` | upstream semantic GLES renderer, ported SDL3/WebGL2 → SDL2/GLES 3 |
| `src/Application*.cpp`, `src/FileStore.*`, `src/SceneDisplay.*` | upstream host: title, runs, endings, saves, replays, checkpoints |
| `src/main.cpp` | Switch main loop, controls, touch, snapshots (replaces upstream `BrowserRuntime.cpp`) |
| `src/SdlCompat.hpp` | the SDL3 file calls of the upstream host on SDL2 |
| `src/GraphicsDevice.*` | ANM textures → renderer surfaces |
| `src/AudioDevice.*` / `src/BgmStream.*` | miniaudio mixer; BGM streamed from `thbgm.dat` with original loop points |
| `src/FontDevice.*` / `src/FontBaker.*` | text glyph tables (upstream GDI tables, cached FreeType bake, or fresh bake) |
| `src/Platform.*` | SD paths, files, log |
| `platform/switch/icon.jpg` | 256×256 NRO icon (the original game cover) |
| `scripts/build_switch.sh` | one-shot Switch build |
| `scripts/nacp_lang.py` | Japanese NACP title slot |
| `scripts/gen_cp932.py` | CP932 → Unicode table for the glyph baker |
| `tests/test_host.cpp` | host tests for the port layer |

The game ticks once per vsync (60 Hz); after a real stall it catches up by at most four ticks, like upstream's frame cadence. The CPU is set to the same 1785 MHz boost the other native ports use.

---

## 🤝 Credits & Acknowledgments

* **ZUN / Team Shanghai Alice** — original creator and developer of the Touhou Project series.
* **[SteinsGateON](https://space.bilibili.com/34714121)** — the TH15 C++ reimplementation this port is built on, published by **[YomotsuHisami](https://github.com/YomotsuHisami/th15)**.
* **Switchbrew & devkitPro Team** — the open-source `libnx` SDK and Switch toolchain.

**Licensing:** the Switch port code (`src/main.cpp`, `src/Platform.*`, `src/SdlCompat.hpp`, `src/BgmStream.*`, `src/FontBaker.*`, `scripts/`, `tests/`, build files) is CC0 1.0 (see `LICENSE`). Everything else is upstream code by SteinsGateON under the **MIT** licence — see [`upstream/LICENSE-upstream-MIT.txt`](upstream/LICENSE-upstream-MIT.txt) and [`upstream/THIRD-PARTY-NOTICES.txt`](upstream/THIRD-PARTY-NOTICES.txt). Bundled third-party headers keep their own licences (`portable/sdl/third_party/`).
