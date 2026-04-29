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
| `/home/user/storage/apps/skill-xp-notify-toolchain/` | ~638 MB | `rm -rf` of that directory |
| **Total** | **~1.94 GB** | within the approved 2 GB budget |

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
