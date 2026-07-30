#pragma once

// Must be the first include in every .cpp in this module: hl.h's HL_PRIM/DEFINE_PRIM
// definitions key off HL_NAME already being #defined the moment hl.h is first
// preprocessed in a TU (hl.h has its own #ifndef guard, so only the first inclusion
// counts). A stray transitive <hl.h> include ahead of this one would silently
// degrade every native in that TU to a no-op stub - compiles fine, hl_fatal4's at
// game launch, not a compile error.
#define HL_NAME(n) dx12_##n
#include <hl.h>
#undef _GUID // hl.h's _GUID ("g") collides with the real Windows SDK _GUID struct

typedef void *dx_resource;
typedef void *dx_device;
typedef void *dx_adapter;
typedef void *dx_compiler;
typedef void *dx_event;
typedef void *dx_window;
typedef void *dx_driver;

#define _RESOURCE _ABSTRACT(dx_resource)
#define _DEVICE _ABSTRACT(dx_device)
#define _ADAPTER _ABSTRACT(dx_adapter)
#define _COMPILER _ABSTRACT(dx_compiler)
#define _EVENT _ABSTRACT(dx_event)
#define _WINDOW _ABSTRACT(dx_window)
#define _DRIVER _ABSTRACT(dx_driver)
#define _CARRAY _ABSTRACT(hl_carray)
