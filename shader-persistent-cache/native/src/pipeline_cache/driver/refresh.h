#pragma once

#include <cstddef>

// dx12_original.hdll lives in this proxy's own data/ subfolder (refreshed by RefreshDx12Impl), so it
// needs its own explicit-full-path load - a bare LoadLibraryA("dx12_original.hdll") would NOT find it.
void GetDx12ImplPath(const char *selfDir, char *outPath, size_t outPathSize);

// NATIVE.md Section 4a: refresh dx12_original.hdll (this mod's own byte-copy of the real dx12.hdll)
// every startup, rather than shipping a stale one-time copy - self-healing across game updates.
void RefreshDx12Impl(const char *selfDir);
