cmake_minimum_required(VERSION 3.13)
project(shader_persistent_cache CXX)

if(NOT WIN32)
    message(FATAL_ERROR "shader-persistent-cache is Windows-only - it shadow-loads and wraps dx12.hdll, a Direct3D 12 API with no other-platform equivalent.")
endif()

set(HASHLINK_DIR "" CACHE PATH "Path to a HashLink checkout or SDK install (needs src/hl.h or include/hl.h, plus a libhl import library)")
if(NOT HASHLINK_DIR)
    message(FATAL_ERROR "Set -DHASHLINK_DIR=<path to a HashLink checkout/install>")
endif()

find_library(HASHLINK_LIBHL_LIB
    NAMES libhl64 libhl
    PATHS ${HASHLINK_DIR}
)
if(NOT HASHLINK_LIBHL_LIB)
    message(FATAL_ERROR "Could not find a libhl import library (libhl64.lib/libhl.lib) under HASHLINK_DIR (${HASHLINK_DIR})")
endif()

# NATIVE.md Section 4/5: the shadow-loaded proxy itself. Shadow-loading (forcing this to load,
# by its explicit full path, before HL's own resolve_library("dx12", ...) ever fires) is handled
# by hlx-boot's own eager plugins/*.hdll scan (hlx-core/hlx-boot/src/boot.c's
# EagerLoadPluginHdlls) - no separate trigger plugin needed here.
# Milestones 1-2 (Sections 6-7) live-verified against the real game; Section 8's PSO pipeline
# library (Section 12 milestone 4) implemented but not yet exercised - see the mod's README.md
# for current status.

add_library(dx12.hdll SHARED
    src/pipeline_cache.cpp
    src/pipeline_cache/common.cpp
    src/pipeline_cache/log.cpp
    src/pipeline_cache/hash.cpp
    src/pipeline_cache/store.cpp
    src/pipeline_cache/driver.cpp
    src/pipeline_cache/driver/refresh.cpp
    src/pipeline_cache/driver/symbols.cpp
    src/pipeline_cache/driver/forwarding.cpp
    src/pipeline_cache/driver/intercepts.cpp
)
set_target_properties(dx12.hdll PROPERTIES
    PREFIX ""
    OUTPUT_NAME "dx12"
    SUFFIX ".hdll"
)
target_include_directories(dx12.hdll PRIVATE
    ${HASHLINK_DIR}/src
    ${HASHLINK_DIR}/include
)
target_link_libraries(dx12.hdll PRIVATE ${HASHLINK_LIBHL_LIB} d3d12)

# RUNTIME only - the .lib import library alongside it is a link-time artifact no one downstream
# needs (hlx-boot loads the .hdll itself, nothing links against this DLL), so leaving it unlisted
# here keeps it out of the staged plugins/ dir and out of dist/*.zip.
install(TARGETS dx12.hdll RUNTIME DESTINATION .)
