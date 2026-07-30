# shader-persistent-cache

## Description

_A mod for Farever, built on the [HLX Modding Framework](https://github.com/hlx-framework/)._

Farever compiles its DX12 shader pipelines on demand, so the game can visibly stutter the first
time it hits a new shader, material, or render-state combination - often right as you walk into a
new area or effect. This mod caches every compiled pipeline to disk: once a pipeline has been
built once, any later session reuses it instead of recompiling it live, so the more you've played,
the fewer and shorter those hitches get.

## Installation instructions

Prefer the use of Vortex to install this mod.

For manual installation:
1. Install [hlx-core](https://www.nexusmods.com/site/mods/2118), the HLX Modding Framework's loader.
2. Download this mod and extract it to `<GameDir>`.
