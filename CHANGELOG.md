# Changelog — Touhou 15 Switch port

## Touhou 15: Legacy of Lunatic Kingdom — Switch Port r3

* **Original loading screen:** start-up now shows the original game's screen, the "東方Project / 15th Project Shrine Maiden" picture from `sig.anm` with the 少女祈祷中 / NowLoading... animation from `ascii.anm` (script 17: the text pulsing, small moons drifting), instead of a grey bar. It runs during the first-launch font build too. Everything is drawn from the player's `th15.dat`; no images ship with the port.
* Thanks to [シエル](https://github.com/kan8223-dotcom/th10-psp-native), who makes native PSP ports of Touhou: they worked out how the original builds this screen (which `sig.anm` textures, `ascii.anm` script 17 at 960,784, no progress bar) and shared their TH15 PSP code for it (`src/LoadingOverlay.*`, `src/LoadingQuad.hpp`), which runs the animation on its own copy of the ANM VM so nothing the game computes changes.
* To get there sooner, start-up loads `text.anm`, `sig.anm` and `ascii.anm` first, then the font, then the rest of the title.
* Display version `1.00b-r3`.

## Touhou 15: Legacy of Lunatic Kingdom — Switch Port r2

* **Freeze fix:** the game froze for a moment whenever a spell card or attack ended (every chapter change). TH15 saves a checkpoint file at each chapter start, and the port compressed it (LZSS) and wrote it to the SD card on the game thread. That now runs on a background thread on another CPU core, like the original's checkpoint worker.
* The checkpoint snapshot itself got faster with many cancel effects alive (two loops that grew with the square of the animation count are now linear).
* `th15-switch.log` notes how long each checkpoint took.
* Display version `1.00b-r2`.


## Touhou 15: Legacy of Lunatic Kingdom — Switch Port r1

First release (display version `1.00b-r1`).

* The full game on SteinsGateON's C++ reimplementation, compiled natively for ARM64.
* BGM streamed from `thbgm.dat`, touch controls, remappable controls.
* The first launch builds the text font from `msgothic.ttc` on three CPU cores and caches it in `fontcache/` (~55 MB); later launches load the cache.
