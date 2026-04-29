# CMake toolchain file: cross-compile from Linux to x86_64 Windows using
# clang-cl-20 + lld-link-20 driving an xwin-fetched MSVC SDK.
#
# Use:   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-xwin.cmake
# Pair with: vcpkg.json + cmake/triplets/x64-windows-clang-cl-xwin.cmake
#            (set VCPKG_TARGET_TRIPLET=x64-windows-clang-cl-xwin so vcpkg
#            chainloads this same file when building deps).

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT DEFINED XWIN_SDK)
    set(XWIN_SDK "/home/user/storage/apps/skill-xp-notify-toolchain/xwin-sdk"
        CACHE PATH "xwin-fetched Windows SDK root (crt/ + sdk/)")
endif()

if(NOT EXISTS "${XWIN_SDK}/crt" OR NOT EXISTS "${XWIN_SDK}/sdk")
    message(FATAL_ERROR
        "XWIN_SDK does not contain crt/ and sdk/: '${XWIN_SDK}'.\n"
        "Re-run xwin splat or pass -DXWIN_SDK=/path/to/xwin-sdk.")
endif()

# Compilers + tools.
set(CMAKE_C_COMPILER   clang-cl-20)
set(CMAKE_CXX_COMPILER clang-cl-20)
set(CMAKE_RC_COMPILER  llvm-rc-20)
set(CMAKE_AR           llvm-lib-20)
set(CMAKE_LINKER       lld-link-20)

# Tell CMake clang-cl is an MSVC-style frontend so it picks msvc-flavoured
# flag handling (this is normally auto-detected on Windows hosts).
set(CMAKE_C_COMPILER_ID                   Clang)
set(CMAKE_CXX_COMPILER_ID                 Clang)
set(CMAKE_C_SIMULATE_ID                   MSVC)
set(CMAKE_CXX_SIMULATE_ID                 MSVC)
set(CMAKE_C_COMPILER_FRONTEND_VARIANT     MSVC)
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT   MSVC)
set(CMAKE_C_COMPILER_TARGET               x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET             x86_64-pc-windows-msvc)

# /winsysroot tells clang-cl where to find the MSVC + Windows SDK trees.
# We also pass /imsvc paths explicitly because clang-cl's /winsysroot lookup
# is case-sensitive on Linux and the splatted xwin tree uses lowercase
# directory aliases under sdk/include and sdk/lib.
set(_xwin_compile_flags
    "/winsysroot${XWIN_SDK}"
    "/imsvc${XWIN_SDK}/crt/include"
    "/imsvc${XWIN_SDK}/sdk/include/ucrt"
    "/imsvc${XWIN_SDK}/sdk/include/um"
    "/imsvc${XWIN_SDK}/sdk/include/shared"
    # MSVC's <execution> uses _Guarded_by_ et al. without first including
    # the header that defines them — relying on an MSVC-host bootstrap we
    # don't have. Force-include xwin's sal.h (which transitively pulls in
    # concurrencysal.h) on every TU so SAL annotation macros exist.
    #
    # Use the absolute path on purpose: directxmath's vcpkg port also
    # ships a competing sal.h shim (lifted from dotnet/runtime) that does
    # NOT include concurrencysal.h, and it sits in the vcpkg_installed
    # include dir which CMake places ahead of /winsysroot. Bare `/FIsal.h`
    # picks up the wrong file and `_Guarded_by_` ends up undefined.
    "/FI${XWIN_SDK}/crt/include/sal.h"
    # See cmake/preinclude.h: pulls `using namespace std::literals` into
    # the global scope so the lib's auto-generated SKSEPluginInfo TU
    # (which uses "..."sv at namespace scope) compiles under clang-cl.
    "/FI${CMAKE_CURRENT_LIST_DIR}/preinclude.h")

string(JOIN " " _xwin_compile_flags_str ${_xwin_compile_flags})

set(CMAKE_C_FLAGS_INIT   "${_xwin_compile_flags_str}")
set(CMAKE_CXX_FLAGS_INIT "${_xwin_compile_flags_str}")

# Linker: ask clang-cl to drive lld-link-20 directly, plus library search
# paths into the xwin tree. Use absolute /libpath: arguments for safety.
set(_xwin_link_flags
    "-fuse-ld=lld-link-20"
    "/libpath:${XWIN_SDK}/crt/lib/x86_64"
    "/libpath:${XWIN_SDK}/sdk/lib/um/x86_64"
    "/libpath:${XWIN_SDK}/sdk/lib/ucrt/x86_64")

string(JOIN " " _xwin_link_flags_str ${_xwin_link_flags})

set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_xwin_link_flags_str}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_xwin_link_flags_str}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_xwin_link_flags_str}")

# Force the static MSVC runtime (/MT) for everything the chainload sees,
# including vcpkg port builds. The Wine prefix doesn't ship the Universal
# CRT forwarder DLLs (`api-ms-win-crt-runtime-l1-1-0.dll` et al.), so a
# /MD-linked plugin gets bounced by LoadLibrary before SKSE can even
# call SKSEPlugin_Load. Setting `VCPKG_CRT_LINKAGE = static` on the
# triplet alone wasn't enough — vcpkg's CRT propagation lands after the
# chainload toolchain runs, so without this line port builds revert to
# the CMake default (`MultiThreadedDLL`) and a /MT plugin can't link
# against /MD ports.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    CACHE STRING "" FORCE)

# When CMake probes the compiler it tries to build an executable, which
# requires the runtime libs to be set up correctly. Building a static lib
# during the probe is sufficient and avoids ordering issues.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Forward XWIN_SDK into try_compile so vcpkg port builds (which run cmake
# under our toolchain) don't lose the path.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XWIN_SDK)
