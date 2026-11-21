# piccobots: a libretro core, Multitap's demo game (four local players, one
# view each). Only libretro.h from deps/libretro-common.
cmake_minimum_required(VERSION 3.25)
project(piccobots LANGUAGES CXX)

option(PICCOBOTS_WARNINGS_AS_ERRORS "Warnings are errors" OFF)
option(PICCOBOTS_SANITIZE "Build with ASan + UBSan" OFF)
# A static archive instead of a shared library, for frontends that link the
# core in (piccolo's browser build: one wasm per core).
option(PICCOBOTS_STATIC "Build piccobots_libretro as a static archive" OFF)
# Where libretro.h is: our submodule, or another copy (multitap-org/cores
# builds from a checkout without submodules and passes its own).
set(PICCOBOTS_LIBRETRO_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/deps/libretro-common/include"
    CACHE PATH "The folder with libretro.h")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)   # only retro_* (RETRO_API) is exported
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(PICCOBOTS_STATIC)
    set(PICCOBOTS_LIBRARY_TYPE STATIC)
else()
    set(PICCOBOTS_LIBRARY_TYPE SHARED)
endif()
add_library(piccobots_libretro ${PICCOBOTS_LIBRARY_TYPE}
    src/art.cpp
    src/canvas.cpp
    src/draw.cpp
    src/font.cpp
    src/game.cpp
    src/libretro_core.cpp
    src/sound.cpp
    src/world.cpp
)
# libretro's name: piccobots_libretro.so / .dll / .dylib, no "lib" in front.
set_target_properties(piccobots_libretro PROPERTIES PREFIX "")
target_include_directories(piccobots_libretro SYSTEM PRIVATE "${PICCOBOTS_LIBRETRO_INCLUDE}")

if(MSVC)
    target_compile_options(piccobots_libretro PRIVATE /W4 /permissive- /utf-8)
    if(PICCOBOTS_WARNINGS_AS_ERRORS)
        target_compile_options(piccobots_libretro PRIVATE /WX)
    endif()
else()
    # Multitap's flags (STYLE.md).
    target_compile_options(piccobots_libretro PRIVATE
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion -Wshadow
        -Wold-style-cast -Wcast-qual -Wnull-dereference
        -Wno-missing-field-initializers
        -fno-exceptions -fno-rtti
    )
    if(PICCOBOTS_WARNINGS_AS_ERRORS)
        target_compile_options(piccobots_libretro PRIVATE -Werror)
    endif()
    if(PICCOBOTS_SANITIZE)
        target_compile_options(piccobots_libretro PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(piccobots_libretro PRIVATE -fsanitize=address,undefined)
    endif()
    if(WIN32 AND NOT PICCOBOTS_STATIC)
        # Only Windows' own DLLs next to the core.
        target_link_options(piccobots_libretro PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
endif()

# The game's own test (no libretro): ctest.
option(PICCOBOTS_BUILD_TESTS "Build the tests" ON)
if(PICCOBOTS_BUILD_TESTS)
    enable_testing()
    add_executable(piccobots_game_test
        tests/game_test.cpp
        src/game.cpp
        src/sound.cpp
        src/world.cpp
    )
    if(NOT MSVC)
        target_compile_options(piccobots_game_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
                               -Wshadow -Wold-style-cast -fno-exceptions -fno-rtti)
        if(PICCOBOTS_WARNINGS_AS_ERRORS)
            target_compile_options(piccobots_game_test PRIVATE -Werror)
        endif()
        if(PICCOBOTS_SANITIZE)
            target_compile_options(piccobots_game_test PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(piccobots_game_test PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
    add_test(NAME piccobots_game COMMAND piccobots_game_test)
endif()
