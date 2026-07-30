#pragma once

// Entry point for the driver/ subpackage: the plumbing that lets pipeline_cache see the real
// D3D12 calls it needs to cache, by shadow-loading and interposing on the game's own dx12.hdll
// (NATIVE.md Section 3-7). This bridge exists only to serve the cache above it, not as a
// standalone concern.

// Refreshes dx12_original.hdll's on-disk copy (driver/refresh.h), loads it, and resolves all 98 real
// native function pointers (driver/symbols.h) - the 94 mechanical passthroughs (driver/forwarding.cpp)
// and the 4 cache-integrating natives (driver/intercepts.cpp) both read through those pointers.
void InitDriverBridge(const char *selfDir);
