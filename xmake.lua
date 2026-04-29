set_xmakever("3.0.0")

set_project("skill-xp-notify")
set_version("0.1.0")
set_arch("x64")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg")

-- v1 targets Skyrim SE/AE 1.6.1170 only. VR support pulls in openvr (a
-- recursive submodule we did not init) plus rapidcsv, so keep it off.
set_config("skyrim_vr", false)

-- Linux → Windows cross-compile toolchain: clang-cl-20 + lld-link-20 driven
-- against an xwin-fetched MSVC SDK. xmake's stock "clang-cl" toolchain assumes
-- a real MSVC environment (vcvars + Windows SDK env vars), which we don't have
-- on Linux. This custom toolchain wires clang-cl to the xwin sysroot directly.
toolchain("clang-cl-xwin")
    set_kind("standalone")
    set_homepage("https://github.com/Jake-Shadle/xwin")
    set_description("clang-cl + lld-link cross-compile against an xwin Windows SDK")

    set_toolset("cc",    "clang-cl-20")
    set_toolset("cxx",   "clang-cl-20")
    set_toolset("ld",    "lld-link-20")
    set_toolset("sh",    "lld-link-20")
    set_toolset("ar",    "llvm-lib-20")
    set_toolset("mrc",   "llvm-rc-20")
    set_toolset("strip", "llvm-strip-20")

    on_check(function (toolchain)
        return true
    end)

    on_load(function (toolchain)
        local sdk = os.getenv("XWIN_SDK") or
                    "/home/user/storage/apps/skill-xp-notify-toolchain/xwin-sdk"

        toolchain:add("cxflags",
            "--target=x86_64-pc-windows-msvc",
            "/winsysroot" .. sdk,
            "/imsvc" .. sdk .. "/crt/include",
            "/imsvc" .. sdk .. "/sdk/include/ucrt",
            "/imsvc" .. sdk .. "/sdk/include/um",
            "/imsvc" .. sdk .. "/sdk/include/shared",
            { force = true })

        toolchain:add("shflags",
            "/libpath:" .. sdk .. "/crt/lib/x86_64",
            "/libpath:" .. sdk .. "/sdk/lib/um/x86_64",
            "/libpath:" .. sdk .. "/sdk/lib/ucrt/x86_64",
            { force = true })

        toolchain:add("ldflags",
            "/libpath:" .. sdk .. "/crt/lib/x86_64",
            "/libpath:" .. sdk .. "/sdk/lib/um/x86_64",
            "/libpath:" .. sdk .. "/sdk/lib/ucrt/x86_64",
            { force = true })
    end)
toolchain_end()

includes("lib/commonlibsse-ng")

target("SkillXPNotify", function()
    add_rules("commonlibsse-ng.plugin", {
        name        = "SkillXPNotify",
        author      = "RexTheCapt",
        description = "Per-action skill XP corner notifications for Skyrim SE/AE.",
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
end)
