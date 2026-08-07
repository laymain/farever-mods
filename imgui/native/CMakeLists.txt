cmake_minimum_required(VERSION 3.13)
project(imgui CXX)

if(NOT WIN32)
    message(FATAL_ERROR "imgui.hdll is Windows-only - it wraps ID3D12*/HWND, DirectX 12 and Win32 APIs with no other-platform equivalent.")
endif()

set(HASHLINK_DIR "" CACHE PATH "Path to a HashLink checkout or SDK install (needs src/hl.h or include/hl.h, plus a libhl import library)")
if(NOT HASHLINK_DIR)
    message(FATAL_ERROR "Set -DHASHLINK_DIR=<path to a HashLink checkout/install>")
endif()

add_library(imgui.hdll SHARED
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    # cimgui.cpp unconditionally wraps ImGui::ShowDemoWindow/ShowAboutWindow/ShowStyleEditor/
    # ShowStyleSelector/ShowUserGuide (igShowDemoWindow etc.) regardless of whether our own codegen
    # binds them - omitting this file is a link error (confirmed by a real MSVC build), not just a
    # missing feature.
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_dx12.cpp
    third_party/cimgui/cimgui.cpp
    src/imgui_native.cpp
    src/generated/imgui_prims.cpp
)

set_target_properties(imgui.hdll PROPERTIES
    PREFIX ""
    OUTPUT_NAME "imgui64"
    SUFFIX ".hdll"
)

target_include_directories(imgui.hdll PRIVATE
    ${HASHLINK_DIR}/src
    ${HASHLINK_DIR}/include
    # third_party/imgui - so backends/imgui_impl_{win32,dx12}.cpp's own bare #include "imgui.h"
    # resolves (they sit one directory below imgui.h itself, so the same-directory quote-include
    # rule that lets imgui.cpp/imgui_draw.cpp etc find it doesn't apply to them).
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    # third_party (not third_party/cimgui) - cimgui.cpp's own includes are
    # `#include "./imgui/imgui.h"` and `"./imgui/imgui_internal.h"`, written expecting imgui to be
    # a submodule AT cimgui/imgui/ (upstream's own layout). Vendoring imgui/ and cimgui/ as plain
    # siblings (see third_party/VERSIONS.md) instead of nesting one inside the other means those
    # relative includes only resolve by falling through to this parent dir on the include path.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

find_library(HASHLINK_LIBHL_LIB
    NAMES libhl64 libhl
    PATHS ${HASHLINK_DIR}
)
if(NOT HASHLINK_LIBHL_LIB)
    message(FATAL_ERROR "Could not find a libhl import library (libhl64.lib/libhl.lib) under HASHLINK_DIR (${HASHLINK_DIR})")
endif()

target_link_libraries(imgui.hdll PRIVATE
    ${HASHLINK_LIBHL_LIB}
    d3d12
    dxgi
    # imgui_impl_dx12.cpp's own font-atlas shader compilation uses D3DCompile() (also declared via
    # its own `#pragma comment(lib, "d3dcompiler")`; listed explicitly here too rather than relying
    # on that alone).
    d3dcompiler
)

# RUNTIME only, same reasoning as shader-cache/native/CMakeLists.txt: the .lib import library
# alongside the .hdll is a link-time-only artifact nothing downstream needs (hlx-boot loads the
# .hdll itself), so leaving it unlisted keeps it out of the staged plugins/ dir and dist/*.zip.
install(TARGETS imgui.hdll RUNTIME DESTINATION .)
