# Changelog — Touhou 15 Switch port

## Touhou 15: Legacy of Lunatic Kingdom — Switch Port r1

First release (display version `1.00b-r1`).

* The full game on SteinsGateON's C++ reimplementation, compiled natively for ARM64.
* BGM streamed from `thbgm.dat`, touch controls, remappable controls.
* The first launch builds the text font from `msgothic.ttc` on three CPU cores and caches it in `fontcache/` (~55 MB); later launches load the cache.
