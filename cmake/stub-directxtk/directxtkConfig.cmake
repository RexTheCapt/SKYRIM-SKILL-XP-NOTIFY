# Stub directxtk CMake config so commonlibsse-ng's find_package(directxtk
# CONFIG REQUIRED) succeeds without us actually building DirectXTK.
#
# Why: DirectXTK's vcpkg port runs CompileShaders.cmd (a Windows .cmd
# script invoking fxc.exe) which won't run on a Linux build host. The
# lib has the dep declared in CMakeLists.txt but no source file in the
# RE/REL/REX/SKSE namespaces actually #includes a DirectXTK header or
# calls a Microsoft::DirectXTK function — confirmed by grep at the
# 530bbc73 commit. Linking against an empty INTERFACE target keeps the
# build graph happy and produces a DLL with no DirectXTK symbol refs.
#
# If a future bump of CommonLibVR starts genuinely depending on
# DirectXTK, the link step will fail with unresolved-symbol errors and
# this stub will need replacing — likely by building DirectXTK on a
# Windows host and dropping the prebuilt artefacts here.

if(NOT TARGET Microsoft::DirectXTK)
    add_library(Microsoft::DirectXTK INTERFACE IMPORTED)

    # The lib genuinely does include <SimpleMath.h> from RE/S/State.h, but
    # SimpleMath is fully inline (its types live in SimpleMath.h +
    # SimpleMath.inl, no .lib symbols). Vendor those two headers so the
    # consuming TU compiles. See cmake/stub-directxtk/include/.
    set_target_properties(Microsoft::DirectXTK PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/include")
endif()

set(directxtk_FOUND TRUE)
