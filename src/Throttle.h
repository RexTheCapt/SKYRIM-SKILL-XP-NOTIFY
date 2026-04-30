#pragma once

namespace SkillXPNotify::Throttle
{
    // Called from Hook::Thunk on every player-skill XP grant. Accumulates
    // the delta per skill and emits a single Notify::Dispatch when the
    // per-skill 1-second cool-down has elapsed. Pre-throttle the engine
    // sometimes fires 50+ grants per second on sneak; raw-dispatching all
    // of them backs up Skyrim's notification queue for minutes.
    void OnXPGain(RE::ActorValue a_skill, float a_delta, float a_pct);
}
