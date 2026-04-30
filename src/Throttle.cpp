#include "pch.h"

#include "Throttle.h"

#include "Config.h"
#include "Notify.h"

#include <RE/H/HUDMenu.h>
#include <RE/H/HUDNotifications.h>
#include <RE/U/UI.h>

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

        // Last time we Notify::Dispatch'd. Even if Skyrim's queue reads
        // empty, it can take a frame or two for the notification to
        // materialise — kPostEmitGuard prevents us from emitting again
        // before the previous one shows up in the queue.
        time_point g_last_emit_time = {};

        std::mutex g_mtx;

        // Per-skill leading-edge wait + post-emit dead time. Tunable from
        // SkillXPNotify.ini (see Config.cpp); defaults match the hard-
        // coded values that shipped through r13.
        std::chrono::milliseconds MinInterval()
        {
            return std::chrono::milliseconds(
                Config::Get().throttle_ms.load());
        }

        std::chrono::milliseconds PostEmitGuard()
        {
            return std::chrono::milliseconds(
                Config::Get().post_emit_guard_ms.load());
        }

        // Returns the number of pending notifications in Skyrim's HUD.
        // Safe to call from the AddSkillExperience hook (we're on the
        // main thread). Returns 0 if the UI singleton or HUDMenu isn't
        // up yet (happens very early in load).
        std::size_t LiveQueueSize()
        {
            auto* ui = RE::UI::GetSingleton();
            if (!ui) {
                return 0;
            }
            auto hud = ui->GetMenu<RE::HUDMenu>();
            if (!hud) {
                return 0;
            }

            // RTTI-free type check: a HUDObject's vtable pointer is at
            // offset 0; HUDNotifications has a unique vtable address
            // resolved once via REL::VariantID.
            static const auto kHUDNotifVTable =
                RE::HUDNotifications::VTABLE[0].address();

            auto& rt = hud->GetRuntimeData();
            for (auto* obj : rt.objects) {
                if (!obj) {
                    continue;
                }
                const auto vt =
                    *reinterpret_cast<const std::uintptr_t*>(obj);
                if (vt == kHUDNotifVTable) {
                    auto* notifs = static_cast<RE::HUDNotifications*>(obj);
                    return notifs->queue.size();
                }
            }
            return 0;
        }

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
            if (now - s.window_start < MinInterval()) {
                return;
            }

            // Window elapsed. Two gates before emitting:
            //   (1) post-emit dead time — covers TaskInterface latency.
            //   (2) live HUDNotifications queue depth — defer until
            //       Skyrim has actually drained earlier notifications.
            // Either gate failing leaves the accumulator armed; the
            // next grant retries.
            if (now - g_last_emit_time < PostEmitGuard()) {
                return;
            }
            if (LiveQueueSize() > 0) {
                return;
            }

            if (s.accumulated_delta > 0.0f) {
                emit_delta          = s.accumulated_delta;
                emit_pct            = s.latest_pct;
                s.accumulated_delta = 0.0f;
                s.window_start      = time_point{};
                g_last_emit_time    = now;
                do_emit             = true;
            }
        }

        if (do_emit) {
            Notify::Dispatch(a_skill, emit_delta, emit_pct);
        }
    }
}  // namespace SkillXPNotify::Throttle
