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
#include "imgui_theme.h"

// Single-TU amalgamated build, per the header's own documented usage - this is the only place in
// the plugin that includes xxhash.h, so there's no ODR risk from defining the implementation here.
// XXH_STATIC_LINKING_ONLY must be defined alongside XXH_IMPLEMENTATION (not just the latter): the
// implementation itself needs the real XXH32/64/3_state_s struct layouts, which the header only
// emits under that macro (confirmed by a real MSVC build - the header's own doc comment implies
// XXH_IMPLEMENTATION alone is enough, it isn't).
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "../third_party/xxhash/xxhash.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <cstring>

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

// Every consuming mod compiles its own private copy of imgui.ImGuiFrame (mods don't share Haxe
// statics), and each copy would otherwise register its own hook on the game's present() - not
// just redundant, but the actual cause of a GPU-visible rendering corruption bug that only
// reproduced with 2+ hl-imgui-consuming mods loaded together (root-caused to the shared hook
// dispatcher fanning out to N contributors, and therefore N reflective Haxe calls, every single
// frame, instead of 1). ImGuiFrame.ensureRegistered() calls this before registering at all - only
// the first mod to call it, process-wide, gets true, so only one hook is ever installed no matter
// how many mods depend on this library.
static bool g_presentHookClaimed = false;

HL_PRIM bool HL_NAME(claim_present_hook)() {
	if (g_presentHookClaimed) return false;
	g_presentHookClaimed = true;
	return true;
}

DEFINE_PRIM(_BOOL, claim_present_hook, _NO_ARG);

// The only shader-visible SRV heap in this plugin - font atlas at slot 0 (exactly as before
// texture support existed), slots 1..SRV_HEAP_CAPACITY-1 as a per-real-frame working set of
// whichever distinct textures actually get drawn that frame (see g_frameTextureSlots below).
// SetDescriptorHeaps binds this heap exactly once per frame (run_frame/render_draw_data) and is
// never called anywhere else - deliberately, since switching a shader-visible heap mid-command-
// list (this plugin appends its draws onto a command list the game already recorded its own scene
// into, rather than owning it from the start) empirically causes GPU-visible corruption on this
// hardware once the switched-to heap is large, and separately, still corrupts even when every
// switch is to a heap with only one descriptor - so the fix isn't "keep each heap small", it's
// "never switch heaps more than once per frame" at all. Texture *storage* is unlimited (see
// TextureEntry) - only how many distinct textures can be visible in one single frame is capped by
// this heap's size, a much smaller and more natural limit than a global texture-count cap.
static const UINT SRV_HEAP_CAPACITY = 64;
static ID3D12DescriptorHeap *g_srvHeap = nullptr;
static UINT g_srvDescriptorSize = 0;
static ID3D12Device *g_device = nullptr;

// Persistent, unbounded texture storage - resource plus a CPU-only (not shader-visible) heap
// holding its SRV. Never touches g_srvHeap or SetDescriptorHeaps at all; image() below copies
// (CopyDescriptorsSimple, a plain descriptor-content write, not a heap bind) from here into
// g_srvHeap's per-frame working set only for whichever textures actually get drawn.
struct TextureEntry {
	ID3D12Resource *resource;
	ID3D12DescriptorHeap *cpuHeap;
};
static std::unordered_map<int64, TextureEntry> g_textures;
static int64 g_nextTextureId = 1; // 0 reserved as the "invalid" sentinel

// Which g_srvHeap slot (if any) each texture currently occupies *this real frame* - reset by
// run_frame/render_draw_data before any panel draws, so slots are freely reused frame to frame.
// A texture drawn multiple times in one frame reuses its already-assigned slot instead of copying
// its descriptor again.
static std::unordered_map<int64, UINT> g_frameTextureSlots;
static UINT g_nextFreeSlot = 1; // slot 0 is the font atlas

// Content-addressed dedup cache: several mods each keep their own private Haxe-side cache and
// have no visibility into one another, so two mods registering the same underlying image (e.g.
// the same UI atlas) would otherwise both pay for a full GPU upload and a heap slot. width/height
// are folded into the key (not just the hash) as cheap insurance against a hash collision between
// differently-sized images, on top of XXH3's own already-astronomically-low collision odds.
struct TextureCacheKey {
	uint64_t hash;
	int width;
	int height;
	bool operator==(const TextureCacheKey &other) const {
		return hash == other.hash && width == other.width && height == other.height;
	}
};

struct TextureCacheKeyHash {
	size_t operator()(const TextureCacheKey &key) const {
		return std::hash<uint64_t>()(key.hash) ^ ((size_t)key.width << 1) ^ ((size_t)key.height << 17);
	}
};

static std::unordered_map<TextureCacheKey, int64, TextureCacheKeyHash> g_textureCache;

static ID3D12CommandQueue *g_uploadQueue = nullptr;
static ID3D12CommandAllocator *g_uploadAllocator = nullptr;
static ID3D12GraphicsCommandList *g_uploadCmdList = nullptr;
static ID3D12Fence *g_uploadFence = nullptr;
static UINT64 g_uploadFenceValue = 0;
static HANDLE g_uploadFenceEvent = nullptr;

// Texture registration is a rare, one-shot-per-texture operation (callers cache the result), so a
// private queue with a synchronous wait per upload is fine - there's no other command queue in
// this plugin to reuse.
static bool ensureUploadQueue() {
	if (g_uploadQueue) return true;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_uploadQueue)))) return false;
	if (FAILED(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_uploadAllocator)))) return false;
	if (FAILED(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_uploadAllocator, nullptr, IID_PPV_ARGS(&g_uploadCmdList)))) return false;
	g_uploadCmdList->Close();
	if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_uploadFence)))) return false;
	g_uploadFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	return g_uploadFenceEvent != nullptr;
}

static void waitForUploadToComplete() {
	g_uploadFenceValue++;
	g_uploadQueue->Signal(g_uploadFence, g_uploadFenceValue);
	if (g_uploadFence->GetCompletedValue() < g_uploadFenceValue) {
		g_uploadFence->SetEventOnCompletion(g_uploadFenceValue, g_uploadFenceEvent);
		WaitForSingleObject(g_uploadFenceEvent, INFINITE);
	}
}

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

// -- bundled Unicode font + user-dropped fallback fonts (Latin + Latin Extended + Greek +
// Cyrillic shipped by default, deliberately NOT CJK - see native/fonts/NotoSans-Regular.ttf's own
// provenance; a player who needs another script drops their own font file into this same fonts/
// folder, see README.md's font section) -----------------------------------------------------------
//
// This plugin has no logging of its own (grepped: no hlx_log/OutputDebugString/etc anywhere in
// this file or imgui_theme.cpp) - the comments below are the only "log" a load failure gets, on
// the theory that the graceful fallback (ImGui's own built-in default font, ASCII/Latin-1 only)
// is a silent degrade, not a crash, so it doesn't warrant inventing a logging path just for this.

// Unicode block ranges (verified against the standard block chart), deliberately wider than
// ImGui's own GetGlyphRangesDefault() (Basic Latin + Latin-1 Supplement only):
//   0x0020-0x00FF  Basic Latin + Latin-1 Supplement
//   0x0100-0x024F  Latin Extended-A + Latin Extended-B
//   0x0370-0x03FF  Greek and Coptic
//   0x0400-0x052F  Cyrillic + Cyrillic Supplement
//   0x2000-0x206F  General Punctuation (smart quotes, dashes, ellipsis - common in any of the
//                  above scripts' real-world text, not just CJK)
// A static ImWchar pair array terminated by 0,0 (ImFontAtlas::AddFontFromFileTTF's own documented
// glyph_ranges shape) is simpler than building one with ImFontGlyphRangesBuilder for a fixed,
// known-in-advance set of blocks like this. This is the PRIMARY font's range only, restricted
// deliberately - see kFallbackGlyphRanges below for why merged fallback fonts get a much wider one.
static const ImWchar kUnicodeGlyphRanges[] = {
	0x0020, 0x00FF,
	0x0100, 0x024F,
	0x0370, 0x03FF,
	0x0400, 0x052F,
	0x2000, 0x206F,
	0, 0,
};

// Glyph range for the one hardcoded fallback font below (kFallbackFontFileName). Noto Sans CJK's
// regional variants (SC/TC/JP/KR) all ship the same pan-CJK glyph repertoire - Han ideographs,
// Hiragana, Katakana, AND Hangul - differing only in the default shape preferred for a handful of
// regionally-ambiguous Han characters, not in which scripts are covered. So this range is correct
// for the SC variant we hardcode below despite covering Korean/Japanese-specific blocks too.
// ImFontConfig::MergeMode (imgui.h) only fills in codepoints the destination ImFont doesn't
// already have, so requesting this whole range is safe regardless: it can never override/corrupt
// the primary's already-loaded Latin/Greek/Cyrillic glyphs, only add glyphs the primary lacks.
//   0x1100-0x11FF  Hangul Jamo
//   0x3000-0x303F  CJK Symbols and Punctuation
//   0x3040-0x309F  Hiragana
//   0x30A0-0x30FF  Katakana
//   0x3400-0x4DBF  CJK Unified Ideographs Extension A
//   0x4E00-0x9FFF  CJK Unified Ideographs (the main ~21000-character block - matches the "full
//                  set" GetGlyphRangesChineseFull()'s own doc comment refers to)
//   0xAC00-0xD7A3  Hangul Syllables
//   0xF900-0xFAFF  CJK Compatibility Ideographs
//   0xFF00-0xFFEF  Halfwidth and Fullwidth Forms (common CJK punctuation/spacing variants)
static const ImWchar kFallbackGlyphRanges[] = {
	0x1100, 0x11FF,
	0x3000, 0x303F,
	0x3040, 0x309F,
	0x30A0, 0x30FF,
	0x3400, 0x4DBF,
	0x4E00, 0x9FFF,
	0xAC00, 0xD7A3,
	0xF900, 0xFAFF,
	0xFF00, 0xFFEF,
	0, 0,
};

// Exact file name of the PRIMARY font within the fonts/ directory below - always loaded first,
// always with the restricted kUnicodeGlyphRanges above. Shipped by this plugin (native/fonts/).
static const char *kPrimaryFontFileName = "NotoSans-Regular.ttf";

// Exact file name of the one supported FALLBACK font. Deliberately a single hardcoded name, not a
// scan of the fonts/ directory for arbitrary files - once the file is hardcoded, so is the correct
// glyph range for it (kFallbackGlyphRanges above), instead of having to guess a range wide enough
// for "whatever a player might drop in". Not shipped by this plugin - a player who wants Chinese/
// Japanese/Korean chat text to render downloads Noto Sans CJK SC themselves and places it here
// (see README.md's "Fonts for other scripts" section) - keeps this plugin's own download small.
// ".otf", not ".ttf": confirmed the real file Google/Noto actually distributes for this (e.g.
// notofonts/noto-cjk's own repo tree, Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf) is an
// OTF container - AddFontFromFileTTF reads whichever real format is inside regardless of its own
// name, but this exact-match check needs to be the real file name or it silently never matches.
static const char *kFallbackFontFileName = "NotoSansCJKsc-Regular.otf";

// The fonts/ directory ships in the SAME directory as this .hdll (hlx/plugins/imgui/fonts/, see
// native/CMakeLists.txt's `install(FILES fonts/NotoSans-Regular.ttf DESTINATION fonts)`), NOT next
// to the game's exe and NOT a hardcoded absolute path - so it must be resolved relative to this
// DLL's own module handle, not GetModuleFileNameA(NULL, ...) (that gives the PROCESS exe's
// directory; see imgui_theme.cpp's GetThemeConfigPath, which deliberately wants that instead, for
// hlx/config/imgui/theme.conf). This plugin doesn't link hlx-boot, so there's no shared "find my
// own directory" helper to reuse - this is a small self-contained local copy of that idea, same
// convention already established elsewhere in this file (e.g. toStdStringUtf8 above).
static bool GetFontsDirectory(char *outPath, size_t outPathSize) {
	HMODULE hModule = nullptr;
	// GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS resolves the module containing this very function's
	// own code address - i.e. always this .hdll, regardless of which process/module called into
	// it. The FLAG_FROM_ADDRESS variant still increments the module's refcount same as a normal
	// GetModuleHandle, so it's paired with FreeLibrary below (safe: it's not truly unloading
	// anything, this module's real refcount is already held elsewhere for as long as it's mapped).
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			(LPCSTR)&GetFontsDirectory, &hModule) || !hModule)
		return false;

	char path[MAX_PATH];
	DWORD len = GetModuleFileNameA(hModule, path, MAX_PATH);
	FreeLibrary(hModule);
	if (len == 0 || len >= MAX_PATH) return false;

	char *slash = strrchr(path, '\\');
	if (slash) slash[1] = 0; else path[0] = 0;

	if (strcpy_s(outPath, outPathSize, path) != 0) return false;
	return strcat_s(outPath, outPathSize, "fonts\\") == 0;
}

// AddFontFromFileTTF's own size_pixels convention (no existing convention elsewhere in this
// codebase to match - PanelTheme/imgui_theme.cpp only ever touch style metrics, never font size)
// - 16.0f reads comfortably at typical UI scale without looking oversized against the rest of the
// default-density theme this plugin already applies (see ApplyDefaultTheme's WindowPadding/
// ItemSpacing et al, all sized for a similar visual scale). Fallback fonts share the same size so
// their merged glyphs line up with the primary font's baseline/height.
static const float kFontSizePixels = 16.0f;

// Called from init() before Fonts->Build(), per AddFontFromFileTTF's own documented lifecycle
// requirement. Loads kPrimaryFontFileName as the PRIMARY font (restricted kUnicodeGlyphRanges),
// then attempts kFallbackFontFileName as a FALLBACK merged into the same logical ImFont
// (ImFontConfig::MergeMode, imgui.h; kFallbackGlyphRanges) - not PushFont/PopFont switching,
// genuine per-codepoint fallback within one font. No directory scan: both are exact, hardcoded
// file names, checked by just trying to load them.
//
// Failure is handled by simply not adding that font: AddFontFromFileTTF degrades gracefully on a
// missing/bad path (IM_ASSERT_USER_ERROR is a no-op assert() under Release/NDEBUG, and it still
// returns nullptr either way) - if the fallback file isn't present (the common case; it's not
// shipped by this plugin, see kFallbackFontFileName's own comment), CJK/etc. glyphs just aren't
// available and the primary is unaffected. If the PRIMARY fails to load, the fallback attempt is
// skipped too (early return below), and if NO font at all ends up loaded, ImFontAtlasBuildMain
// (imgui_draw.cpp) unconditionally calls atlas->AddFontDefault() whenever atlas->Sources is still
// empty at Build() time, so the net effect is exactly today's pre-existing behavior (ImGui's
// built-in ASCII/Latin-1 default font) rather than a crash.
static void LoadFonts() {
	char fontsDir[MAX_PATH];
	if (!GetFontsDirectory(fontsDir, MAX_PATH)) return;

	char primaryPath[MAX_PATH];
	if (strcpy_s(primaryPath, MAX_PATH, fontsDir) != 0) return;
	if (strcat_s(primaryPath, MAX_PATH, kPrimaryFontFileName) != 0) return;

	ImFont *primaryFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(primaryPath, kFontSizePixels, nullptr, kUnicodeGlyphRanges);
	if (!primaryFont) return;

	char fallbackPath[MAX_PATH];
	if (strcpy_s(fallbackPath, MAX_PATH, fontsDir) != 0) return;
	if (strcat_s(fallbackPath, MAX_PATH, kFallbackFontFileName) != 0) return;

	ImFontConfig fallbackConfig;
	fallbackConfig.MergeMode = true;
	ImGui::GetIO().Fonts->AddFontFromFileTTF(fallbackPath, kFontSizePixels, &fallbackConfig, kFallbackGlyphRanges);
}

HL_PRIM void HL_NAME(init)(ID3D12Device *device, int numFramesInFlight, int rtvFormat) {
	if (g_contextInitialized) return;
	g_contextInitialized = true;

	g_device = device;
	g_device->AddRef();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ApplyDefaultTheme();
	LoadOrInitThemeConfig();

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
	heapDesc.NumDescriptors = SRV_HEAP_CAPACITY;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_srvHeap));
	g_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// The legacy single-descriptor Init overload (imgui_impl_dx12.h, guarded by
	// IMGUI_DISABLE_OBSOLETE_FUNCTIONS which this build leaves undefined) - the current
	// ImGui_ImplDX12_InitInfo-struct API additionally wants a command queue purely for its own
	// internal texture-upload bookkeeping, which this plugin's render-hookup model (mods drive
	// newFrame/render themselves, see farever-mods' approved plan for this plugin) has no natural
	// single owner for.
	ImGui_ImplDX12_Init(device, numFramesInFlight, (DXGI_FORMAT)rtvFormat, g_srvHeap,
		g_srvHeap->GetCPUDescriptorHandleForHeapStart(), g_srvHeap->GetGPUDescriptorHandleForHeapStart());

	// Must run before Fonts->Build() below, per AddFontFromFileTTF's own documented lifecycle - see
	// LoadFonts's own comment for the fallback behavior if this fails to find/load any font.
	LoadFonts();

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
	for (auto &entry : g_textures) {
		entry.second.resource->Release();
		entry.second.cpuHeap->Release();
	}
	g_textures.clear();
	g_textureCache.clear();
	g_frameTextureSlots.clear();
	g_nextFreeSlot = 1;
	if (g_uploadCmdList) { g_uploadCmdList->Release(); g_uploadCmdList = nullptr; }
	if (g_uploadAllocator) { g_uploadAllocator->Release(); g_uploadAllocator = nullptr; }
	if (g_uploadQueue) { g_uploadQueue->Release(); g_uploadQueue = nullptr; }
	if (g_uploadFence) { g_uploadFence->Release(); g_uploadFence = nullptr; }
	if (g_uploadFenceEvent) { CloseHandle(g_uploadFenceEvent); g_uploadFenceEvent = nullptr; }
	g_uploadFenceValue = 0;
	if (g_device) { g_device->Release(); g_device = nullptr; }
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
	// ImGui_ImplDX12_RenderDrawData never binds g_srvHeap itself - see run_frame's identical call
	// below for why that matters.
	commandList->SetDescriptorHeaps(1, &g_srvHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

DEFINE_PRIM(_VOID, render_draw_data, _RESOURCE);

// -- textures --------------------------------------------------------------------------------

// Hashes only the real width * 4 RGBA bytes of each row, skipping any trailing srcRowPitch
// padding - two images with identical pixel content but different (uninitialized) padding must
// still hash identically. Streamed row-by-row rather than copied into one contiguous buffer first.
static uint64_t hashTextureContent(vbyte *pixels, int width, int height, int srcRowPitch) {
	XXH3_state_t *state = XXH3_createState();
	XXH3_64bits_reset(state);
	size_t rowBytes = (size_t)width * 4;
	for (int row = 0; row < height; row++)
		XXH3_64bits_update(state, pixels + (size_t)row * srcRowPitch, rowBytes);
	uint64_t hash = XXH3_64bits_digest(state);
	XXH3_freeState(state);
	return hash;
}

// pixels must already be RGBA8, row-major, with row pitch srcRowPitch (may exceed width * 4 -
// e.g. hxd.Pixels.stride can include row padding). Registration is one-shot per texture and
// never per-frame, so this uploads and waits synchronously rather than sharing a command list
// with the game's own render hook. Identical pixel content (even across different mods, or
// repeated calls for the same asset) is deduplicated via g_textureCache - callers don't need to
// avoid redundant calls themselves.
HL_PRIM int64 HL_NAME(register_texture)(vbyte *pixels, int width, int height, int srcRowPitch) {
	TextureCacheKey key = {hashTextureContent(pixels, width, height, srcRowPitch), width, height};
	auto cached = g_textureCache.find(key);
	if (cached != g_textureCache.end()) return cached->second;

	if (!g_device || !ensureUploadQueue()) return 0;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = (UINT64)width;
	texDesc.Height = (UINT)height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES defaultHeapProps = {};
	defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	ID3D12Resource *texture = nullptr;
	if (FAILED(g_device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture))))
		return 0;

	UINT64 uploadSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	UINT numRows = 0;
	UINT64 rowSizeInBytes = 0;
	g_device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadSize);

	D3D12_HEAP_PROPERTIES uploadHeapProps = {};
	uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = uploadSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource *staging = nullptr;
	if (FAILED(g_device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&staging)))) {
		texture->Release();
		return 0;
	}

	uint8_t *mapped = nullptr;
	staging->Map(0, nullptr, (void **)&mapped);
	UINT copyRowBytes = (UINT)width * 4; // dst row content is exactly width RGBA8 pixels; src row may have trailing padding beyond that (srcRowPitch)
	for (UINT row = 0; row < numRows; row++)
		memcpy(mapped + (size_t)row * footprint.Footprint.RowPitch, pixels + (size_t)row * srcRowPitch, copyRowBytes);
	staging->Unmap(0, nullptr);

	g_uploadAllocator->Reset();
	g_uploadCmdList->Reset(g_uploadAllocator, nullptr);

	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = texture;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.pResource = staging;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint = footprint;

	g_uploadCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	g_uploadCmdList->ResourceBarrier(1, &barrier);

	g_uploadCmdList->Close();
	ID3D12CommandList *lists[] = {g_uploadCmdList};
	g_uploadQueue->ExecuteCommandLists(1, lists);
	waitForUploadToComplete();

	staging->Release();

	// CPU-only (not shader-visible) - this texture's descriptor never gets bound to the GPU on its
	// own; image() below copies it into g_srvHeap's per-frame working set only when actually drawn.
	D3D12_DESCRIPTOR_HEAP_DESC cpuHeapDesc = {};
	cpuHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cpuHeapDesc.NumDescriptors = 1;
	cpuHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ID3D12DescriptorHeap *cpuHeap = nullptr;
	if (FAILED(g_device->CreateDescriptorHeap(&cpuHeapDesc, IID_PPV_ARGS(&cpuHeap)))) {
		texture->Release();
		return 0;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	g_device->CreateShaderResourceView(texture, &srvDesc, cpuHeap->GetCPUDescriptorHandleForHeapStart());

	int64 texId = g_nextTextureId++;
	g_textures[texId] = {texture, cpuHeap};
	g_textureCache[key] = texId;
	return texId;
}

DEFINE_PRIM(_I64, register_texture, _BYTES _I32 _I32 _I32);

// texId is an opaque handle from register_texture above (not itself a GPU value - see
// TextureEntry). Assigns this texture a slot in g_srvHeap's per-frame working set (reusing its
// existing slot if already drawn earlier this same frame), then draws it - no SetDescriptorHeaps
// call here at all, just a CopyDescriptorsSimple into an already-bound heap when a slot is newly
// assigned, which is the whole point of this design (see g_srvHeap's own comment).
HL_PRIM void HL_NAME(image)(int64 texId, float w, float h, float u0, float v0, float u1, float v1) {
	auto textureIt = g_textures.find(texId);
	if (textureIt == g_textures.end()) return;

	UINT slot;
	auto slotIt = g_frameTextureSlots.find(texId);
	if (slotIt != g_frameTextureSlots.end()) {
		slot = slotIt->second;
	} else {
		if (g_nextFreeSlot >= SRV_HEAP_CAPACITY) return; // this frame's working set is full - skip rather than corrupt
		slot = g_nextFreeSlot++;
		D3D12_CPU_DESCRIPTOR_HANDLE dst = g_srvHeap->GetCPUDescriptorHandleForHeapStart();
		dst.ptr += (SIZE_T)slot * g_srvDescriptorSize;
		g_device->CopyDescriptorsSimple(1, dst, textureIt->second.cpuHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_frameTextureSlots[texId] = slot;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = g_srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += (UINT64)slot * g_srvDescriptorSize;
	ImTextureRef texRef((ImTextureID)gpuHandle.ptr);
	ImGui::Image(texRef, ImVec2(w, h), ImVec2(u0, v0), ImVec2(u1, v1));
}

DEFINE_PRIM(_VOID, image, _I64 _F32 _F32 _F32 _F32 _F32 _F32);

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

	// Reset before any panel draws (the hl_dyn_call loop below, where image() runs) so this real
	// frame's working set starts empty - see g_frameTextureSlots' own comment.
	g_frameTextureSlots.clear();
	g_nextFreeSlot = 1;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	for (auto &entry : g_entries) {
		// One mod's panel throwing an uncaught Haxe exception must never take every OTHER mod's
		// panel down with it, nor skip this function's own cleanup below (Render()/
		// RenderDrawData()/the g_frameOpenThisTick reset) - HL's exception mechanism is a
		// longjmp-style unwind (hl_setup.throw_jump) that skips past this C++ frame entirely
		// (no destructors, no fall-through to the code after this loop) and lands wherever the
		// nearest Haxe try/catch up the call stack is - for the present() hook that's
		// Dispatcher.dispatch's own try/catch (hlx-loader), several frames higher than this
		// function. Left unguarded, that means g_frameOpenThisTick above never gets reset to
		// false, permanently wedging run_frame() into a no-op (see its own check at the top of
		// this function) - i.e. ImGui stops rendering for every mod, forever, for the rest of the
		// session, the moment any ONE registered panel throws once. Mirrors hlx-boot's own
		// call_closure (reflection.c) - the identical __try/__except around the identical
		// hl_dyn_call, just not yet applied here too.
		__try {
			hl_dyn_call(entry->draw, nullptr, 0);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			// Swallow and move on: the throwing panel simply doesn't render this frame, every
			// other panel and this function's own end-of-frame cleanup still run normally.
		}
	}

	// ImGui::Render() triggers EndFrame() if it hasn't already run this frame, and EndFrame()
	// does its own ErrorRecoveryTryToRecoverState pass against the real per-frame baseline
	// captured in NewFrame() (imgui.cpp: NewFrame()/EndFrame()) - so one mod panel's unmatched
	// Begin/End or PushID/PopID mistake still gets forcibly closed out before the frame ends,
	// without this loop needing to (mis)manage recovery state itself between panels.
	ImGui::Render();

	// ImGui_ImplDX12_RenderDrawData never binds g_srvHeap itself - it only stores the pointer from
	// Init(), the caller must SetDescriptorHeaps it. Without this, SetGraphicsRootDescriptorTable
	// calls inside RenderDrawData resolve against whatever heap the game's own earlier draws on
	// this command list left bound instead, corrupting unrelated GPU state. Safe to do here every
	// frame: cmdList is Reset() by the game before its own next frame, so this doesn't leak forward.
	cmdList->SetDescriptorHeaps(1, &g_srvHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

	g_frameOpenThisTick = false;
}

DEFINE_PRIM(_VOID, run_frame, _RESOURCE);

// Persistent-pointer variants of every widget with an hl.Ref<T> parameter (igCheckbox_Ptr,
// igSliderFloat_Ptr, igBegin_Ptr, ...) are generated automatically, not hand-written here - see
// native/codegen/generate.mts's hasRefArg/toPointerVariantArgs and
// hl-imgui/README.md's "Persisting widget state in a field".

// ImGuiTableSortSpecs::Specs/SpecsCount/SpecsDirty are plain struct fields, not cimgui functions -
// igTableGetSortSpecs() itself (returning the opaque struct pointer) is generated, since it's a
// real cimgui function, but the generator only binds cimgui's flat function list, so it has
// nothing to bind for reading what that pointer points to. Hand-written here instead.
HL_PRIM int HL_NAME(ImGuiTableSortSpecs_GetSpecsCount)(ImGuiTableSortSpecs *self) {
	return self ? self->SpecsCount : 0;
}
DEFINE_PRIM(_I32, ImGuiTableSortSpecs_GetSpecsCount, _ABSTRACT(ImGuiTableSortSpecs));

HL_PRIM bool HL_NAME(ImGuiTableSortSpecs_GetSpecsDirty)(ImGuiTableSortSpecs *self) {
	return self ? self->SpecsDirty : false;
}
DEFINE_PRIM(_BOOL, ImGuiTableSortSpecs_GetSpecsDirty, _ABSTRACT(ImGuiTableSortSpecs));

HL_PRIM void HL_NAME(ImGuiTableSortSpecs_SetSpecsDirty)(ImGuiTableSortSpecs *self, bool dirty) {
	if (self) self->SpecsDirty = dirty;
}
DEFINE_PRIM(_VOID, ImGuiTableSortSpecs_SetSpecsDirty, _ABSTRACT(ImGuiTableSortSpecs) _BOOL);

HL_PRIM int HL_NAME(ImGuiTableSortSpecs_GetColumnIndex)(ImGuiTableSortSpecs *self, int i) {
	if (!self || !self->Specs || i < 0 || i >= self->SpecsCount) return -1;
	return self->Specs[i].ColumnIndex;
}
DEFINE_PRIM(_I32, ImGuiTableSortSpecs_GetColumnIndex, _ABSTRACT(ImGuiTableSortSpecs) _I32);

HL_PRIM int HL_NAME(ImGuiTableSortSpecs_GetSortDirection)(ImGuiTableSortSpecs *self, int i) {
	if (!self || !self->Specs || i < 0 || i >= self->SpecsCount) return ImGuiSortDirection_None;
	return (int)self->Specs[i].SortDirection;
}
DEFINE_PRIM(_I32, ImGuiTableSortSpecs_GetSortDirection, _ABSTRACT(ImGuiTableSortSpecs) _I32);

// igInputText's real callback/user_data params are dropped by the codegen (generate.mts's
// resolveBase has no HL type tag for a function-pointer arg), so ImGuiInputTextFlags_
// CallbackCompletion (fired when the user presses Tab while the widget is active) can never reach
// a Haxe closure through the generated igInputText binding - it's hardcoded to NULL, NULL there.
//
// The completion closure below is a plain Void->Void, deliberately not one taking or returning the
// real ImGuiInputTextCallbackData* - hl_dyn_call's args/return are vdynamic*, and boxing a raw
// pointer into one by hand means constructing an hl_type ourselves (hl_alloc_dynamic(t) + v.ptr,
// same shape as hlx-boot/src/reflection.c's box_dynamic_ptr), which needs a real hl_type matching
// whatever the Haxe side's hl.Abstract<"..."> compiles to - not the risk worth taking here when a
// Void->Void closure (exactly register()'s panel-draw shape above, already proven live) sidesteps
// it entirely. Whatever text the closure decides to complete to, it hands back through
// setCompletionText() below instead - an ordinary native-bound vbyte* parameter, the same
// well-proven marshaling every other @:hlNative(..., _BYTES) call in this file already uses.
static char g_completionScratch[256];

HL_PRIM void HL_NAME(setCompletionText)(vbyte *textUtf8) {
	strcpy_s(g_completionScratch, sizeof(g_completionScratch), textUtf8 ? (const char *)textUtf8 : "");
}
DEFINE_PRIM(_VOID, setCompletionText, _BYTES);

static int InputTextCompletionTrampoline(ImGuiInputTextCallbackData *data) {
	vclosure *onCompletion = (vclosure *)data->UserData;
	if (!onCompletion) return 0;

	g_completionScratch[0] = 0;
	__try {
		hl_dyn_call(onCompletion, nullptr, 0);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// Swallow, per run_frame's own established pattern - one mod's callback throwing must not
		// take down text input for the whole window.
		return 0;
	}

	size_t len = strlen(g_completionScratch);
	if (len == 0) return 0;

	data->DeleteChars(0, data->BufTextLen);
	data->InsertChars(0, g_completionScratch, g_completionScratch + len);
	return 0;
}

HL_PRIM bool HL_NAME(inputTextWithCompletion)(vbyte *label, vbyte *buf, int64 buf_size, int flags, vclosure *onCompletion) {
	return (bool)ImGui::InputText((const char *)label, (char *)buf, (size_t)buf_size,
		(ImGuiInputTextFlags)flags | ImGuiInputTextFlags_CallbackCompletion,
		InputTextCompletionTrampoline, (void *)onCompletion);
}
DEFINE_PRIM(_BOOL, inputTextWithCompletion, _BYTES _BYTES _I64 _I32 _FUN(_VOID, _NO_ARG));

// ImGuiListClipper's constructor/destructor are among the ~112 cimgui excludes
// native/codegen/generate.mts's compileConstructorsAndDestructors deliberately skips (real
// placement-new via IM_NEW, not a plain callable cimgui's JSON can express - same as every other
// stateful cimgui type in this plugin, e.g. ImGuiTextFilter/ImDrawListSplitter, none of which have
// a generated constructor either). Named to match cimgui's own ov_cimguiname for this pair
// (ImGuiListClipper_ImGuiListClipper / ImGuiListClipper_destroy) rather than inventing new/delete
// names, so it reads as "the constructor the generator would have produced" rather than a
// different convention.
HL_PRIM ImGuiListClipper *HL_NAME(ImGuiListClipper_ImGuiListClipper)() {
	return IM_NEW(ImGuiListClipper)();
}
DEFINE_PRIM(_ABSTRACT(ImGuiListClipper), ImGuiListClipper_ImGuiListClipper, _NO_ARG);

HL_PRIM void HL_NAME(ImGuiListClipper_destroy)(ImGuiListClipper *self) {
	IM_DELETE(self);
}
DEFINE_PRIM(_VOID, ImGuiListClipper_destroy, _ABSTRACT(ImGuiListClipper));

HL_PRIM int HL_NAME(ImGuiListClipper_get_DisplayStart)(ImGuiListClipper *self) {
	return self->DisplayStart;
}
DEFINE_PRIM(_I32, ImGuiListClipper_get_DisplayStart, _ABSTRACT(ImGuiListClipper));

HL_PRIM int HL_NAME(ImGuiListClipper_get_DisplayEnd)(ImGuiListClipper *self) {
	return self->DisplayEnd;
}
DEFINE_PRIM(_I32, ImGuiListClipper_get_DisplayEnd, _ABSTRACT(ImGuiListClipper));

HL_PRIM int HL_NAME(ImGuiListClipper_get_ItemsCount)(ImGuiListClipper *self) {
	return self->ItemsCount;
}
DEFINE_PRIM(_I32, ImGuiListClipper_get_ItemsCount, _ABSTRACT(ImGuiListClipper));

// WantCaptureMouse/WantCaptureKeyboard are plain ImGuiIO struct fields, not cimgui functions, so
// the generator has nothing to bind them from - same hand-written-getter carve-out as
// ImGuiListClipper's DisplayStart/DisplayEnd above. Not currently called by any mod (ChatPanel.hx
// uses isAnyItemActive/isAnyItemFocused/isPopupOpen instead, since these two go true from mere
// hover) - kept as general-purpose plugin API for whatever might want raw hover-capture state.
HL_PRIM bool HL_NAME(ImGuiIO_get_WantCaptureMouse)(ImGuiIO *self) {
	return self->WantCaptureMouse;
}
DEFINE_PRIM(_BOOL, ImGuiIO_get_WantCaptureMouse, _ABSTRACT(ImGuiIO));

HL_PRIM bool HL_NAME(ImGuiIO_get_WantCaptureKeyboard)(ImGuiIO *self) {
	return self->WantCaptureKeyboard;
}
DEFINE_PRIM(_BOOL, ImGuiIO_get_WantCaptureKeyboard, _ABSTRACT(ImGuiIO));
