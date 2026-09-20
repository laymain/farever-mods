#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

typedef bool (*ResolveStaticIntFieldLiteralFn)(const char *, const char *, int *);

static const char *VERSION_FIELDS[4] = { "VERSION_MAJOR", "VERSION_SUB", "VERSION_NETWORK", "VERSION_BUILD" };

/* hlx-boot ("libhl64.dll") is always already loaded and initialized by the time this can even
 * be called - it's the one that LoadLibrary's this driver in the first place (driver.c) - so
 * GetModuleHandleA here can never fail in practice, only GetProcAddress if a mismatched hlx-boot
 * build ever shipped without this export. Reuses hlx-boot's own hlboot.dat disk parse
 * (reflection.c's reflection_init_constructor_table) rather than re-parsing the file here:
 * that parse already runs once per process regardless, and is verified against real bytecode. */
__declspec(dllexport) int hlx_driver_game_version_v1(int32_t *outMajor, int32_t *outMinor, int32_t *outPatch, int32_t *outBuild)
{
    HMODULE hlxBoot = GetModuleHandleA("libhl64.dll");
    if (!hlxBoot) return 0;

    ResolveStaticIntFieldLiteralFn resolve = (ResolveStaticIntFieldLiteralFn)GetProcAddress(hlxBoot, "resolve_static_int_field_literal");
    if (!resolve) return 0;

    int values[4];
    for (int i = 0; i < 4; i++) {
        if (!resolve("$Const", VERSION_FIELDS[i], &values[i])) return 0;
    }

    *outMajor = values[0];
    *outMinor = values[1];
    *outPatch = values[2];
    *outBuild = values[3];
    return 1;
}
