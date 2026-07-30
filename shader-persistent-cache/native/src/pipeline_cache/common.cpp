#include "common.h"

#include <cstring>

char GProxySelfDir[MAX_PATH];

void Dx12CaptureOwnDir(HMODULE self, char *outDir, size_t outDirSize) {
	char path[MAX_PATH];
	GetModuleFileNameA(self, path, MAX_PATH);
	char *lastSlash = strrchr(path, '\\');
	size_t len = lastSlash ? (size_t)(lastSlash - path) : 0;
	if (len >= outDirSize) len = outDirSize - 1;
	memcpy(outDir, path, len);
	outDir[len] = '\0';
}
