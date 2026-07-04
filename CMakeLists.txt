cmake_minimum_required(VERSION 3.20)
project(Grow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- After Effects SDK path ---
if(NOT DEFINED AESDK_ROOT)
    if(DEFINED ENV{AESDK_ROOT})
        set(AESDK_ROOT $ENV{AESDK_ROOT})
    else()
        message(FATAL_ERROR
            "AESDK_ROOT is not set. "
            "Download the After Effects SDK from Adobe and set "
            "-DAESDK_ROOT=<path-to-sdk> or the AESDK_ROOT env var.")
    endif()
endif()

# Locate AE_Effect.h to auto-detect SDK layout
file(GLOB_RECURSE _ae_effect_h "${AESDK_ROOT}/**/AE_Effect.h")
if(_ae_effect_h)
    list(GET _ae_effect_h 0 _ae_effect_h_first)
    get_filename_component(AE_HEADERS "${_ae_effect_h_first}" DIRECTORY)
    message(STATUS "AE SDK headers found at: ${AE_HEADERS}")
else()
    set(AE_HEADERS "${AESDK_ROOT}/Examples/Headers")
    message(WARNING "AE_Effect.h not found by search, assuming: ${AE_HEADERS}")
endif()

get_filename_component(_examples_dir "${AE_HEADERS}" DIRECTORY)
set(AE_UTIL "${_examples_dir}/Util")
set(AE_RES  "${_examples_dir}/Resources")

# --- Sources ---
set(SOURCES
    src/Grow.cpp
    src/GrowKernel.cpp
)

if(WIN32)
    list(APPEND SOURCES res/Grow.rc)
endif()

add_library(Grow MODULE ${SOURCES})

target_include_directories(Grow PRIVATE
    src
    ${AE_HEADERS}
    ${AE_HEADERS}/adobesdk
    ${AE_HEADERS}/SP
    ${AE_UTIL}
    ${AE_RES}
)

# --- Platform config ---
if(WIN32)
    target_compile_definitions(Grow PRIVATE
        _CRT_SECURE_NO_WARNINGS
        MSWindows
        WIN32
    )
    set_target_properties(Grow PROPERTIES
        SUFFIX ".aex"
        LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/res/Grow.def\""
    )
elseif(APPLE)
    set_target_properties(Grow PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/res/Info.plist"
    )
    target_compile_options(Grow PRIVATE -Wno-deprecated-declarations)

    # --- Compile PiPL resource with Rez ---
    # AE reads the PiPL from Contents/Resources/<Executable>.rsrc (data fork),
    # exactly what Xcode produces for the SDK samples. The .rsrc file name
    # must match the executable name (Grow.rsrc, not GrowPiPL.rsrc).
    set(PIPL_R "${CMAKE_CURRENT_SOURCE_DIR}/res/GrowPiPL.r")
    set(PIPL_RSRC "${CMAKE_CURRENT_BINARY_DIR}/Grow.rsrc")

    add_custom_command(
        OUTPUT "${PIPL_RSRC}"
        COMMAND xcrun Rez
            -useDF
            -d __MACH__
            -I "${AE_HEADERS}"
            -I "${AE_RES}"
            "${PIPL_R}"
            -o "${PIPL_RSRC}"
        DEPENDS "${PIPL_R}"
        COMMENT "Compiling PiPL resource"
    )
    add_custom_target(GrowPiPL DEPENDS "${PIPL_RSRC}")
    add_dependencies(Grow GrowPiPL)

    add_custom_command(TARGET Grow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Grow>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PIPL_RSRC}"
            "$<TARGET_BUNDLE_DIR:Grow>/Contents/Resources/Grow.rsrc"
        COMMAND /bin/sh -c "printf 'eFKTFXTC' > '$<TARGET_BUNDLE_DIR:Grow>/Contents/PkgInfo'"
        COMMAND codesign --force --deep --sign -
            "$<TARGET_BUNDLE_DIR:Grow>"
        COMMENT "Installing PiPL resource, PkgInfo, and signing plugin"
    )
endif()

target_compile_options(Grow PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -O2>
)
