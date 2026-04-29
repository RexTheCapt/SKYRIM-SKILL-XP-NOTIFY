// Force-included before every TU via /FI in cmake/clang-cl-xwin.cmake.
//
// commonlibsse-ng's CMake helper writes an auto-generated
// __<plugin>Plugin.cpp that uses "..."sv literals at namespace scope to
// fill in SKSEPluginInfo(). The lib's only `using namespace std::literals`
// lives inside `namespace SKSE`, so at global scope the `sv` literal
// operator is unresolved under clang-cl. Pulling the literal namespace
// in globally here fixes the auto-generated TU without us patching the
// upstream submodule.

#pragma once

#include <string>
#include <string_view>

using namespace std::literals;
