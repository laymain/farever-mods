# farever-mods

Mods for Farever, built on the [HLX Modding Framework](https://hlx-framework.github.io/).  
Each mod lives in its own top-level folder; this root holds what's shared across all of them (build tooling, CI).

## Requirements

- [Haxe](https://haxe.org/) 4.3.3
- [Node.js](https://nodejs.org/) 22+
- [.NET SDK](https://dotnet.microsoft.com/) (for `HLX.GamelibGenerator`)
- [HLX.GamelibGenerator](https://www.nuget.org/packages/HLX.GamelibGenerator) dotnet tool: `dotnet tool install -g HLX.GamelibGenerator`
- Access to a Farever install (for its `hlboot.dat`, needed to generate `farever-gamelib`)

## Mods

| Mod | Description |
| --- | --- |
| [pew-pew-meter](pew-pew-meter/README.md) | A lightweight DPS meter for Farever. |

## Local development

Set up the haxelibs each mod compiles against:

```
haxelib git hlx-runtime https://github.com/hlx-framework/hlx-core.git main hlx-runtime/src
hlx-gamelib-generator <path/to/farever/hlboot.dat> <output/gamelib/directory>
haxelib dev farever-gamelib <output/gamelib/directory>
```

Then install npm dependencies and build:

```
npm install
npm run build            # builds every mod
npm run build -- <mod>   # builds just one mod, e.g. pew-pew-meter
```
