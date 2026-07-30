#pragma once

#include <windows.h>

// Shared across every file in this module - dx12_original.hdll, debug.log and the PSO cache blob all
// live in "<GProxySelfDir>\data\".
#define DX12_DATA_SUBDIR "data"

// Populated once, at DllMain time (Dx12CaptureOwnDir) - every other file reads this
// instead of re-deriving its own directory.
extern char GProxySelfDir[MAX_PATH];

void Dx12CaptureOwnDir(HMODULE self, char *outDir, size_t outDirSize);
