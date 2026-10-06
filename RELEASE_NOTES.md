<img width="256" height="256" alt="Touhou 15" src="https://raw.githubusercontent.com/saekaze/th15-switch/main/platform/switch/icon.jpg" />

## Touhou 15 — 1.00b-r1 (first release)

A native Nintendo Switch port of Touhou 15: Legacy of Lunatic Kingdom v1.00b, built on SteinsGateON's TH15 C++ reimplementation (published by YomotsuHisami) and compiled natively for ARM64.

* 🌙 **The full game:** Reimu, Marisa, Sanae and Reisen, Legacy and Pointdevice modes, every difficulty, all 6 stages and Extra, chapter checkpoints, endings, music room and replays.
* 🎮 **Remappable controls** through Option → Key Config (defaults match the other ports; the D-Pad and sticks only move).
* 🔊 BGM streamed straight from `thbgm.dat` with the original loop points.
* 👆 Touch controls in handheld mode.
* 📁 Recommended folder: `sd:/switch/touhou/touhou15/`, next to the other ports.

> ⏳ **The first launch is long — this is normal, only happens once.** The game's text needs Windows fonts that aren't in the game files, so the port builds the font itself from `msgothic.ttc` (about 7,500 characters in 8 sizes) and saves it as `fontcache/` (~55 MB) on the SD card. Let the loading bar finish. **After that it's fine** — later launches load the saved font and start normally.

Put `touhou15.nro`, `th15.dat`, `thbgm.dat` and `msgothic.ttc` in the folder and launch from hbmenu. If it runs out of memory from the album, start hbmenu by holding **R** while launching a game.
