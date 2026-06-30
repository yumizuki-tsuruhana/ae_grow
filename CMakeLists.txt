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

set(AE_HEADERS "${AESDK_ROOT}/Examples/Headers")
set(AE_SP      "${AESDK_ROOT}/Examples/Headers/SP")
set(AE_UTIL    "${AESDK_ROOT}/Examples/Util")
set(AE_RES     "${AESDK_ROOT}/Examples/Resources")

# --- Sources ---
set(SOURCES
    src/Grow.cpp
    src/GrowKernel.cpp
    src/Grow_Strings.cpp
    ${AE_UTIL}/AEGP_SuiteHandler.cpp
    ${AE_UTIL}/MissingSuiteError.cpp
)

if(WIN32)
    list(APPEND SOURCES res/Grow.rc)
endif()

add_library(Grow MODULE ${SOURCES})

target_include_directories(Grow PRIVATE
    src
    ${AE_HEADERS}
    ${AE_HEADERS}/adobesdk
    ${AE_SP}
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
endif()

target_compile_options(Grow PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -O2>
)
