#include "log.h"

#include "common.h"

#include <windows.h>
#include <cstdio>
#include <cstdarg>

static HANDLE GLogFile = INVALID_HANDLE_VALUE;
static SRWLOCK GLogLock = SRWLOCK_INIT;

void Dx12ShadowLogOpen(const char *selfDir) {
	char dataDir[MAX_PATH];
	snprintf(dataDir, sizeof(dataDir), "%s\\" DX12_DATA_SUBDIR, selfDir);
	CreateDirectoryA(dataDir, NULL); // best-effort - a failure here just means the CreateFileA below fails too

	char logPath[MAX_PATH];
	snprintf(logPath, sizeof(logPath), "%s\\debug.log", dataDir);

	// CREATE_ALWAYS: truncate on this process's first (and only) open, matching the previous
	// per-process-truncate/then-append behavior with an explicit lifetime instead of a static flag.
	GLogFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void Dx12ShadowLogClose() {
	if (GLogFile == INVALID_HANDLE_VALUE) return;
	CloseHandle(GLogFile);
	GLogFile = INVALID_HANDLE_VALUE;
}

void Dx12ShadowLog(const char *tag, const char *fmt, ...) {
	if (GLogFile == INVALID_HANDLE_VALUE) return;

	char line[2048];
	int offset = 0;

	SYSTEMTIME t;
	GetLocalTime(&t);
	offset += snprintf(line + offset, sizeof(line) - offset, "[%02d:%02d:%02d.%03d] [%s] ",
		t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, tag);

	va_list args;
	va_start(args, fmt);
	offset += vsnprintf(line + offset, sizeof(line) - offset, fmt, args);
	va_end(args);

	if (offset < 0) return;
	if (offset >= (int)sizeof(line) - 1) offset = (int)sizeof(line) - 2; // truncate, keep room for '\n'
	line[offset++] = '\n';

	// The flush thread and the game's own render thread can both log concurrently
	// (PersistPipelineLibrary logs from either path) - serialize writes so lines never interleave.
	AcquireSRWLockExclusive(&GLogLock);
	DWORD written = 0;
	WriteFile(GLogFile, line, (DWORD)offset, &written, NULL);
	// Flushed on every write, not just periodically: this mod's whole PSO-cache design exists to
	// tolerate unclean shutdowns, so an unflushed buffered log would lose exactly the diagnostic
	// lines that matter most when that happens.
	FlushFileBuffers(GLogFile);
	ReleaseSRWLockExclusive(&GLogLock);
}
