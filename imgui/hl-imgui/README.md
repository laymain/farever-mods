# hl-imgui

Haxe/HashLink bindings for [Dear ImGui](https://github.com/ocornut/imgui), backed by the `imgui`
plugin's native `imgui64.hdll` (see `../README.md`).

`imgui.ImGui` is the one type a mod needs to import - every ImGui widget is a static function on
it (`ImGui.button(...)`, `ImGui.sliderFloat(...)`, `ImGui.beginTable(...)`, ...), generated from
cimgui's own API metadata (`../native/codegen/generate.mts`) plus a small hand-written lifecycle
layer. Everything is `abstract`/`inline`, so there's no runtime cost beyond the native calls
themselves.

## Using it in a mod

```
haxelib git hl-imgui https://github.com/laymain/farever-mods.git main imgui/hl-imgui/src
```

then add `-lib hl-imgui` to the mod's `compile.hxml`. Frame lifecycle is handled for you: just by
depending on `hl-imgui`, a mod also gets `imgui.ImGuiFrame` compiled in, which hooks
`h3d.impl.DX12Driver.present` once and drives `ImGui.init`/`initWin32`/`newFrame`/`render`/
`renderDrawData` automatically, every real frame - a mod never calls any of those itself. All a mod
does is register a draw callback:

```haxe
ImGui.register("my-mod", panel.draw);
```

`ImGui.register(name, draw)` hands a `Void->Void` callback to a native, cross-mod registry (kept
alive via HashLink's `hl_add_root` until a matching `ImGui.unregister(name)` - most mods never need
to call that, a registered panel just lives for the process). Every registered callback runs once
per real frame, from inside the single shared `ImGuiFrame` hook every hl-imgui-consuming mod gets
for free - `name` only needs to be unique among mods that also use hl-imgui.

A minimal mod:

```haxe
package mymod;

import imgui.ImGui;
import imgui.ref.BoolRef;
import imgui.ref.FloatRef;

@:build(hlx.runtime.Mod.build())
class MyMod {
	static function main():Void {
		var panel = new MyPanel();
		ImGui.register(HlxRuntime.moduleName(), panel.draw);
	}
}

class MyPanel {
	var checked = new BoolRef();
	var speed = new FloatRef(1.0);

	public function new() {}

	public function draw():Void {
		if (ImGui.begin("My Panel")) {
			ImGui.checkbox("A checkbox", checked);
			ImGui.sliderFloat("Speed", speed, 0, 10);
		}
		ImGui.end();
	}
}
```

See `../../imgui-demo/` for a complete, working example.

Widget functions take plain Haxe types - `String` for labels and text, no manual byte-buffer
conversion needed. Anything that needs to persist across frames (`MyPanel`'s `checked`/`speed`
above) uses a small wrapper type from `imgui.ref` (`BoolRef`, `FloatRef`, `IntRef`, `DoubleRef`)
instead of a plain field: `checked.get()`/`speed.get()` reads the current value (or just use the
ref directly where a `Bool`/`Single` is expected, e.g. `if (checked) ...` - it converts implicitly).

A plain `Bool`/`Float`/`Int` field silently doesn't work for this - HashLink only writes back to a
local variable at the call site, never to a field, so the widget would appear to update for one
frame and then revert. `imgui.ref.*` gives it a real, stable heap pointer instead.

Multi-component widgets (`sliderFloat3`, `colorEdit4`, ...) take a raw value buffer instead of one
of the ref types above - there's no wrapper for a 2/3/4-component vector. Build one with
`ImGui.v3(x, y, z)`/`ImGui.v4(x, y, z, w)`. `ImGui.vec2(x, y)`/`ImGui.vec4(x, y, z, w)` build an
actual `ImVec2`/`ImVec4` struct instead, for the (much more common) functions taking a
position/size argument by value.

## Bumping the ImGui/cimgui version

See `../native/third_party/VERSIONS.md`. After re-vendoring, run `node native/codegen/generate.mts`
from `imgui/` and review the diff in `ImGui.hx` (only the text between its `GENERATED`/`END
GENERATED` marker pairs should change), `{Enums,Structs}.hx`, and
`../native/src/generated/imgui_prims.cpp` before committing. See `../NATIVE.md` for how the
generator decides naming/wrapping if you need to extend it.
