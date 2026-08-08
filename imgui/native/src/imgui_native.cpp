// Lifecycle glue the codegen (native/codegen/generate.mts) can't express: context creation, the
// DX12/Win32 backends, and per-frame plumbing. Everything else (widgets, style, ID stack, ...) is
// native/src/generated/imgui_prims.cpp.
#define HL_NAME(n) imgui_##n
#include <hl.h>
// hl.h's own `#define _GUID "g"` (a DEFINE_PRIM abstract-type-signature string) collides by name
// with guiddef.h's real `_GUID` struct tag once <windows.h> is included below - same fix as
// shader-cache/native/src/shadercache.cpp's identical `#undef _GUID`.
#undef _GUID

#ifdef HL_WIN_DESKTOP
#include <windows.h>
#include <d3d12.h>
#include <dxgiformat.h>
#endif

#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_win32.h"
#include "../third_party/imgui/backends/imgui_impl_dx12.h"

#include <string>
#include <vector>
#include <cstdlib>

// Forward-declared here rather than pulled from imgui_impl_win32.h directly - that header
// deliberately leaves it commented out (inside a `#if 0` block) to avoid dragging a <windows.h>
// dependency onto callers who don't need it; we already include <windows.h> above, so we just
// paste the declaration exactly as imgui_impl_win32.h's own comment says to.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define _DEVICE _ABSTRACT(dx_device)
#define _RESOURCE _ABSTRACT(dx_resource)
#define _WINDOW _ABSTRACT(dx_window)

// Same type names/strings shader-cache/src/sc/native/Native.hx already established - a Device or
// Resource a consuming mod obtained from dx12.hdll's own natives (e.g. @:hlNative("dx12",
// "get_device")) passes straight into these without any Dynamic boxing/unboxing, because it's a
// genuine native-to-native handoff on both ends. The same is now also true of values read off a
// farever-gamelib instance (e.g. h3d.impl.DxFrame.commandList, dx.Window.win): HLX.GamelibGenerator
// emits those as real hl.Abstract<"dx_resource">/hl.Abstract<"dx_window"> fields, routed through
// HlxRuntime.resolveAbstract - so they arrive already correctly typed, with no unbox_* helper
// needed on this end at all (see hlx-runtime's HlxRuntime.resolveAbstract for the full mechanism).

static HWND g_hwnd = nullptr;
static WNDPROC g_originalWndProc = nullptr;

static LRESULT CALLBACK HlxImGuiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
	return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}

// Owns a single, dedicated SRV descriptor heap slot for ImGui's own font atlas texture, rather
// than asking every consuming mod to source and hand in a free slot from the HOST engine's own
// heap - farever-gamelib exposes h3d.impl.DxFrame.srvHeap, but figuring out a slot within it that
// isn't already claimed by the game's own rendering requires bookkeeping this plugin has no
// visibility into. A tiny heap of our own sidesteps that coordination problem entirely.
static ID3D12DescriptorHeap *g_srvHeap = nullptr;

// Each of init()/init_win32() below guards itself independently (rather than sharing one flag) so
// a construct() retry after a partial failure (see ImGuiFrame.hx's construct(), which calls both
// in sequence inside one try/catch and retries next frame on any exception) still only re-runs
// whichever of the two genuinely hasn't succeeded yet - not required for correctness (every
// consuming mod's own private Haxe-side `constructed` bool already believes it's the very first
// caller, so without this guard a second mod's copy of construct() would call
// ImGui::CreateContext() again, destroying/leaking the first mod's real context), but cheap
// insurance beyond that. The actual cross-mod safety property lives in run_frame's own
// g_frameOpenThisTick guard below.
static bool g_contextInitialized = false;
static bool g_win32Initialized = false;

HL_PRIM void HL_NAME(init)(ID3D12Device *device, int numFramesInFlight, int rtvFormat) {
	if (g_contextInitialized) return;
	g_contextInitialized = true;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// This is an overlay over the game's own window, not a standalone ImGui app - the game, not
	// ImGui, owns the system cursor. Without this flag, imgui_impl_win32's
	// ImGui_ImplWin32_UpdateMouseCursor calls ::SetCursor() every NewFrame() and on every
	// WM_SETCURSOR message, unconditionally replacing the game's custom cursor icon with a plain
	// arrow and un-hiding it whenever the game hides it (see imgui_impl_win32.cpp's
	// ImGui_ImplWin32_UpdateMouseCursor). Setting NoMouseCursorChange makes that function a no-op
	// and lets WM_SETCURSOR fall through to the game's original WndProc instead.
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_srvHeap));

	// The legacy single-descriptor Init overload (imgui_impl_dx12.h, guarded by
	// IMGUI_DISABLE_OBSOLETE_FUNCTIONS which this build leaves undefined) - the current
	// ImGui_ImplDX12_InitInfo-struct API additionally wants a command queue purely for its own
	// internal texture-upload bookkeeping, which this plugin's render-hookup model (mods drive
	// newFrame/render themselves, see farever-mods' approved plan for this plugin) has no natural
	// single owner for.
	ImGui_ImplDX12_Init(device, numFramesInFlight, (DXGI_FORMAT)rtvFormat, g_srvHeap,
		g_srvHeap->GetCPUDescriptorHandleForHeapStart(), g_srvHeap->GetGPUDescriptorHandleForHeapStart());

	// Confirmed live (access violation inside ImGui::NewFrame, specifically
	// UpdateFontsNewFrame -> GetDefaultFont's lazy ImFontAtlasBuildMain call): imgui.h's own Build()
	// comment says plainly "From 1.92 with backends supporting ImGuiBackendFlags_RendererHasTextures:
	// Calling Build() ... is not needed" - meaning a LEGACY backend like ours (which explicitly
	// clears that flag, see ImGui_ImplDX12_Init's own legacy wrapper) still does. Without this, the
	// first NewFrame() ever called tries to lazily build the atlas at the same moment it's locking
	// all font atlases (since RendererHasTextures is off) - building it explicitly here, before
	// that ever happens, sidesteps the interaction entirely.
	ImGui::GetIO().Fonts->Build();
}

DEFINE_PRIM(_VOID, init, _DEVICE _I32 _I32);

HL_PRIM void HL_NAME(init_win32)(HWND hwnd) {
	if (g_win32Initialized) return;
	g_win32Initialized = true;

	g_hwnd = hwnd;
	ImGui_ImplWin32_Init((void *)g_hwnd);
	g_originalWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)HlxImGuiWndProc);
}

DEFINE_PRIM(_VOID, init_win32, _WINDOW);

HL_PRIM void HL_NAME(shutdown)() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	if (g_hwnd && g_originalWndProc) SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
	g_hwnd = nullptr;
	g_originalWndProc = nullptr;
	if (g_srvHeap) {
		g_srvHeap->Release();
		g_srvHeap = nullptr;
	}
	ImGui::DestroyContext();
}

DEFINE_PRIM(_VOID, shutdown, _NO_ARG);

HL_PRIM void HL_NAME(new_frame)() {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

DEFINE_PRIM(_VOID, new_frame, _NO_ARG);

HL_PRIM void HL_NAME(render)() {
	ImGui::Render();
}

DEFINE_PRIM(_VOID, render, _NO_ARG);

// commandList is whatever the caller's own render hook already has open (e.g. via
// @:hlNative("dx12", "command_list_...") natives, matching the plan's manual/mod-driven render
// hookup) - this only records draw commands into it, it never opens/closes/executes it itself.
HL_PRIM void HL_NAME(render_draw_data)(ID3D12GraphicsCommandList *commandList) {
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

DEFINE_PRIM(_VOID, render_draw_data, _RESOURCE);

// -- cross-mod panel registry + per-real-frame idempotent NewFrame/Render (ImGuiFrame.hx) --------
//
// Every consuming mod compiles its own private copy of hl-imgui's ImGuiFrame class into its own
// separate HL module (mods don't share Haxe statics), and each copy independently installs a
// @:hlx.prefix on the same real h3d.impl.DX12Driver.present - so this native side, not any single
// mod's Haxe code, is what makes a single process-wide ImGui frame/registry actually work: it's
// the only thing genuinely shared across every mod using hl-imgui.

// PanelEntry is heap-allocated (a std::vector<PanelEntry*>, not std::vector<PanelEntry>)
// specifically so hl_add_root's rooted address (&entry->draw) stays valid for the entry's whole
// lifetime - a vector of PanelEntry BY VALUE would relocate its elements (and silently invalidate
// any root taken on one) the moment a later register() call grows the vector past its current
// capacity. The indirection is required, not stylistic.
struct PanelEntry {
	std::string name;
	vclosure *draw;
};
static std::vector<PanelEntry *> g_entries;

// Guards one real present() tick: every mod's own copy of ImGuiFrame.drivePresent calls run_frame
// once per real frame, but only the first one to arrive should actually open/close the ImGui
// frame - the rest are redundant re-entries into the same tick and must no-op.
static bool g_frameOpenThisTick = false;

// register/unregister take hl.Bytes on the Haxe side (ImGui.hx's cstr() helper - the same
// String -> hl.Bytes conversion every other widget call taking string data already goes
// through), so the bytes arriving here are already UTF-8 and NUL-terminated - no hl_to_utf8
// UTF-16 -> UTF-8 conversion is needed (or correct: that call expects HL's native uchar*
// string representation, not raw UTF-8 bytes, and would corrupt this input).
static std::string toStdStringUtf8(vbyte *nameUtf8) {
	return std::string(nameUtf8 == nullptr ? "" : (const char *)nameUtf8);
}

// Shared by register() (which must replace, not duplicate, an existing same-name entry - see the
// unroot/delete/erase-then-push-fresh below) and unregister() (which just wants the entry gone) -
// both erase-by-name loops did the exact same hl_remove_root+delete+erase, so this factors that
// out rather than keeping two copies in sync.
static void eraseEntryByName(const std::string &name) {
	for (auto it = g_entries.begin(); it != g_entries.end(); ++it) {
		if ((*it)->name == name) {
			hl_remove_root(&(*it)->draw);
			delete *it;
			g_entries.erase(it);
			return;
		}
	}
}

// draw is a Haxe Void->Void closure (e.g. a DemoPanel instance's own `draw` method, per
// ImguiDemoMod.hx: `ImGui.register("imgui-demo", panel.draw)`) - hl_add_root roots the fixed
// memory address &entry->draw (not the vclosure's own contents), which is exactly what's needed
// here: without it, nothing else in the process keeps this mod-owned closure reachable from any
// HL module's own GC roots, since the only reference to it now lives in this plugin's native
// global registry, entirely outside any Haxe stack/heap the GC already scans.
//
// Idempotent by name: a re-run of a mod's main() (dev-time hot-reload or any other re-init path)
// calling register() again with the same name must replace the old entry, not append a second one
// - two live entries under the same name would both draw under the same ImGui window title with
// two unrelated BoolRef open-flags, so closing one window leaves the other still open/drawing.
HL_PRIM void HL_NAME(register)(vbyte *nameUtf8, vclosure *draw) {
	std::string name = toStdStringUtf8(nameUtf8);
	eraseEntryByName(name);

	PanelEntry *entry = new PanelEntry();
	entry->name = name;
	entry->draw = draw;
	hl_add_root(&entry->draw);
	g_entries.push_back(entry);
}

DEFINE_PRIM(_VOID, register, _BYTES _FUN(_VOID, _NO_ARG));

HL_PRIM void HL_NAME(unregister)(vbyte *nameUtf8) {
	eraseEntryByName(toStdStringUtf8(nameUtf8));
}

DEFINE_PRIM(_VOID, unregister, _BYTES);

HL_PRIM void HL_NAME(run_frame)(ID3D12GraphicsCommandList *cmdList) {
	if (g_frameOpenThisTick) return;
	g_frameOpenThisTick = true;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	for (auto &entry : g_entries) {
		hl_dyn_call(entry->draw, nullptr, 0);
	}

	// ImGui::Render() triggers EndFrame() if it hasn't already run this frame, and EndFrame()
	// does its own ErrorRecoveryTryToRecoverState pass against the real per-frame baseline
	// captured in NewFrame() (imgui.cpp: NewFrame()/EndFrame()) - so one mod panel's unmatched
	// Begin/End or PushID/PopID mistake still gets forcibly closed out before the frame ends,
	// without this loop needing to (mis)manage recovery state itself between panels.
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

	g_frameOpenThisTick = false;
}

DEFINE_PRIM(_VOID, run_frame, _RESOURCE);

// Persistent-pointer variants of every widget with an hl.Ref<T> parameter (igCheckbox_Ptr,
// igSliderFloat_Ptr, igBegin_Ptr, ...) are generated automatically, not hand-written here - see
// native/codegen/generate.mts's hasRefArg/toPointerVariantArgs and
// hl-imgui/README.md's "Persisting widget state in a field".
