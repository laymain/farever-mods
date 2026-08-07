# imgui

A native [HLX framework](https://github.com/hlx-framework) plugin wrapping
[Dear ImGui](https://github.com/ocornut/imgui) as `imgui64.hdll`, exposed to mods through the
`hl-imgui` haxelib (`hl-imgui/`) so any mod can build a debug or tool UI without touching native
code. See `../imgui-demo/` for a complete, working example mod.

## Setup

1. Drop the built `imgui64.hdll` into `hlx/plugins/` (next to the game's own plugins).
2. Install the `hl-imgui` haxelib into your mod's Haxe project:

   ```
   haxelib git hl-imgui https://github.com/laymain/farever-mods.git main imgui/hl-imgui/src
   ```
3. Add `-lib hl-imgui` to the mod's `compile.hxml`.

That's it - depending on `hl-imgui` also pulls in `imgui.ImGuiFrame`, which hooks the game's
present call and drives ImGui's init/new-frame/render cycle automatically. A mod never calls any
of that itself.

## How to use

A mod registers a draw callback with `ImGui.register(name, callback)`; it runs once per frame from
then on. `ImGui.unregister(name)` removes it again, though most mods never need to - a registered
panel just lives for the process. `name` only needs to be unique among mods that also use
`hl-imgui`, so pass `HlxRuntime.moduleName()`:

```haxe
ImGui.register(HlxRuntime.moduleName(), panel.draw);
```

### Minimal example

```haxe
package mymod;

import imgui.ImGui;
import imgui.ref.FloatRef;

@:build(hlx.runtime.Mod.build())
class MyMod {
	static function main():Void {
		var panel = new HelloPanel();
		ImGui.register(HlxRuntime.moduleName(), panel.draw);
	}
}

class HelloPanel {
	var clickCount = 0;
	var speed = new FloatRef(1);

	public function new() {}

	public function draw():Void {
		if (ImGui.begin("Hello")) {
			ImGui.text("Hello, world!");
			if (ImGui.button("Click me"))
				clickCount++;
			ImGui.text('Clicked $clickCount times');
			ImGui.sliderFloat("Speed", speed, 0, 10);
		}
		ImGui.end();
	}
}
```

Every ImGui widget is a static function on `imgui.ImGui` (`ImGui.button(...)`,
`ImGui.sliderFloat(...)`, `ImGui.beginTable(...)`, ...). See `hl-imgui/README.md` for the full
rundown - value refs (`imgui.ref.BoolRef`/`FloatRef`/...) for widgets that need to persist state
across frames, multi-component vectors, and everything else the demo mod exercises.

## Repository layout

- `native/third_party/imgui`, `native/third_party/cimgui` - vendored sources. See
  `native/third_party/VERSIONS.md` for exactly which commits, and the regeneration steps when
  bumping either.
- `native/codegen/generate.mts` - reads cimgui's own API metadata and regenerates the binding
  surface (`native/src/generated/imgui_prims.cpp`, `hl-imgui/src/imgui/{Enums,Structs,ImGui}.hx`).
  Run from `imgui/` as `node native/codegen/generate.mts` after bumping the vendored sources -
  output is committed, not built on the fly.
- `native/src/imgui_native.cpp` - hand-written lifecycle glue the generator can't express: context
  creation and the DX12/Win32 backends.

See `NATIVE.md` for build/integration history (what broke, what got fixed, and why).

## Building

`npm run build imgui` (or `deploy`) from `farever-mods/` - same native-plugin pipeline every other
`PLUGIN`-type mod here uses. Requires Windows + MSVC + the Windows 10 SDK
(`d3d12.h`/`dxgi.h`/`d3dcompiler.h`) + a HashLink checkout/SDK (`hl.h` + `libhl64.lib`), configured
once via `npm run setup` - same requirements as `shader-cache`'s own native half, see that mod's
README for the WSL/MSVC cross-compile recipe if not building from native Windows.
