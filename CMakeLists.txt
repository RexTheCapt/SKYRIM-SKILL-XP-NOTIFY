cmake_minimum_required(VERSION 3.21)

# Match the static CRT linkage we set in the vcpkg triplet so our DLL is
# self-contained — no MSVCP140/VCRUNTIME140/UCRT-forwarder DLL imports.
# See the triplet for why this is needed for Linux→Wine specifically.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    CACHE STRING "" FORCE)

project(SkillXPNotify
    VERSION     0.1.0
    DESCRIPTION "Per-action skill XP corner notifications for Skyrim SE/AE."
    LANGUAGES   CXX)

# v1 targets SE/AE 1.6.1170 only. VR pulls in rapidcsv (which the upstream
# CMakeLists searches for unconditionally) and openvr (a sub-submodule we
# did not init), so disable it. Tests off — we don't run CommonLibSSE's
# Catch2 suite from this project.
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS      OFF CACHE BOOL "" FORCE)

# DirectXTK is declared as a public dep by commonlibsse-ng but no source
# in its RE/REL/REX/SKSE namespaces actually uses it (verified at commit
# 530bbc73). Its vcpkg port build fails on Linux because it shells out to
# CompileShaders.cmd. Point find_package at a stub Config so the lib's
# `find_package(directxtk CONFIG REQUIRED)` is satisfied with an empty
# INTERFACE target. See cmake/stub-directxtk/directxtkConfig.cmake.
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stub-directxtk")

# The lib's CMakeLists calls find_path(RAPIDCSV_INCLUDE_DIRS "rapidcsv.h")
# unconditionally and then puts the result into target_include_directories
# also unconditionally — meaning even with VR off, a NOTFOUND propagates.
# Pre-set it to an empty include dir so configure passes.
set(RAPIDCSV_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/empty"
    CACHE INTERNAL "stubbed: rapidcsv only needed for VR runtime support")

add_subdirectory(lib/commonlibsse-ng EXCLUDE_FROM_ALL)

# Consume the helper that generates SKSEPluginInfo() boilerplate.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/commonlibsse-ng/cmake")
include(CommonLibSSE)

# add_commonlibsse_plugin links against the alias `CommonLibSSE::CommonLibSSE`
# that the lib's installed Config.cmake sets up. When we add_subdirectory
# instead, the raw target is just `CommonLibSSE` — alias it so the helper
# works either way.
if(NOT TARGET CommonLibSSE::CommonLibSSE AND TARGET CommonLibSSE)
    add_library(CommonLibSSE::CommonLibSSE ALIAS CommonLibSSE)
endif()

add_commonlibsse_plugin(SkillXPNotify
    AUTHOR  "RexTheCapt"
    VERSION ${PROJECT_VERSION}
    USE_ADDRESS_LIBRARY
    SOURCES
        src/Plugin.cpp
        src/Hook.cpp)

target_include_directories(SkillXPNotify PRIVATE src)
target_precompile_headers(SkillXPNotify PRIVATE src/pch.h)
target_compile_features(SkillXPNotify PRIVATE cxx_std_23)
