#include "pipeline_cache/common.h"
#include "pipeline_cache/driver.h"
#include "pipeline_cache/log.h"
#include "pipeline_cache/store.h"

#include <windows.h>

// Entry point for the whole native module: a persistent, disk-backed cache for the game's D3D12
// pipeline state objects (PSOs), so it doesn't recompile them on every launch. The actual D3D12
// interception is a subordinate concern that exists only to serve this goal - see
// pipeline_cache/driver.h.
//
// NATIVE.md Section 6/Section 7: exports every real "dx12" native under this proxy's own name so
// `hl_module_init_natives` finds a correctly-signed hlp_<name> for each one the game's own
// bytecode resolves (without this, the game aborts - hl_fatal4, loud, by design,
// hashlink/src/module.c - the instant it needs any real "dx12" native, since our module now owns
// that name, Section 3).
//
// Shadow-loading this DLL is hlx-boot's own job: it eagerly LoadLibraryA's every plugins/*.hdll
// by full path, before hlx-loader.hl (and therefore any mod's main()) even runs
// (hlx-core/hlx-boot/src/boot.c's EagerLoadPluginHdlls) - simpler, and wins the race
// deterministically instead of depending on mod main()s running before the game reaches its own
// device setup. This mod's own Haxe side (ShaderPersistentCacheMod.hx) is now just a build-tooling
// anchor with no native calls at all.
//
// This exact set of 98 natives/signatures was generated from the game's own compiled bytecode
// (HLX.Viewer's `funcs dx12` against the real hlboot.dat), cross-checked against the real
// dx12.hdll's own export table (`objdump -p`) - both give exactly 98 dx12_<name>/hlp_<name>
// pairs (196 exports total), matching NATIVE.md Section 2's own count exactly.

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_DETACH) {
		Dx12PsoCache_Shutdown(GProxySelfDir);
		Dx12ShadowLogClose();
		return TRUE;
	}
	if (reason != DLL_PROCESS_ATTACH) return TRUE;

	Dx12CaptureOwnDir((HMODULE)hinstDLL, GProxySelfDir, sizeof(GProxySelfDir));
	Dx12ShadowLogOpen(GProxySelfDir);
	Dx12ShadowLog("dx12_proxy", "DllMain: own module base = %p", (void *)hinstDLL);

	InitDriverBridge(GProxySelfDir);
	Dx12PsoCache_StartFlushThread();

	return TRUE;
}
