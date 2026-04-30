#pragma once

#include <array>
#include <atomic>

namespace SkillXPNotify::Config
{
    // Runtime-tunable settings, populated from
    // Data/SKSE/Plugins/SkillXPNotify.ini at plugin load. Defaults match
    // the hard-coded constants the throttle shipped with through r13 so
    // a missing .ini behaves exactly like pre-M8 builds.
    struct Settings
    {
        // M7 throttle knobs.
        std::atomic<int> throttle_ms{ 1500 };
        std::atomic<int> post_emit_guard_ms{ 500 };

        // Per-skill skip mask, indexed by PlayerSkills::Data::Skills::Skill
        // (kOneHanded=0..kEnchanting=17). Skipped skills don't generate
        // corner notifications (the hook still runs and the verbose
        // skill-xp log line still gets written, just no in-game UI).
        std::array<std::atomic<bool>, 18> skip{};
    };

    // Single global instance. The hot path (Throttle, Hook) reads atomics;
    // Load() runs once during SKSEPlugin_Load before any reads happen.
    Settings& Get();

    // Parses Data/SKSE/Plugins/SkillXPNotify.ini next to our DLL and
    // applies the values to Get(). Logs whatever happens. Safe to call
    // even if the file doesn't exist — leaves defaults in place.
    void Load();
}
