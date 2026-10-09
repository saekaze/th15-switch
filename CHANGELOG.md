# Changelog — Touhou 15 Switch port

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
