#include "pch.h"

#include "Throttle.h"

#include "Notify.h"

#include <array>
#include <chrono>
#include <mutex>

namespace SkillXPNotify::Throttle
{
    namespace
    {
        // Per-skill accumulator. The engine calls AddSkillExperience from
        // game-update threads, so guard with a mutex even though contention
        // is rare in practice.
        struct PerSkill
        {
            float                                              accumulated_delta = 0.0f;
            float                                              latest_pct        = 0.0f;
            std::chrono::steady_clock::time_point              last_fire         = {};
        };

        std::array<PerSkill, 18> g_state{};
        std::mutex               g_mtx;

        constexpr auto kMinInterval = std::chrono::seconds(1);

        constexpr int SkillIndex(RE::ActorValue a_av)
        {
            return static_cast<int>(a_av) -
                   static_cast<int>(RE::ActorValue::kOneHanded);
        }
    }  // namespace

    void OnXPGain(RE::ActorValue a_skill, float a_delta, float a_pct)
    {
        const auto idx = SkillIndex(a_skill);
        if (idx < 0 || idx >= static_cast<int>(g_state.size())) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();

        float emit_delta = 0.0f;
        float emit_pct   = 0.0f;
        bool  do_emit    = false;

        {
            std::lock_guard lock(g_mtx);
            auto& s = g_state[idx];
            s.accumulated_delta += a_delta;
            s.latest_pct = a_pct;

            // Emit when the per-skill cool-down has elapsed AND we have
            // something positive accumulated. The first call after process
            // start always fires (last_fire defaults to the epoch). We
            // intentionally don't try to flush the tail accumulation when
            // the user stops — it'll roll into the next grant for that
            // skill, which is fine for v1.
            if (now - s.last_fire >= kMinInterval &&
                s.accumulated_delta > 0.0f) {
                emit_delta = s.accumulated_delta;
                emit_pct   = s.latest_pct;
                s.accumulated_delta = 0.0f;
                s.last_fire = now;
                do_emit = true;
            }
        }

        if (do_emit) {
            Notify::Dispatch(a_skill, emit_delta, emit_pct);
        }
    }
}  // namespace SkillXPNotify::Throttle
