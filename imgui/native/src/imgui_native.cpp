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
		hl_dyn_call(entry->draw, nullptr, 0);
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
