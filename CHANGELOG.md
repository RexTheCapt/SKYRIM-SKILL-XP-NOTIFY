# Changelog

All notable changes to SkillXPNotify, in reverse chronological order.
Versions follow [SemVer](https://semver.org/): `MAJOR.MINOR.PATCH`. Each
release is also a git tag (e.g. `v1.1.0`) on the public repo.

## [1.1.0] — 2026-04-30

### Added

- **Live INI reload.** Edit `SkillXPNotify.ini` in any text editor while
  the game is running. Changes apply on:
  - the **F11 hotkey** (configurable via `[reload].key_code`; set to `0`
    to disable). A "SkillXPNotify: config reloaded." corner notification
    confirms.
  - any **save-load** or **new-game** event, automatically (driven by
    SKSE's `kPostLoadGame` / `kNewGame` messages).
- **`[reload]` INI section** with `key_code` (DirectInput scancode,
  default `0x57` = F11). Decimal or `0x`-prefixed hex both accepted.

### Fixed

- **Skill rename mods now show the right name.** Notifications used to
  hardcode the vanilla skill names ("Pickpocket", "Lockpicking", etc.),
  so mods like *Hand to Hand - An Adamant Addon* that rename skills
  via AVIF `FULL` overrides showed the wrong text. v1.1.0 reads the
  display name live from
  `RE::ActorValueList::GetActorValueInfo(av)->GetFullName()` — the same
  field Skyrim's stats menu uses — so any rename loaded by the engine
  is reflected automatically. XP tracking accuracy was already correct
  in v1.0.0 (it keys off the `ActorValue` integer ID).

### Internal

- New module `src/HotkeyReload.{h,cpp}` — `BSTEventSink<RE::InputEvent*>`
  attached to `RE::BSInputDeviceManager::GetSingleton()`.
- `Plugin.cpp` registers a `MessagingInterface` listener for the reload
  events; hotkey sink is installed on `kInputLoaded`.
- `Config::Load()` is now safe to call repeatedly. Hot-path
  `Config::Get()` reads stay lock-free atomic.

## [1.0.0] — 2026-04-30

First public release. Per-action skill XP corner notifications for
Skyrim SE/AE 1.6.1170.

### Added

- **`AddSkillExperience` hook** — observes every player skill XP grant
  the engine awards. Hand-rolled trampoline detour writing a 5-byte
  rel32 JMP at the function entry into a gateway allocated by SKSE's
  `Trampoline::allocate()`; copies a boundary-aligned 7-byte prologue
  (`sub rsp, 0x48` + `xorps xmm0, xmm0`) for the saved-prologue stub.
  Address Library `RELOCATION_ID(39413, 40488)`.
- **Corner notifications** in the format `+<delta> <SkillName> -> <pct>%`
  via `RE::SendHUDMessage::ShowHUDMessage`, dispatched on the main
  thread through `SKSE::GetTaskInterface()->AddTask`.
- **Throttle.** Per-skill leading-edge wait (default 1.5 s) accumulates
  XP within a window then emits ONE notification with the sum, so
  bursty activities like sneaking don't flood the queue. A live read
  of `RE::HUDNotifications::queue.size()` defers new notifications
  until earlier ones have finished displaying — never two stacked
  on screen.
- **`SkillXPNotify.ini`** for runtime config:
  - `[throttle].interval_ms` — per-skill window (ms).
  - `[throttle].post_emit_guard_ms` — dead time after each emit (ms).
  - `[skip].<SkillName>` — suppress corner notifications for a specific
    skill (still logged to file). Aliases recognised:
    archery↔marksman, speech↔speechcraft.
  - Section/key names are case-insensitive and ignore spaces / hyphens
    / underscores.
- **Detailed log** at
  `Documents\My Games\Skyrim Special Edition\SKSE\SkillXPNotify.log`
  with raw delta, current xp, and threshold for every grant.
- **MIT licensed.** Vendored DirectXTK `SimpleMath.h` / `.inl` keep
  their original MIT notice.

### Compatibility

- Skyrim SE/AE 1.6.1170. Other 1.6.x builds may work but untested.
- Requires SKSE 2.2.6+ and Address Library for SKSE Plugins v11
  (Nexus 32444).
- VR is not supported.
- Plays cleanly with Skill Uncapper et al. — observes after the engine
  has applied the gain, so any rate multipliers are reflected in the
  displayed delta.
- Single ~970 KB DLL, statically linked, pure system-DLL imports
  (KERNEL32 / SHELL32 / USER32 / VERSION / ole32). No ESP, no Papyrus,
  no save-game state.
