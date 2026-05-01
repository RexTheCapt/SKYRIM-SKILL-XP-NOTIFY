# Install Log

Every external tool, package, or sizeable artifact installed for the SkillXPNotify project is recorded here. Read **bottom-to-top** to fully reverse the project's footprint on this machine.

The intent: nothing this project introduces should be undiscoverable later. If a future maintainer (or a future fresh OS install) wants to know what got added because of this work, this file is the single source of truth.

Format per entry:

- **Name & version**
- **Source / how installed**
- **Install location & approx size**
- **Reverse / uninstall command**

---

## 2026-04-29 — apt packages: clang-20, lld-20, llvm-20, libclang-rt-20-dev, cmake, ninja-build

System packages, installed by user via:

```
sudo apt install clang-20 lld-20 llvm-20 libclang-rt-20-dev cmake ninja-build
```

- **Source:** Ubuntu apt repositories.
- **Versions targeted:** clang/lld/llvm `1:20.0-63ubuntu1`, cmake `3.31.6-2ubuntu6`, ninja-build `1.12.1-1`.
- **Install location:** system-wide under `/usr/bin/`, `/usr/lib/llvm-20/`, `/usr/include/llvm-20/`, etc.
- **Approx size:** ~1.3 GB system-installed (all six packages plus auto-installed deps).
- **Reverse:**

  ```
  sudo apt remove --purge clang-20 lld-20 llvm-20 libclang-rt-20-dev cmake ninja-build
  sudo apt autoremove --purge
  ```

  Note: `cmake` and `ninja-build` are general dev tools and may be wanted by other projects on this system. Keep them installed unless intentionally cleaning everything up.

## 2026-04-29 — xwin 0.9.0

`xwin` is a tool that downloads and unpacks the MSVC toolchain headers and import libraries needed for cross-compiling Windows binaries from Linux with `clang-cl`. <https://github.com/Jake-Shadle/xwin>

- **Source:** GitHub release tarball — `https://github.com/Jake-Shadle/xwin/releases/download/0.9.0/xwin-0.9.0-x86_64-unknown-linux-musl.tar.gz`
- **Install location:** `/home/user/storage/apps/skill-xp-notify-toolchain/xwin` (single static binary, ~7.5 MB).
- **Reverse:**

  ```
  rm /home/user/storage/apps/skill-xp-notify-toolchain/xwin
  ```

  (Removed alongside the rest of the toolchain dir in the final cleanup step.)

## 2026-04-29 — Windows SDK + MSVC CRT (fetched by xwin)

Microsoft Windows SDK and MSVC CRT headers + import libraries, fetched + splatted by `xwin` for use as a `--target=x86_64-pc-windows-msvc` sysroot when cross-compiling with `clang-cl-20`.

- **Source:** Microsoft VS / Win SDK packages (license accepted via `xwin --accept-license`).
- **xwin command run:**

  ```
  cd /home/user/storage/apps/skill-xp-notify-toolchain
  ./xwin --accept-license splat --output ./xwin-sdk
  ```

- **Install location:** `/home/user/storage/apps/skill-xp-notify-toolchain/xwin-sdk/` containing `crt/{include,lib}` and `sdk/{Include,Lib,include,lib}` (lowercase aliases are casefold symlinks for Linux).
- **Size:** **630 MB** (`crt` 110 MB, `sdk` 520 MB; default `desktop` variant, `x86_64` arch only, no ATL, no debug runtime).
- **Intermediate `.xwin-cache/`** (1.1 GB) was deleted after splat completed — not needed once `xwin-sdk/` is produced. To recreate: re-run the xwin command above (it will redownload).
- **Reverse:**

  ```
  rm -rf /home/user/storage/apps/skill-xp-notify-toolchain/xwin-sdk
  ```

  (Removed alongside the rest of the toolchain dir in the final cleanup step.)

## 2026-04-29 — xmake 3.0.8

xmake was tried briefly during M3 as the build system before the project pivoted to CMake + vcpkg. Installed portably under the toolchain dir as a standalone bundle (no system pollution); kept around in case future tooling needs it.

- **Source:** GitHub release tarball — `https://github.com/xmake-io/xmake/releases/download/v3.0.8/xmake-bundle-v3.0.8.linux.x86_64`
- **Install location:** `/home/user/storage/apps/skill-xp-notify-toolchain/xmake/bin/xmake` (single self-contained binary, ~2.6 MB).
- **Reverse:**

  ```
  rm -rf /home/user/storage/apps/skill-xp-notify-toolchain/xmake
  ```

  (Removed alongside the rest of the toolchain dir in the final cleanup step.)

## 2026-04-29 — vcpkg (manifest mode)

Used by the CMake build (after the M3 pivot away from xmake) to fetch and build the plugin's transitive deps under our `clang-cl-xwin` cross-compile toolchain.

- **Source:** `git clone --depth 1 https://github.com/microsoft/vcpkg.git` then `./bootstrap-vcpkg.sh -disableMetrics`.
- **Install location:** `/home/user/storage/apps/skill-xp-notify-toolchain/vcpkg/`. The bootstrap also downloads vcpkg's own pinned cmake (4.2.3) and ninja into `vcpkg/downloads/tools/`.
- **Approx size:** ~90 MB after bootstrap; grows to ~200–400 MB once ports (`spdlog`, `directxmath`, transitively `fmt`) build into `vcpkg/buildtrees/` and `vcpkg/installed/`.
- **What gets installed per build:** see `vcpkg.json` in the project root. Currently pulls `spdlog` (with `wchar` feature) and `directxmath`. DirectXTK is **not** pulled — its vcpkg port runs `CompileShaders.cmd` (a Windows `.cmd` invoking `fxc.exe`) which won't run on Linux. We provide a stub `find_package(directxtk CONFIG REQUIRED)` config under `cmake/stub-directxtk/` plus a vendored copy of `SimpleMath.h`/`.inl` (the only DirectXTK header the lib actually uses; SimpleMath is fully inline so no .lib symbols are needed).
- **Reverse:**

  ```
  rm -rf /home/user/storage/apps/skill-xp-notify-toolchain/vcpkg
  ```

  (Removed alongside the rest of the toolchain dir in the final cleanup step.)

## 2026-04-29 — Pascal-case symlinks under xwin-sdk/sdk/lib/um/x86_64/

CommonLibVR's `CMakeLists.txt` links libraries with mixed-case names (`Advapi32.lib`, `Dbghelp.lib`, `D3D11.lib`, `Ole32.lib`, `Version.lib`, etc.) but xwin's splatted SDK ships only `ALLCAPS.lib` / `lowercase.lib` / one mixed form per library — none of which match those Pascal-case forms. Linux is case-sensitive, so the link fails. We add a small set of symlinks pointing at the lowercase originals.

- **Files added:** `Advapi32.lib`, `Dbghelp.lib`, `Ole32.lib`, `Version.lib`, `D3D11.lib`, `D3dcompiler.lib`, `Dxgi.lib` under `/home/user/storage/apps/skill-xp-notify-toolchain/xwin-sdk/sdk/lib/um/x86_64/`. Each is a symlink to its lowercase sibling (a few KB total).
- **Reverse:** they live inside the xwin-sdk directory which is already covered by the xwin-sdk uninstall entry above; the parent `rm -rf` of `xwin-sdk/` removes them. To remove just the symlinks while keeping xwin-sdk:

  ```
  cd /home/user/storage/apps/skill-xp-notify-toolchain/xwin-sdk/sdk/lib/um/x86_64/
  for n in Advapi32 Dbghelp Ole32 Version D3D11 D3dcompiler Dxgi; do
      [ -L "$n.lib" ] && rm "$n.lib"
  done
  ```

## 2026-04-29 — toolchain verification artifact

Tiny hello-world Windows DLL produced as a smoke test of the cross-compile pipeline. Not part of the plugin itself; safe to delete at any time.

- **Files:** `/home/user/storage/apps/skill-xp-notify-toolchain/test/{hello.cpp,build.sh}` (build script reproduces `hello.dll` on demand).
- **Verified:** `hello.dll` built as `PE32+ executable for MS Windows 6.00 (DLL), x86-64, 5 sections`. Confirms clang-cl-20 + lld-link-20 + xwin SDK pipeline is functional.
- **Reverse:** `rm -rf /home/user/storage/apps/skill-xp-notify-toolchain/test` (removed alongside the rest of the toolchain dir in the final cleanup step).

---

## Footprint summary (post-milestone 2)

| Where | Size | Reversible by |
|---|---|---|
| apt packages (system) | ~1.3 GB | `sudo apt remove --purge ...` (see top of file) |
| `/home/user/storage/apps/skill-xp-notify-toolchain/` | ~750 MB–1 GB (xwin binary 7.5 MB + xwin-sdk 630 MB + xmake 2.6 MB + test/ + vcpkg ~90 MB bootstrap, grows with port cache) | `rm -rf` of that directory |
| **Total** | **~2.0–2.3 GB** | over the original 2 GB approved budget once vcpkg ports are cached; stays under if `vcpkg/buildtrees/` and `vcpkg/downloads/` are pruned periodically. |

---

## Final cleanup (single command)

When the project is done with and everything project-introduced should go away:

```
# system packages (only if you don't want them for other projects)
sudo apt remove --purge clang-20 lld-20 llvm-20 libclang-rt-20-dev cmake ninja-build
sudo apt autoremove --purge

# self-contained toolchain dir (xwin binary + downloaded Windows SDK)
rm -rf /home/user/storage/apps/skill-xp-notify-toolchain

# the project source tree itself (only if you also want to remove the repo)
rm -rf /home/user/storage/apps/skill-xp-notify

# the deployed plugin in the game install (if it was installed)
rm -f "/home/user/storage/SteamLibrary/steamapps/common/Skyrim Special Edition/Data/SKSE/Plugins/SkillXPNotify.dll"
rm -f "/home/user/storage/SteamLibrary/steamapps/common/Skyrim Special Edition/Data/SKSE/Plugins/SkillXPNotify.toml"
rm -f "/home/user/storage/SteamLibrary/steamapps/common/Skyrim Special Edition/Data/SKSE/Plugins/SkillXPNotify.ini"
```
