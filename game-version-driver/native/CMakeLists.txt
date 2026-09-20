cmake_minimum_required(VERSION 3.12)
project(game-version-driver C)

if(NOT WIN32)
    message(FATAL_ERROR "game_version.dll is Windows-only, matching hlx-boot itself.")
endif()
if(NOT MSVC)
    message(FATAL_ERROR "game_version.dll requires MSVC - it uses __try/__except for parse-failure safety.")
endif()

set(CMAKE_C_STANDARD 11)

add_library(game-version-driver SHARED src/game_version_driver.c)

set_target_properties(game-version-driver PROPERTIES
    OUTPUT_NAME "game_version"
    PREFIX ""
)

install(TARGETS game-version-driver RUNTIME DESTINATION .)
