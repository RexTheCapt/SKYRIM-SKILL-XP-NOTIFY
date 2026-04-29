# vcpkg triplet: x64 Windows, statically-linked port libs against the
# dynamic MSVC CRT (MD), built with our clang-cl + xwin SDK toolchain so
# port builds run under the same compiler as our plugin DLL.
#
# Use:   -DVCPKG_TARGET_TRIPLET=x64-windows-clang-cl-xwin
#        -DVCPKG_OVERLAY_TRIPLETS=cmake/triplets

set(VCPKG_TARGET_ARCHITECTURE x64)
# Static CRT linkage: the DLL ends up self-contained, no runtime dependency
# on MSVCP140.dll / VCRUNTIME140.dll / api-ms-win-crt-*.dll. Required for
# this Linux→Wine build because the Wine prefix has the VC++ libs but not
# the Universal CRT forwarder DLLs (`api-ms-win-crt-runtime-l1-1-0.dll`
# et al.), so a /MD-linked plugin gets rejected by LoadLibrary before
# SKSE can call SKSEPlugin_Load. /MT also matches what most shipped SKSE
# plugins do for end-user portability.
set(VCPKG_CRT_LINKAGE         static)
set(VCPKG_LIBRARY_LINKAGE     static)
set(VCPKG_CMAKE_SYSTEM_NAME   Windows)

# Chainload our cross-compile toolchain into every port's CMake invocation.
get_filename_component(_self_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${_self_dir}/../clang-cl-xwin.cmake")
