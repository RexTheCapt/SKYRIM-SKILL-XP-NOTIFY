#include "pch.h"

#include "Hook.h"

namespace SkillXPNotify
{
    namespace
    {
        using AddSkillExperience_t =
            void (*)(RE::PlayerCharacter*, RE::ActorValue, float);

        AddSkillExperience_t _original = nullptr;

        // ActorValue::kOneHanded is 6, the 18 player skills are contiguous
        // through ActorValue::kEnchanting, but PlayerSkills::Data::skills[]
        // is indexed 0..17. Convert by subtracting kOneHanded.
        constexpr int SkillIndex(RE::ActorValue a_av)
        {
            return static_cast<int>(a_av) -
                   static_cast<int>(RE::ActorValue::kOneHanded);
        }

        constexpr bool IsPlayerSkill(RE::ActorValue a_av)
        {
            const auto idx = SkillIndex(a_av);
            return idx >= 0 && idx < 18;
        }

        void Thunk(RE::PlayerCharacter* a_player,
                   RE::ActorValue a_skill,
                   float a_delta)
        {
            // Run the engine implementation first so post-state reads reflect
            // the gain — and so any rate multipliers from coexisting plugins
            // (Skill Uncapper et al.) have already been applied to xp/level.
            _original(a_player, a_skill, a_delta);

            if (!a_player || !IsPlayerSkill(a_skill)) {
                return;
            }
            auto* skills = a_player->skills;
            if (!skills || !skills->data) {
                return;
            }

            const auto idx = SkillIndex(a_skill);
            const auto& sd = skills->data->skills[idx];

            // Bare per-grant log line. M6 turns this into a corner
            // notification; M7 throttles + accumulates per skill.
            SKSE::log::info(
                "skill-xp av={} idx={} delta={:+.4f} level={:.2f} "
                "xp={:.4f} threshold={:.4f} pct={:.1f}%",
                static_cast<int>(a_skill),
                idx,
                a_delta,
                sd.level,
                sd.xp,
                sd.levelThreshold,
                sd.levelThreshold > 0.0f ? (sd.xp / sd.levelThreshold) * 100.0f
                                         : 0.0f);
        }
    }  // namespace

    void Hook::Install()
    {
        SKSE::AllocTrampoline(14);
        auto& tramp = SKSE::GetTrampoline();

        REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(39413, 40488) };

        _original = reinterpret_cast<AddSkillExperience_t>(
            tramp.write_branch<5>(target.address(),
                                  reinterpret_cast<std::uintptr_t>(&Thunk)));

        SKSE::log::info(
            "AddSkillExperience hook installed at 0x{:x} (relocation 39413/40488).",
            target.address());
    }
}  // namespace SkillXPNotify
