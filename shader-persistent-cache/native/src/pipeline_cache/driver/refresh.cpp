#include "refresh.h"

#include "../common.h"
#include "../log.h"

#include <windows.h>
#include <cstdio>
#include <cstring>

// The game's own real dx12.hdll sits directly alongside its .exe (confirmed against the real
// install), not anywhere near this proxy's own plugins/shader-persistent-cache/ folder.
static bool GetGameDir(char *outDir, size_t outDirSize) {
	char exePath[MAX_PATH];
	if (!GetModuleFileNameA(NULL, exePath, MAX_PATH)) return false;

	char *lastSlash = strrchr(exePath, '\\');
	if (!lastSlash) return false;

	size_t len = (size_t)(lastSlash - exePath);
	if (len >= outDirSize) return false;

	memcpy(outDir, exePath, len);
	outDir[len] = '\0';
	return true;
}

void GetDx12ImplPath(const char *selfDir, char *outPath, size_t outPathSize) {
	snprintf(outPath, outPathSize, "%s\\" DX12_DATA_SUBDIR "\\dx12_original.hdll", selfDir);
}

void RefreshDx12Impl(const char *selfDir) {
	char gameDir[MAX_PATH];
	if (!GetGameDir(gameDir, sizeof(gameDir))) {
		Dx12ShadowLog("dx12_proxy", "RefreshDx12Impl: GetGameDir failed, err=%lu", GetLastError());
		return;
	}

	char realPath[MAX_PATH];
	snprintf(realPath, sizeof(realPath), "%s\\dx12.hdll", gameDir);

	char implDir[MAX_PATH];
	snprintf(implDir, sizeof(implDir), "%s\\" DX12_DATA_SUBDIR, selfDir);
	if (!CreateDirectoryA(implDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		Dx12ShadowLog("dx12_proxy", "RefreshDx12Impl: CreateDirectoryA(%s) failed, err=%lu", implDir, GetLastError());
		return;
	}

	char implPath[MAX_PATH];
	GetDx12ImplPath(selfDir, implPath, sizeof(implPath));

	WIN32_FILE_ATTRIBUTE_DATA realInfo;
	if (!GetFileAttributesExA(realPath, GetFileExInfoStandard, &realInfo)) {
		Dx12ShadowLog("dx12_proxy", "RefreshDx12Impl: real dx12.hdll not found at %s, err=%lu", realPath, GetLastError());
		return;
	}

	WIN32_FILE_ATTRIBUTE_DATA implInfo;
	bool implExists = GetFileAttributesExA(implPath, GetFileExInfoStandard, &implInfo) != 0;
	bool sourceIsNewer = implExists && CompareFileTime(&realInfo.ftLastWriteTime, &implInfo.ftLastWriteTime) > 0;

	if (implExists && !sourceIsNewer) {
		Dx12ShadowLog("dx12_proxy", "RefreshDx12Impl: %s already up to date", implPath);
		return;
	}

	if (CopyFileA(realPath, implPath, FALSE)) {
		Dx12ShadowLog("dx12_proxy", "RefreshDx12Impl: copied %s -> %s (%s)", realPath, implPath,
			implExists ? "source was newer" : "no existing copy");
	} else {
		Dx12ShadowLog("dx12_proxy", "RefreshDx12Impl: CopyFileA(%s -> %s) failed, err=%lu", realPath, implPath, GetLastError());
	}
}
