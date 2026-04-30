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
        using clock      = std::chrono::steady_clock;
        using time_point = clock::time_point;

        // Per-skill state. window_start records the time of the FIRST grant
        // in the current accumulation window (zero = "no window armed").
        // After we emit, window_start is reset so the next grant starts a
        // fresh window — this gives the user-visible "leading-edge wait"
        // behaviour: when you start using a skill the first message holds
        // off until the window has elapsed, then shows the total XP gained
        // during that window.
        struct PerSkill
        {
            float      accumulated_delta = 0.0f;
            float      latest_pct        = 0.0f;
            time_point window_start      = {};
        };

        std::array<PerSkill, 18> g_state{};

        // Global serialiser: a model of when Skyrim's HUDNotifications
        // queue is expected to be empty. Each emit pushes this forward by
        // kDisplayTime; emits before this time are deferred so we don't
        // stack messages on top of each other in-game.
        time_point g_queue_clear_time = {};

        std::mutex g_mtx;

        // Per-skill leading-edge wait. Long enough to feel like an
        // accumulation, short enough to feel responsive on bursty actions.
        constexpr auto kMinInterval = std::chrono::milliseconds(1500);

        // Conservative model of Skyrim's per-notification display time
        // (default fNotificationDisplayTime ≈ 3 s, plus a buffer so we
        // don't race the drain). M8 (INI) will expose this knob.
        constexpr auto kDisplayTime = std::chrono::milliseconds(3500);

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

        const auto now = clock::now();

        float emit_delta = 0.0f;
        float emit_pct   = 0.0f;
        bool  do_emit    = false;

        {
            std::lock_guard lock(g_mtx);
            auto& s = g_state[idx];

            s.accumulated_delta += a_delta;
            s.latest_pct = a_pct;

            // Leading edge: first grant of a quiet window. Start the
            // timer and bail without emitting — we want the in-game
            // notification to *delay* the message after activity begins,
            // not flash one immediately.
            if (s.window_start == time_point{}) {
                s.window_start = now;
                return;
            }

            // Still accumulating inside the per-skill window.
            if (now - s.window_start < kMinInterval) {
                return;
            }

            // Window elapsed. Defer if Skyrim's queue is still expected
            // to be displaying earlier notifications — keep accumulating
            // and the next grant will retry.
            if (now < g_queue_clear_time) {
                return;
            }

            if (s.accumulated_delta > 0.0f) {
                emit_delta          = s.accumulated_delta;
                emit_pct            = s.latest_pct;
                s.accumulated_delta = 0.0f;
                s.window_start      = time_point{};

                // Push the modeled queue-clear forward. If we're already
                // past the previous clear time the new emit anchors at
                // `now`; if we somehow got here while still within the
                // drain window (shouldn't, due to the check above) we
                // chain from `g_queue_clear_time` instead so the model
                // stays monotonic.
                // (Windows.h's `max` macro shadows std::max in this TU,
                // so write the take-the-later-of-two with a ternary.)
                const time_point anchor =
                    (now > g_queue_clear_time) ? now : g_queue_clear_time;
                g_queue_clear_time = anchor + kDisplayTime;
                do_emit = true;
            }
        }

        if (do_emit) {
            Notify::Dispatch(a_skill, emit_delta, emit_pct);
        }
    }
}  // namespace SkillXPNotify::Throttle
