# imgui-demo

A mod for Farever, built on the HLX Modding Framework.

Example/reference mod for `../imgui/` + `../imgui/hl-imgui/` - opens a small tabbed ImGui window
covering most of the available widgets, proving the native plugin + haxelib + a real mod wire
together end to end. Not a real feature - copy this mod's structure as the starting point for a
mod that wants an ImGui-based debug UI of its own:

- `DemoPanel.hx` - owns the UI's own state and widget calls (`ImGui.begin`/`button`/`sliderFloat`/
  etc.). One class per panel is the intended shape - `ImguiDemoMod` itself never calls a widget
  function directly.
- `ImguiDemoMod.hx` - the entire mod entry point: build the panel once, then
  `ImGui.register("imgui-demo", panel.draw)`. That's it - `hl-imgui`'s own `imgui.ImGuiFrame` drives
  the actual frame lifecycle (see `../imgui/hl-imgui/README.md`'s "Using it in a mod" section), so a
  consuming mod never hooks a render function of its own.
- `DarkPastelTheme.hx` - an `imgui.Theme` (push/pop color and style-var pairs) applied around the
  whole panel, to show what styling a real mod's UI looks like beyond ImGui's defaults.

## Building

`npm run build imgui imgui-demo` (or `deploy`) from `farever-mods/` - `imgui-demo` needs `imgui`'s
native `imgui64.hdll` alongside it to actually render (see `../imgui/README.md`).
