cmake_minimum_required(VERSION 3.16)
project(touhou15 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Upstream game logic (YomotsuHisami/th15, th15_web/cpp/game) + the shared
# GLES renderer, and the Switch host in src/ (upstream th15_web/cpp/sdl with
# the browser runtime replaced by src/main.cpp).
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/game/*.cpp)
file(GLOB HOST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM HOST_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
list(APPEND HOST_SOURCES portable/sdl/Renderer.cpp)

# CP932 -> UTF-16 map for the glyph baker (same derivation as upstream).
set(CP932_TABLE ${CMAKE_BINARY_DIR}/generated/cp932_table.inc)
add_custom_command(OUTPUT ${CP932_TABLE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/gen_cp932.py ${CP932_TABLE}
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/gen_cp932.py
    COMMENT "Generating CP932 table" VERBATIM)

# Game + host objects, shared by the game and the host tests.
add_library(th15_core OBJECT ${GAME_SOURCES} ${HOST_SOURCES} ${CP932_TABLE})
target_include_directories(th15_core PUBLIC src ${CMAKE_BINARY_DIR}/generated)

# Same code-generation contract as the upstream web build: no FMA
# contraction (bit-exact float order with the original), no strict aliasing,
# no exceptions/RTTI. GCC defaults to -ffp-contract=fast on AArch64, so this
# flag is load-bearing for replay/RNG determinism.
target_compile_options(th15_core PUBLIC
    -O2 -ffp-contract=off -fno-strict-aliasing
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-class-memaccess> -Wno-misleading-indentation -Wno-parentheses)
target_compile_definitions(th15_core PUBLIC TH_NATIVE_PLATFORM=1 TH15_DEVELOPMENT_HARNESS=0)

add_executable(touhou15 src/main.cpp)
target_link_libraries(touhou15 PRIVATE th15_core)

if(NINTENDO_SWITCH)
    # ---- Nintendo Switch (devkitA64; configure with the devkitPro Switch
    # toolchain file, which defines NINTENDO_SWITCH) ----
    # Layout/naming follows the Saekaze Touhou ports: touhou15.nro +
    # sdmc:/switch/touhou/touhou15/ (th15.dat, thbgm.dat, msgothic.ttc).
    if(NOT DEFINED ENV{DEVKITPRO})
        set(ENV{DEVKITPRO} "/opt/devkitpro")
    endif()
    set(TH_DEVKITPRO "$ENV{DEVKITPRO}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg")
        message(FATAL_ERROR "platform/switch/icon.jpg (256x256 JPEG) not found.")
    endif()
    target_compile_definitions(th15_core PUBLIC __SWITCH__)
    target_include_directories(th15_core PUBLIC
        ${TH_DEVKITPRO}/portlibs/switch/include
        ${TH_DEVKITPRO}/portlibs/switch/include/SDL2
        ${TH_DEVKITPRO}/portlibs/switch/include/freetype2
        ${TH_DEVKITPRO}/libnx/include)
    target_link_directories(touhou15 PRIVATE
        ${TH_DEVKITPRO}/portlibs/switch/lib
        ${TH_DEVKITPRO}/libnx/lib)
    # Static link order matters: dependents first, dependencies after.
    target_link_libraries(touhou15 PRIVATE
        SDL2 EGL glapi drm_nouveau GLESv2 freetype harfbuzz png bz2 z nx m)
    set_target_properties(touhou15 PROPERTIES OUTPUT_NAME "touhou15")

    # Default (all languages): romanised title. The original Japanese title is
    # patched into the Japanese slot only - same as saekaze/th07 and th10.
    nx_generate_nacp(OUTPUT "${CMAKE_BINARY_DIR}/touhou15-base.nacp"
        NAME "Touhou 15: Legacy of Lunatic Kingdom"
        AUTHOR "ZUN / Team Shanghai Alice"
        VERSION "1.00b-r2")
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/touhou15.nacp"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
                "${CMAKE_BINARY_DIR}/touhou15-base.nacp" "${CMAKE_BINARY_DIR}/touhou15.nacp"
                --lang Japanese
                --name "東方紺珠伝　～ Legacy of Lunatic Kingdom"
                --author "上海アリス幻樂団"
                --fill-empty-name "Touhou 15: Legacy of Lunatic Kingdom"
                --fill-empty-author "ZUN / Team Shanghai Alice"
        DEPENDS "${CMAKE_BINARY_DIR}/touhou15-base.nacp"
                "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
        COMMENT "Patching Japanese title into NACP"
        VERBATIM)
    nx_create_nro(touhou15
        ICON "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg"
        NACP "${CMAKE_BINARY_DIR}/touhou15.nacp")
else()
    # ---- Linux test build (same sources, SDL2 + GLES3 desktop) ----
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(FT REQUIRED freetype2)
    target_include_directories(th15_core PUBLIC ${SDL2_INCLUDE_DIRS} ${FT_INCLUDE_DIRS})
    target_link_libraries(touhou15 PRIVATE ${SDL2_LIBRARIES} ${FT_LIBRARIES} GLESv2 m)

    # Host tests: ./build/th15_host_tests (run under xvfb-run for the renderer)
    add_executable(th15_host_tests tests/test_host.cpp)
    target_link_libraries(th15_host_tests PRIVATE th15_core ${SDL2_LIBRARIES} ${FT_LIBRARIES} GLESv2 m)

endif()
