# Vendored versions

| Library | Version | Commit |
| --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.92.9 (+1 fix commit) | `a9e7a8c880cf4a7fbf142b13a41bcbb1f43d0938` |
| [cimgui](https://github.com/cimgui/cimgui) | 1.92.9 | tag `1.92.9` |

The Dear ImGui commit is **not** the `v1.92.9` tag itself — it's the exact commit cimgui's
`1.92.9` tag pins as its `imgui` submodule (one commit past the tag, a same-day regression fix
for `ImDrawData::CmdListsCount`). Vendoring that exact commit, not the tag, keeps `cimgui.cpp`'s
calls into Dear ImGui's C++ API binary- and signature-compatible with what cimgui was actually
generated against.

Vendored subset:
- `imgui/`: core (`imgui.{h,cpp}`, `imgui_draw.cpp`, `imgui_internal.h`, `imgui_tables.cpp`,
  `imgui_widgets.cpp`, `imconfig.h`, `imstb_*.h`) + `backends/imgui_impl_{win32,dx12}.{h,cpp}` +
  `imgui_demo.cpp` - the latter isn't needed for anything this plugin itself calls, but
  `cimgui.cpp` unconditionally wraps `ImGui::ShowDemoWindow`/`ShowAboutWindow`/`ShowStyleEditor`/
  `ShowStyleSelector`/`ShowUserGuide`, so omitting it is a link error, not just a missing feature
  (confirmed by a real MSVC build).
- `cimgui/`: `cimgui.{h,cpp}`, `cimconfig.h`, and `generator/output/{definitions,
  structs_and_enums,typedefs_dict}.json` — the machine-readable API metadata
  `native/codegen/generate.mts` reads to emit bindings. No `imgui/` submodule copy — cimgui's own
  `cimgui.cpp` is compiled directly against the `../imgui/` sources above.

## Bumping the version

1. Find the imgui commit a new cimgui tag pins: `git ls-tree <cimgui-tag> imgui`.
2. Re-copy the file sets above from matching checkouts of both repos.
3. Re-run `node ../codegen/generate.mts` and review the diff in generated output before committing.
