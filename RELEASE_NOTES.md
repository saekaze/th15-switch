<img width="256" height="256" alt="Touhou 15" src="https://raw.githubusercontent.com/saekaze/th15-switch/main/platform/switch/icon.jpg" />

## Touhou 15 — 1.00b-r2

* 🧊 **No more freeze when a spell card ends.** At every chapter change TH15 saves a checkpoint file; the port compressed and wrote it to the SD card on the game thread, which froze the game for a moment right as the bullets disappeared. It now happens in the background on another CPU core, like in the original game.
* The checkpoint snapshot is also faster when many bullets were just cancelled.

Update: replace `touhou15.nro` in `sd:/switch/touhou/touhou15/`. Saves, replays and `fontcache/` are kept.

---

## Touhou 15 — 1.00b-r1 (first release)

A native Nintendo Switch port of Touhou 15: Legacy of Lunatic Kingdom v1.00b, built on SteinsGateON's TH15 C++ reimplementation (published by YomotsuHisami) and compiled natively for ARM64.

* 🌙 **The full game:** Reimu, Marisa, Sanae and Reisen, Legacy and Pointdevice modes, every difficulty, all 6 stages and Extra, chapter checkpoints, endings, music room and replays.
* 🎮 **Remappable controls** through Option → Key Config (defaults match the other ports; the D-Pad and sticks only move).
* 🔊 BGM streamed straight from `thbgm.dat` with the original loop points.
* 👆 Touch controls in handheld mode.
* 📁 Recommended folder: `sd:/switch/touhou/touhou15/`, next to the other ports.

> ⏳ **The first launch is long — this is normal, only happens once.** The game's text needs Windows fonts that aren't in the game files, so the port builds the font itself from `msgothic.ttc` (about 7,500 characters in 8 sizes) and saves it as `fontcache/` (~55 MB) on the SD card. Let the loading bar finish. **After that it's fine** — later launches load the saved font and start normally.

Put `touhou15.nro`, `th15.dat`, `thbgm.dat` and `msgothic.ttc` in the folder and launch from hbmenu. If it runs out of memory from the album, start hbmenu by holding **R** while launching a game.
