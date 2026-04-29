#pragma once

namespace SkillXPNotify
{
    struct Hook
    {
        // Install the trampoline on RE::PlayerCharacter::AddSkillExperience
        // (Address Library 39413 / 40488). Call once from SKSEPlugin_Load
        static void Install();
    };
}
