# Upstream

`game/`, `src/` (except the files listed below) and `portable/` come from
**[YomotsuHisami/th15](https://github.com/YomotsuHisami/th15)** — the TH15
reimplementation by [@SteinsGateON](https://space.bilibili.com/34714121),
MIT-licensed (`LICENSE-upstream-MIT.txt`) — at commit
`fe113b6f9cbcb676b15dfd95cfd5916c4d5de95e` (2026-10-06):

| Here | Upstream |
| :--- | :--- |
| `game/` | `th15_web/cpp/game/` |
| `src/` | `th15_web/cpp/sdl/` (host: title, runs, endings, replays, checkpoints, audio, text) |
| `portable/sdl/`, `portable/input/` | `portable/sdl/`, `portable/input/` (renderer, shaders, miniaudio / stb headers, keyboard map, touch gestures) |

Switch port files (new): `src/main.cpp` (replaces `BrowserRuntime.cpp`),
`src/Platform.*`, `src/SdlCompat.hpp`, `src/BgmStream.*`, `src/FontBaker.*`,
`src/MiniaudioConfig.hpp`, `src/BackgroundJob.hpp`, `src/LoadingScreen.*`,
`scripts/`, `tests/`, `CMakeLists.txt`. `src/LoadingOverlay.*` and
`src/LoadingQuad.hpp` come from the TH15 PSP port (shared with this port by
its author; include paths adapted, UVs as `AnmRenderer::pack_quad` builds them).

Changes to upstream files:

* `game/` — in [`switch-port.patch`](switch-port.patch): GCC rejects
  `visual={}` on `AnmVisualState`, so it is `visual=AnmVisualState{}`.
  `BattleCheckpoint::build_file` splits the section building out of
  `write_file`, so the host can LZSS-encode and write Pointdevice checkpoints
  on a worker thread (r2; a spell end froze the game otherwise), and
  `AnmCheckpoint` keeps its newest-first order in a deque and looks entries up
  by handle directly (both were quadratic in the number of snapshotted
  animations). The game code is otherwise untouched; it is already 64-bit clean.
* `portable/sdl/` — in [`switch-port.patch`](switch-port.patch): SDL3/WebGL2 →
  SDL2/GLES 3 (Switch Mesa) window and context, `EXT_clip_control` through the
  GLES loader, aspect-correct pillarboxed present, `packed` (a reserved GLSL ES
  word Mesa rejects) renamed.
* `src/` host files (edited in place, marked "Switch port"): SDL3 file calls go
  through `SdlCompat.hpp`; th15.dat, saves and replays live in the SD-card
  folder; saves remove the old file before renaming (FAT); audio uses SDL2
  queued output and streams BGM from the original thbgm.dat instead of OGG
  files; text glyph tables are baked from msgothic.ttc with FreeType on first
  launch instead of Windows-GDI files; start-up loads text/sig/ascii.anm
  before the fonts and the rest of the title, so `LoadingScreen` shows the
  original start-up screen meanwhile (`loading_ready` / `loading_step`,
  `GraphicsDevice::prepared`).

Line endings were normalised to LF. Include paths `../../../portable/` became
`../portable/`.

When upstream updates: copy the new `th15_web/cpp/game` and `portable/` over
these folders, re-apply `switch-port.patch`, and merge `th15_web/cpp/sdl`
changes into `src/` by hand.
