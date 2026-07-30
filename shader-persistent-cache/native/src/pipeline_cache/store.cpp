#include "store.h"

#include "common.h"
#include "hash.h"
#include "log.h"

#include <d3d12.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Ported from shader-cache/native/src/shadercache.cpp's own library_create, reused largely as-is
// (NATIVE.md Section 8) - none of that mod's Haxe-reflection problems apply here, since this
// already operates on a real ID3D12Device* (get_device's own captured result), not anything
// reconstructed from Haxe.

static void ReportDxError(HRESULT err, int line) {
	hl_error("DX12PROXY DXERROR %X line %d", (DWORD)err, line);
}

// blob/blobSize come from LoadPersistedCacheBlob - NULL/0 on a fresh install (first run).
static ID3D12PipelineLibrary *SetupPipelineLibrary(ID3D12Device *device, void *blob, int blobSize) {
	ID3D12Device1 *device1 = NULL;
	if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device1))))
		return NULL;

	void *blobPtr = NULL;
	UINT blobSizeU = 0;
	if (blobSize > 0 && blob != NULL) {
		blobPtr = malloc((size_t)blobSize);
		if (blobPtr != NULL) {
			memcpy(blobPtr, blob, (size_t)blobSize);
			blobSizeU = (UINT)blobSize;
		}
	}

	ID3D12PipelineLibrary *lib = NULL;
	HRESULT r = device1->CreatePipelineLibrary(blobPtr, blobSizeU, IID_PPV_ARGS(&lib));
	if (FAILED(r)) {
		// The blob's header encodes the adapter/driver it was serialized against -
		// CreatePipelineLibrary validates that here and fails with one of these three codes if it
		// no longer matches (GPU swap, driver update, or a corrupt/foreign blob) rather than
		// returning garbage. Falling back to an empty library is the documented recovery: the
		// cache just rebuilds from scratch this session, and the next successful persist overwrites
		// the stale blob on disk.
		if (r == D3D12_ERROR_ADAPTER_NOT_FOUND || r == D3D12_ERROR_DRIVER_VERSION_MISMATCH || r == E_INVALIDARG) {
			Dx12ShadowLog("dx12_proxy", "SetupPipelineLibrary: persisted cache invalid for current adapter/driver (hr=%lx), starting fresh", (unsigned long)r);
			r = device1->CreatePipelineLibrary(NULL, 0, IID_PPV_ARGS(&lib));
		}
		if (FAILED(r)) {
			device1->Release();
			ReportDxError(r, __LINE__);
			return NULL;
		}
	}
	device1->Release();
	return lib;
}

// Set once, from Dx12PsoCache_OnDeviceCaptured, once a real device exists - the cache-integrating
// natives below read/write through this.
static ID3D12PipelineLibrary *GPipelineLibrary = NULL;

// Guards every GPipelineLibrary method call (Load*/Store*/Serialize) - needed since
// PersistPipelineLibrary can run concurrently with a Store on the game's own render thread (the
// periodic flush thread below). SRWLOCK over a CRITICAL_SECTION: no separate init/destroy
// lifetime to manage, just SRWLOCK_INIT. Contention is a non-issue either way - PSO creation
// happens once per distinct shader, not per frame, and a flush is a rare, brief event.
static SRWLOCK GPipelineLibraryLock = SRWLOCK_INIT;

// ---- PSO cache persistence (NATIVE.md Section 12 milestone 6) ----
static void GetPsoCachePath(const char *selfDir, char *outPath, size_t outPathSize) {
	snprintf(outPath, outPathSize, "%s\\" DX12_DATA_SUBDIR "\\cache.bin", selfDir);
}

// Reads the whole file into a malloc'd buffer the CALLER owns - SetupPipelineLibrary already
// makes its own permanent malloc'd copy of whatever's passed to it, so this buffer only needs to
// survive until that call returns. Missing file (first run, or a fresh install) is the ordinary
// case, not an error.
static void LoadPersistedCacheBlob(const char *selfDir, void **outBlob, int *outBlobSize) {
	*outBlob = NULL;
	*outBlobSize = 0;

	char blobPath[MAX_PATH];
	GetPsoCachePath(selfDir, blobPath, sizeof(blobPath));

	HANDLE file = CreateFileA(blobPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		Dx12ShadowLog("dx12_proxy", "LoadPersistedCacheBlob: no existing cache at %s (err=%lu)", blobPath, GetLastError());
		return;
	}

	LARGE_INTEGER size;
	if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 0x7FFFFFFF) {
		CloseHandle(file);
		Dx12ShadowLog("dx12_proxy", "LoadPersistedCacheBlob: %s has an invalid size, ignoring", blobPath);
		return;
	}

	int blobSize = (int)size.QuadPart;
	void *blob = malloc((size_t)blobSize);
	if (!blob) {
		CloseHandle(file);
		Dx12ShadowLog("dx12_proxy", "LoadPersistedCacheBlob: malloc(%d) failed", blobSize);
		return;
	}

	DWORD bytesRead = 0;
	bool ok = ReadFile(file, blob, (DWORD)blobSize, &bytesRead, NULL) && bytesRead == (DWORD)blobSize;
	CloseHandle(file);
	if (!ok) {
		Dx12ShadowLog("dx12_proxy", "LoadPersistedCacheBlob: ReadFile(%s) failed, err=%lu", blobPath, GetLastError());
		free(blob);
		return;
	}

	Dx12ShadowLog("dx12_proxy", "LoadPersistedCacheBlob: loaded %d bytes from %s", blobSize, blobPath);
	*outBlob = blob;
	*outBlobSize = blobSize;
}

// Called from Dx12PsoCache_Shutdown (DLL_PROCESS_DETACH, a final best-effort catch-up flush if
// that ever actually runs) AND periodically from FlushThreadProc below, whichever comes first for
// any given piece of newly-cached data.
static void PersistPipelineLibrary(const char *selfDir) {
	if (!GPipelineLibrary) return;

	AcquireSRWLockExclusive(&GPipelineLibraryLock);
	SIZE_T size = GPipelineLibrary->GetSerializedSize();
	void *buffer = NULL;
	HRESULT r = S_OK;
	if (size > 0) {
		buffer = malloc(size);
		if (buffer) r = GPipelineLibrary->Serialize(buffer, size);
	}
	ReleaseSRWLockExclusive(&GPipelineLibraryLock);

	if (size == 0) {
		Dx12ShadowLog("dx12_proxy", "PersistPipelineLibrary: empty library, nothing to write");
		return;
	}
	if (!buffer) {
		Dx12ShadowLog("dx12_proxy", "PersistPipelineLibrary: malloc(%zu) failed", (size_t)size);
		return;
	}
	if (FAILED(r)) {
		Dx12ShadowLog("dx12_proxy", "PersistPipelineLibrary: Serialize failed, hr=%lx", (unsigned long)r);
		free(buffer);
		return;
	}

	char blobPath[MAX_PATH];
	GetPsoCachePath(selfDir, blobPath, sizeof(blobPath));

	HANDLE file = CreateFileA(blobPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		Dx12ShadowLog("dx12_proxy", "PersistPipelineLibrary: CreateFileA(%s) failed, err=%lu", blobPath, GetLastError());
		free(buffer);
		return;
	}

	DWORD written = 0;
	bool ok = WriteFile(file, buffer, (DWORD)size, &written, NULL) && written == (DWORD)size;
	CloseHandle(file);
	free(buffer);

	if (ok) {
		Dx12ShadowLog("dx12_proxy", "PersistPipelineLibrary: wrote %zu bytes to %s", (size_t)size, blobPath);
	} else {
		Dx12ShadowLog("dx12_proxy", "PersistPipelineLibrary: WriteFile(%s) failed, err=%lu", blobPath, GetLastError());
	}
}

// ---- Periodic flush (Section 12 milestone 6/8) ----
// DLL_PROCESS_DETACH alone isn't reliable enough to depend on for this: many real shutdown paths
// (TerminateProcess, a crash, the user killing the process, a hard alt-F4 some engines don't
// intercept) skip DLL_PROCESS_DETACH entirely, and even when it does run, Microsoft's own DllMain
// guidance is to keep DETACH minimal, not to treat it as a guaranteed last chance to do real work.
// So instead of relying on a clean exit, a background thread wakes up on its own schedule and
// flushes whatever's new since the last flush - an unclean shutdown then only ever loses the
// stores made in the last FLUSH_INTERVAL_MS, not the entire session's worth.
static const DWORD FLUSH_INTERVAL_MS = 15000;
static volatile bool GCacheDirty = false;

static DWORD WINAPI FlushThreadProc(LPVOID) {
	for (;;) {
		Sleep(FLUSH_INTERVAL_MS);
		if (GCacheDirty) {
			GCacheDirty = false;
			PersistPipelineLibrary(GProxySelfDir);
		}
	}
}

void Dx12PsoCache_StartFlushThread() {
	// Deliberately fire-and-forget: no exit event, no join. This thread just idles (Sleep) between
	// flushes, so letting the OS tear it down at process exit - clean or not - is safe and
	// standard practice; trying to synchronize with/join a thread from
	// DllMain(DLL_PROCESS_DETACH) risks a loader-lock deadlock for no real benefit here.
	HANDLE thread = CreateThread(NULL, 0, FlushThreadProc, NULL, 0, NULL);
	if (!thread) {
		Dx12ShadowLog("dx12_proxy", "StartFlushThread: CreateThread failed, err=%lu", GetLastError());
		return;
	}
	CloseHandle(thread); // we never wait on or otherwise reference it again - just let it run
}

void Dx12PsoCache_Shutdown(const char *selfDir) {
	PersistPipelineLibrary(selfDir);
}

void Dx12PsoCache_OnDeviceCaptured(dx_device device, const char *selfDir) {
	void *blob = NULL;
	int blobSize = 0;
	LoadPersistedCacheBlob(selfDir, &blob, &blobSize);

	GPipelineLibrary = SetupPipelineLibrary((ID3D12Device *)device, blob, blobSize);
	if (blob) free(blob); // SetupPipelineLibrary already made its own permanent copy

	Dx12ShadowLog("dx12_proxy", "get_device: PSO library %s", GPipelineLibrary ? "created" : "FAILED to create");
}

// ---- Cached create (NATIVE.md Section 7/Section 10 point 6, Section 12 milestone 5) ----
// Shared by both PSO-state natives (they differ only in desc type and which
// ID3D12PipelineLibrary method applies). On a hit, Load*Pipeline returns the cached PSO directly -
// no call-through to dx12_original.hdll at all. This export already holds the real
// ID3D12PipelineState* (either Load*Pipeline's own out-param on a hit, or the call-through's own
// return value on a miss), so returning it is a plain, direct return.

template <typename Desc>
static HRESULT LoadPipelineFromLibrary(LPCWSTR name, Desc *desc, ID3D12PipelineState **out);

template <>
HRESULT LoadPipelineFromLibrary<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(LPCWSTR name, D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc, ID3D12PipelineState **out) {
	return GPipelineLibrary->LoadGraphicsPipeline(name, desc, IID_PPV_ARGS(out));
}

template <>
HRESULT LoadPipelineFromLibrary<D3D12_COMPUTE_PIPELINE_STATE_DESC>(LPCWSTR name, D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, ID3D12PipelineState **out) {
	return GPipelineLibrary->LoadComputePipeline(name, desc, IID_PPV_ARGS(out));
}

// hashFn is a function pointer, not an already-computed hash, so it's only ever called once the
// GPipelineLibrary/desc guard below has passed.
template <typename Desc>
static dx_resource CachedPipelineCreate(const char *nativeName, Desc *desc, uint64_t (*hashFn)(const Desc *),
		dx_resource (*realFn)(void *), void *rawDesc) {
	if (!GPipelineLibrary || !desc) {
		return realFn ? realFn(rawDesc) : NULL;
	}

	wchar_t name[32];
	FormatCacheKeyName(hashFn(desc), name, sizeof(name) / sizeof(name[0]));

	AcquireSRWLockExclusive(&GPipelineLibraryLock);
	ID3D12PipelineState *cached = NULL;
	HRESULT loadResult = LoadPipelineFromLibrary<Desc>(name, desc, &cached);
	ReleaseSRWLockExclusive(&GPipelineLibraryLock);
	if (SUCCEEDED(loadResult)) {
		return (dx_resource)cached;
	}

	dx_resource created = realFn ? realFn(rawDesc) : NULL;
	if (created) {
		// StorePipeline treats a duplicate name as a benign E_INVALIDARG, not an error
		// (NATIVE.md Section 10 point 4) - only anything else is worth logging. Hits/misses
		// themselves aren't logged per-call - once live-verified, that's routine volume, not a
		// diagnostic signal; a genuine store failure still is.
		AcquireSRWLockExclusive(&GPipelineLibraryLock);
		HRESULT storeResult = GPipelineLibrary->StorePipeline(name, (ID3D12PipelineState *)created);
		ReleaseSRWLockExclusive(&GPipelineLibraryLock);
		if (FAILED(storeResult) && storeResult != E_INVALIDARG) {
			Dx12ShadowLog("dx12_proxy", "%s: StorePipeline(%ls) failed, hr=%lx", nativeName, name, (unsigned long)storeResult);
		} else {
			GCacheDirty = true;
		}
	}
	return created;
}

dx_resource Dx12PsoCache_CreateGraphicsPipeline(void *rawDesc, dx_resource (*realFn)(void *)) {
	return CachedPipelineCreate("create_graphics_pipeline_state", (D3D12_GRAPHICS_PIPELINE_STATE_DESC *)rawDesc,
		HashGraphicsPipelineKey, realFn, rawDesc);
}

dx_resource Dx12PsoCache_CreateComputePipeline(void *rawDesc, dx_resource (*realFn)(void *)) {
	return CachedPipelineCreate("create_compute_pipeline_state", (D3D12_COMPUTE_PIPELINE_STATE_DESC *)rawDesc,
		HashComputePipelineKey, realFn, rawDesc);
}
