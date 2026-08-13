# enhanced-chat

## Description

_A mod for Farever, built on the [HLX Modding Framework](https://github.com/hlx-framework/)._

Replaces Farever's default chat window with a tabbed one. Channels (general, local, group, ...)
each get their own tab instead of being merged into one scrolling feed, and every direct-message
conversation opens in its own tab instead of interleaving with everything else. Whisper another
player straight from any tab, and ignore a player to stop seeing their messages.

Chat also supports `!` commands, typed straight into the input box - `!whisper <player> <message>`
included. Autocompletion isn't limited to this mod's own commands: any installed mod can register
its own via the HLX Modding Framework's shared Registry and Bus, and it shows up in chat the same
way - e.g. `!dps` opens [pew-pew-meter](../pew-pew-meter/README.md)'s panel if that mod is installed.

## Installation instructions

Prefer the use of Vortex to install this mod.

For manual installation:
1. Install [hlx-core](https://www.nexusmods.com/site/mods/2118), the HLX Modding Framework's loader.
2. Install [imgui](https://www.nexusmods.com/farever/mods/4), the native plugin this mod's UI is now rendered through.
3. Download this mod and extract it to `<GameDir>`.
