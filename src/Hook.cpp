#include "pch.h"

#include "Hook.h"

#include <cstring>

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
            // Run the engine implementation first, via the saved-prologue stub
            // in the trampoline gateway, so post-state reads reflect the gain
            // and any rate multipliers from coexisting plugins (Skill Uncapper
            // et al.) have already been applied to xp/threshold.
            _original(a_player, a_skill, a_delta);

            if (!a_player || !IsPlayerSkill(a_skill)) {
                return;
            }
            auto* skills = a_player->skills;
            if (!skills || !skills->data) {
                return;
            }

            const auto  idx = SkillIndex(a_skill);
            const auto& sd  = skills->data->skills[idx];

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

        // Layout written into SKSE's trampoline buffer (allocated near the
        // game executable, so a 5-byte rel32 JMP from the function entry
        // can reach it):
        //
        //   offset 0 ..  4  saved 5-byte original prologue   ──╮  _original
        //   offset 5 .. 18  FF 25 00 00 00 00 + abs64(func+5) ─╯  starts here
        //   offset 19 .. 32 FF 25 00 00 00 00 + abs64(&Thunk)
        //
        // The function entry's first 5 bytes are overwritten with
        //   E9 + rel32(disp to gateway+19)
        // i.e. JMP to the bottom redirector. From there we absolute-JMP into
        // our DLL's Thunk (which can be far from the game .exe and so does
        // not fit in a rel32 from the function entry directly).
        //
        // Calling `_original(...)` from Thunk runs the saved prologue, then
        // JMPs back to func+5 to continue the engine's implementation
        // unchanged. Stack discipline matches a normal Win64 CALL.

        constexpr std::size_t kPrologueSize = 5;
        constexpr std::size_t kAbsJmpSize   = 14;
        constexpr std::size_t kGatewaySize  = kPrologueSize + kAbsJmpSize +
                                              kAbsJmpSize;

        bool BuildAbsoluteJmp(std::uint8_t* a_dst, std::uint64_t a_target)
        {
            // FF 25 00 00 00 00     JMP qword ptr [rip+0]
            // <abs64 target>
            a_dst[0] = 0xFF;
            a_dst[1] = 0x25;
            a_dst[2] = 0x00;
            a_dst[3] = 0x00;
            a_dst[4] = 0x00;
            a_dst[5] = 0x00;
            std::memcpy(a_dst + 6, &a_target, sizeof(a_target));
            return true;
        }

        bool WriteRel32Jmp(std::uintptr_t a_src, std::uintptr_t a_dst)
        {
            const std::int64_t disp =
                static_cast<std::int64_t>(a_dst) -
                static_cast<std::int64_t>(a_src + 5);
            if (disp > INT32_MAX || disp < INT32_MIN) {
                SKSE::log::error(
                    "rel32 JMP out of range: src=0x{:x} dst=0x{:x} disp={}",
                    a_src, a_dst, disp);
                return false;
            }
            std::uint8_t patch[5];
            patch[0]                = 0xE9;
            const std::int32_t d32  = static_cast<std::int32_t>(disp);
            std::memcpy(patch + 1, &d32, sizeof(d32));
            REL::safe_write(a_src, patch, sizeof(patch));
            return true;
        }
    }  // namespace

    void Hook::Install()
    {
        REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(39413, 40488) };
        const auto src_addr = target.address();

        SKSE::AllocTrampoline(64);
        auto& tramp = SKSE::GetTrampoline();

        auto* gateway = static_cast<std::uint8_t*>(
            tramp.allocate(kGatewaySize));
        if (!gateway) {
            SKSE::log::error(
                "Trampoline allocate({}) failed; aborting hook install.",
                kGatewaySize);
            return;
        }

        // [0..5)   saved prologue
        std::memcpy(gateway,
                    reinterpret_cast<const void*>(src_addr),
                    kPrologueSize);

        // [5..19)  abs JMP back to function+5 (continues original execution)
        BuildAbsoluteJmp(gateway + kPrologueSize, src_addr + kPrologueSize);

        // [19..33) abs JMP to our Thunk (in the plugin DLL, far from the exe)
        BuildAbsoluteJmp(gateway + kPrologueSize + kAbsJmpSize,
                         reinterpret_cast<std::uint64_t>(&Thunk));

        // _original points at the saved prologue, which falls through to the
        // JMP-back stub. Invoking it from Thunk runs the original function.
        _original = reinterpret_cast<AddSkillExperience_t>(gateway);

        // Patch the function entry: 5-byte rel32 JMP into the bottom
        // redirector slot of the gateway.
        const auto redirector_addr =
            reinterpret_cast<std::uintptr_t>(gateway) + kPrologueSize +
            kAbsJmpSize;
        if (!WriteRel32Jmp(src_addr, redirector_addr)) {
            _original = nullptr;
            return;
        }

        SKSE::log::info(
            "AddSkillExperience hook installed: target=0x{:x} gateway=0x{:x} "
            "(relocation 39413/40488).",
            src_addr,
            reinterpret_cast<std::uintptr_t>(gateway));
    }
}  // namespace SkillXPNotify
