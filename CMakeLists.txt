cmake_minimum_required(VERSION 3.21)
project(matrimelee VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The neogeorecomp toolkit: the in-tree copy (a submodule in a clean
# checkout), or a checkout next to this project. The in-tree copy is the
# v0.2.0 toolkit (commit af96b2b4) with the generic, title-independent
# runtime fixes used by the other ports in this release tree.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp/CMakeLists.txt)
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp)
else()
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/../neogeorecomp)
endif()
set(NEOGEORECOMP_DIR ${ng_default} CACHE PATH "neogeorecomp checkout")
add_subdirectory(${NEOGEORECOMP_DIR} neogeorecomp_build)

find_package(Python3 COMPONENTS Interpreter QUIET)

# Your unzipped MAME "matrim" set and "neogeo" system ROMs. The build
# decrypts them into ${CMAKE_BINARY_DIR}/roms; nothing derived from them is
# committed. Without it the player builds interpreter-only.
set(MATRIMLEE_ROM_DIR "" CACHE PATH "Directory with your matrim and Neo Geo BIOS ROM files")
# Optional: entry points a previous run found at runtime (--dump-misses).
set(MATRIMLEE_ENTRIES "${CMAKE_BINARY_DIR}/entries.txt" CACHE FILEPATH "Extra entry points for the recompiler")

# Stage 0: ROM preparation (K2K2 program blocks, NEO-CMC 7050 graphics,
# CMC50 M1, NEO-PCM2 mode-1 V ROMs, embedded 512 KB S ROM).
add_executable(matrimelee_decrypt tools/decrypt.c)

# Stage 1: the generator.
add_executable(matrimelee_recomp src/recomp_main.c src/game.c)
target_link_libraries(matrimelee_recomp PRIVATE m68krecomp_core)
target_include_directories(matrimelee_recomp PRIVATE src)

# Stage 2: the player, with the generated C when ROMs are available.
set(GAME_SOURCES src/main.c)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_FILES 16)
if(MATRIMLEE_ROM_DIR)
    set(DEC_DIR ${CMAKE_BINARY_DIR}/roms)
    set(DEC_STAMP ${DEC_DIR}/.decrypted)
    add_custom_command(
        OUTPUT ${DEC_STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEC_DIR}
        COMMAND matrimelee_decrypt --rom-path ${MATRIMLEE_ROM_DIR} --out ${DEC_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${DEC_STAMP}
        DEPENDS matrimelee_decrypt
        COMMENT "Preparing Matrimelee ROMs (K2K2 + NEO-CMC 7050 + CMC50 M1 + PCM2 V)")

    set(GEN_SOURCES ${GEN_DIR}/recomp_table.c)
    math(EXPR last "${GEN_FILES} - 1")
    foreach(i RANGE ${last})
        if(i LESS 10)
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_0${i}.c)
        else()
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_${i}.c)
        endif()
    endforeach()
    # The entry list starts empty; the `profile` target fills it from a
    # scripted play session and the next build regenerates with it.
    if(NOT EXISTS ${MATRIMLEE_ENTRIES})
        file(WRITE ${MATRIMLEE_ENTRIES} "")
    endif()
    file(MAKE_DIRECTORY ${GEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SOURCES} ${GEN_DIR}/recomp_common.h
        COMMAND matrimelee_recomp --rom-path ${DEC_DIR} --out ${GEN_DIR} --files ${GEN_FILES}
                --entries ${MATRIMLEE_ENTRIES}
        DEPENDS matrimelee_recomp ${MATRIMLEE_ENTRIES} ${DEC_STAMP}
        COMMENT "Recompiling Matrimelee from ${DEC_DIR}")
    list(APPEND GAME_SOURCES ${GEN_SOURCES})
endif()

add_executable(matrimelee ${GAME_SOURCES} src/game.c)
set_target_properties(matrimelee PROPERTIES OUTPUT_NAME "Matrimelee")
target_link_libraries(matrimelee PRIVATE neogeorecomp)
target_include_directories(matrimelee PRIVATE src ${GEN_DIR})
if(MATRIMLEE_ROM_DIR)
    add_custom_target(profile
        COMMAND matrimelee --headless --rom-path ${DEC_DIR} --frames 6000
                --dump-misses ${MATRIMLEE_ENTRIES}
        COMMAND matrimelee --headless --rom-path ${DEC_DIR} --frames 19000
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play_long.txt
                --dump-misses ${MATRIMLEE_ENTRIES}
        DEPENDS matrimelee
        COMMENT "Profiling scripted sessions for runtime-only entry points")
    if(Python3_Interpreter_FOUND)
        add_custom_target(audio-check
            COMMAND matrimelee --headless --rom-path ${DEC_DIR} --frames 19000
                    --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play_long.txt
                    --wav ${CMAKE_BINARY_DIR}/audio-check.wav
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_audio.py
                    ${CMAKE_BINARY_DIR}/audio-check.wav --ignore 0-70 --ignore 235-295
            DEPENDS matrimelee
            COMMENT "Checking Matrimelee audio continuity (ignores boot and the scripted GAME OVER/attract window)")
    endif()
    target_compile_definitions(matrimelee PRIVATE MATRIMLEE_HAVE_RECOMP)
    if(MSVC)
        # Generated routines are large; /bigobj lifts the section limit.
        # /O1 avoids MSVC's superlinear optimizer blow-up on the giant
        # dispatch functions and still runs the 68000 code at full speed.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "/bigobj;/O1")
    else()
        # GCC/Clang at -O3 spend a long time on these giant switch functions.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-O1")
    endif()
endif()
if(MSVC)
    target_compile_definitions(matrimelee PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(matrimelee_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(matrimelee_decrypt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ship the SDL2 DLL (when the window is built) next to the executable.
if(WIN32 AND TARGET SDL2::SDL2)
    add_custom_command(TARGET matrimelee POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:matrimelee> $<TARGET_FILE_DIR:matrimelee>
        COMMAND_EXPAND_LISTS)
endif()
