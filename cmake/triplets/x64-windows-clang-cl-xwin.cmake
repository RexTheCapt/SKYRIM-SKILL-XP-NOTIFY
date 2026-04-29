# vcpkg triplet: x64 Windows, statically-linked port libs against the
# dynamic MSVC CRT (MD), built with our clang-cl + xwin SDK toolchain so
# port builds run under the same compiler as our plugin DLL.
#
# Use:   -DVCPKG_TARGET_TRIPLET=x64-windows-clang-cl-xwin
#        -DVCPKG_OVERLAY_TRIPLETS=cmake/triplets

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE         dynamic)
set(VCPKG_LIBRARY_LINKAGE     static)
set(VCPKG_CMAKE_SYSTEM_NAME   Windows)

# Chainload our cross-compile toolchain into every port's CMake invocation.
get_filename_component(_self_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${_self_dir}/../clang-cl-xwin.cmake")
