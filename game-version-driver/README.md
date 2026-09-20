# game-version-driver

## Description

_A companion for mods built on the [HLX Modding Framework](https://github.com/hlx-framework/)._

Lets the HLX Modding Framework recognize which version of Farever you have installed. Mod
authors can then mark their mods as only working with certain game versions, and the framework
will automatically skip loading a mod that isn't compatible with your current version - instead
of it crashing or misbehaving after a game update.

This is optional. Without it, HLX simply loads every mod as usual, with no version checks.

## Installation instructions

Prefer the use of Vortex to install this mod.

For manual installation:
1. Install [hlx-core](https://www.nexusmods.com/site/mods/2118), the HLX Modding Framework's loader.
2. Download this mod and extract it to `<GameDir>`.
