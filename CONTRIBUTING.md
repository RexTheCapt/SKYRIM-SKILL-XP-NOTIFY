# Contributing to SkillXPNotify

Mostly notes for myself, but if you do open a PR, this is the
convention the project follows.

## Commit messages

We use [Conventional Commits](https://www.conventionalcommits.org/) so
GitHub's "Generate release notes" button and our own
[`scripts/release-notes.sh`](scripts/release-notes.sh) can group commits
into the right sections automatically.

```
<type>(<scope>): <subject>

[body — wrapped at ~72 chars, explain *why*]

[footers — BREAKING CHANGE:, Co-Authored-By:, etc.]
```

### Allowed types

| Type | Section in release notes | When |
|---|---|---|
| `feat` | **Features** | New user-visible functionality |
| `fix` | **Bug Fixes** | Defect / regression fix |
| `perf` | **Performance** | Speed / memory / efficiency win |
| `docs` | **Documentation** | README / CHANGELOG / code comments / `.ini.example` |
| `refactor` | **Refactoring** | Internal restructure, no behavior change |
| `test` | **Tests** | Test-only changes |
| `build` | **Build / Toolchain** | CMake, vcpkg, scripts/, dist tooling |
| `chore` | **Maintenance** | Dep bumps, .gitignore, repo hygiene |

A `BREAKING CHANGE: <description>` footer triggers a major version
bump and gets its own **Breaking Changes** section in the release
notes.

### Recommended scopes

Use the existing project taxonomy when relevant:

- `hook` — `src/Hook.{h,cpp}` and the trampoline detour
- `notify` — `src/Notify.{h,cpp}`, corner-message dispatch
- `throttle` — `src/Throttle.{h,cpp}`, per-skill window + queue check
- `config` — `src/Config.{h,cpp}`, INI parsing
- `reload` — `src/HotkeyReload.{h,cpp}`, F11 + SKSE messaging listener
- `crt` — anything CRT/runtime-linkage related
- `m<N>` — when a commit closes or advances milestone N

### Examples

```
feat(reload): live INI reload via SKSE messaging + F11 hotkey

Edit SkillXPNotify.ini in any text editor; F11 or save-load applies.
Defaults match v1.0.0; key_code = 0 disables the hotkey.
```

```
fix(crt): static-link the MSVC runtime so the DLL loads under Proton

Wine prefix lacks UCRT forwarder DLLs; /MD-linked builds got bounced
by LoadLibrary before SKSEPlugin_Load ran.
```

```
build: switch from xmake to CMake + vcpkg

BREAKING CHANGE: build invocation changes — see new README section.
```

## Cutting a release

1. Bump `project(... VERSION x.y.z)` in `CMakeLists.txt` and
   `version-string` in `vcpkg.json`. Commit as `chore(release): bump
   to vx.y.z`.
2. Run `./scripts/release-notes.sh v<prev>..HEAD` to generate the
   markdown block for the release page.
3. Add a new top section to `CHANGELOG.md` matching that block
   (manual; auto-generated content is the source of truth, the
   CHANGELOG file is the human-readable mirror).
4. Tag: `git tag -a vx.y.z -m "release vx.y.z"`.
5. `git push origin main && git push origin vx.y.z`.
6. Run `./scripts/package.sh` to produce `dist/SkillXPNotify-x.y.z.zip`.
7. Create the GitHub release on the web UI:
   <https://github.com/RexTheCapt/SKYRIM-SKILL-XP-NOTIFY/releases/new>
   - **Choose a tag:** pick `vx.y.z` from the dropdown.
   - **Release title:** `vx.y.z` (or a one-line summary).
   - **Describe this release:** paste the output of
     `./scripts/release-notes.sh v<prev>..vx.y.z`.
   - **Attach:** drag `dist/SkillXPNotify-x.y.z.zip` onto the assets area.
   - Click *Publish release*.

## Building from source

See the [README](README.md#building-from-source).

## Identity in commits

Commits in this repo use the GitHub no-reply email
(`6693554+RexTheCapt@users.noreply.github.com`). If you push as
yourself, configure your own no-reply via `git config user.email
<your-id>+<your-handle>@users.noreply.github.com` so GitHub's
[email-privacy block](https://docs.github.com/en/account-and-profile/setting-up-and-managing-your-personal-account-on-github/managing-email-preferences/blocking-command-line-pushes-that-expose-your-personal-email-address)
doesn't reject the push.
