#include "pch.h"

#include "Notify.h"

#include <RE/S/SendHUDMessage.h>

#include <array>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace SkillXPNotify::Notify
{
    namespace
    {
        // Indexed by PlayerSkills::Data::Skills::Skill (kOneHanded=0 .. kEnchanting=17).
        // Display strings — what the user actually sees in the corner.
        constexpr std::array<std::string_view, 18> kSkillDisplayNames = {
            "One-Handed",  "Two-Handed", "Archery",     "Block",
            "Smithing",    "Heavy Armor", "Light Armor", "Pickpocket",
            "Lockpicking", "Sneak",       "Alchemy",     "Speech",
            "Alteration",  "Conjuration", "Destruction", "Illusion",
            "Restoration", "Enchanting"
        };

        constexpr int SkillIndex(RE::ActorValue a_av)
        {
            return static_cast<int>(a_av) -
                   static_cast<int>(RE::ActorValue::kOneHanded);
        }
    }  // namespace

    void Dispatch(RE::ActorValue a_skill, float a_delta, float a_pct)
    {
        const auto idx = SkillIndex(a_skill);
        if (idx < 0 || idx >= static_cast<int>(kSkillDisplayNames.size())) {
            return;
        }

        // as a tofu glyph in Skyrim's default font. Start with ASCII "->".
        // We can switch to "→" once the in-game test confirms it renders.
        std::string msg = std::format("+{:.1f} {} -> {:.0f}%",
                                      a_delta,
                                      kSkillDisplayNames[idx],
                                      a_pct);

        const auto* task = SKSE::GetTaskInterface();
        if (!task) {
            // Fallback: SKSE task interface unavailable (very early in load).
            // Drop the notification rather than risking a UI call from
            // whatever thread we're on.
            SKSE::log::warn(
                "Notify::Dispatch dropped (no TaskInterface): {}", msg);
            return;
        }

        task->AddTask([m = std::move(msg)]() {
            RE::SendHUDMessage::ShowHUDMessage(
                m.c_str(),
                /*soundToPlay=*/nullptr,
                /*cancelIfAlreadyQueued=*/false);
        });
    }
}  // namespace SkillXPNotify::Notify
