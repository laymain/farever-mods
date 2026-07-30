#pragma once

// Dependency-free logging: DllMain runs before hlx-loader's trace() machinery exists, so this
// can't go through hlx/logs/hlx.log. Writes to "<selfDir>\data\debug.log" (NATIVE.md
// Section 6 step 3). Dx12ShadowLogOpen must be called once (from DllMain, before anything else
// logs) and Dx12ShadowLogClose once at shutdown - the handle stays open for the process
// lifetime instead of being reopened on every call.

void Dx12ShadowLogOpen(const char *selfDir);
void Dx12ShadowLogClose();
void Dx12ShadowLog(const char *tag, const char *fmt, ...);
