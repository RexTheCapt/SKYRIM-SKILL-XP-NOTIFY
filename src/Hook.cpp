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
            // Diagnostic line on EVERY entry before we do anything. Tells
            // us whether the hook is firing at all, before _original or any
            // null-checks could silently drop the call.
            SKSE::log::info(
                "thunk-enter av={} delta={:+.4f} player=0x{:x}",
                static_cast<int>(a_skill),
                a_delta,
                reinterpret_cast<std::uintptr_t>(a_player));

            // Run the engine implementation first via the saved-prologue stub
            // so post-state reflects the gain and any rate multipliers from
            // coexisting plugins (Skill Uncapper et al.) have been applied.
            _original(a_player, a_skill, a_delta);

            SKSE::log::info("thunk-after-original");

            if (!a_player || !IsPlayerSkill(a_skill)) {
                SKSE::log::info("thunk-skip: not-player-skill");
                return;
            }
            auto* skills = a_player->skills;
            if (!skills || !skills->data) {
                SKSE::log::info("thunk-skip: skills/data null");
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

        // Per the r5 diagnostic, AddSkillExperience starts with:
        //   48 83 ec 48      sub rsp, 0x48           (4 bytes)
        //   0f 57 c0         xorps xmm0, xmm0        (3 bytes)
        //   0f 2f d0         comiss xmm2, xmm0       (3 bytes)
        //   76 1e            jbe +0x1e               (2 bytes)  ← rel8, can't relocate
        //
        // Copying the first 7 bytes (sub rsp + xorps) ends on an instruction
        // boundary and contains no RIP-relative or relative-jump payloads, so
        // the saved-prologue stub can execute freely from the trampoline.
        // The 5-byte rel32 JMP we write at the function entry leaves 2 stale
        // bytes; we fill them with NOPs so a thread that's mid-prologue when
        // we patch doesn't see torn bytes.
        constexpr std::size_t kPrologueSize = 7;
        constexpr std::size_t kAbsJmpSize   = 14;
        constexpr std::size_t kRel32JmpSize = 5;
        constexpr std::size_t kGatewaySize  = kPrologueSize + kAbsJmpSize +
                                              kAbsJmpSize;

        void BuildAbsoluteJmp(std::uint8_t* a_dst, std::uint64_t a_target)
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
        }

        bool WriteEntryPatch(std::uintptr_t a_src,
                             std::uintptr_t a_dst,
                             std::size_t    a_total_size)
        {
            const std::int64_t disp =
                static_cast<std::int64_t>(a_dst) -
                static_cast<std::int64_t>(a_src + kRel32JmpSize);
            if (disp > INT32_MAX || disp < INT32_MIN) {
                SKSE::log::error(
                    "rel32 JMP out of range: src=0x{:x} dst=0x{:x} disp={}",
                    a_src, a_dst, disp);
                return false;
            }

            std::uint8_t patch[kPrologueSize];
            patch[0]               = 0xE9;
            const std::int32_t d32 = static_cast<std::int32_t>(disp);
            std::memcpy(patch + 1, &d32, sizeof(d32));
            // Fill any leftover bytes (here: kPrologueSize - kRel32JmpSize = 2)
            // with single-byte NOPs.
            for (std::size_t i = kRel32JmpSize; i < a_total_size; ++i) {
                patch[i] = 0x90;
            }
            REL::safe_write(a_src, patch, a_total_size);
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

        // Layout in the gateway:
        //   [0 .. 7)         saved 7 bytes of original prologue   ──╮ _original
        //   [7 .. 21)        FF 25 + abs64(func+7)                ──╯ starts
        //                    (jump-back trampoline that resumes
        //                     execution at the comiss xmm2,xmm0
        //                     instruction)
        //   [21 .. 35)       FF 25 + abs64(&Thunk)
        //                    (entry redirector — what the patched
        //                     function entry rel32-jumps to)

        std::memcpy(gateway,
                    reinterpret_cast<const void*>(src_addr),
                    kPrologueSize);

        BuildAbsoluteJmp(gateway + kPrologueSize, src_addr + kPrologueSize);

        BuildAbsoluteJmp(gateway + kPrologueSize + kAbsJmpSize,
                         reinterpret_cast<std::uint64_t>(&Thunk));

        _original = reinterpret_cast<AddSkillExperience_t>(gateway);

        const auto redirector_addr =
            reinterpret_cast<std::uintptr_t>(gateway) + kPrologueSize +
            kAbsJmpSize;

        if (!WriteEntryPatch(src_addr, redirector_addr, kPrologueSize)) {
            _original = nullptr;
            return;
        }

        SKSE::log::info(
            "AddSkillExperience hook installed: target=0x{:x} gateway=0x{:x} "
            "(7-byte prologue copy: sub rsp 0x48 + xorps xmm0 xmm0).",
            src_addr,
            reinterpret_cast<std::uintptr_t>(gateway));
    }
}  // namespace SkillXPNotify
