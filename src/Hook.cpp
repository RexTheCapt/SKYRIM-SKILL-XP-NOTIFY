#include "pch.h"

#include "Hook.h"

#include <cstring>

namespace SkillXPNotify
{
    void Hook::Install()
    {
        // r5 is a diagnostic-only build. r4 crashed because copying 5 raw
        // bytes of the function's prologue split an instruction — when the
        // saved-prologue stub ran, the CPU fell off the end of half an
        // instruction into our `FF 25` jump-back and decoded garbage.
        //
        // To pick a prologue size that ends on an instruction boundary we
        // need the actual bytes at the AddSkillExperience entry. Log them
        // and bail without patching anything, so the game runs unhooked
        // and we can read the disassembly out of SkillXPNotify.log.
        REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(39413, 40488) };
        const auto src_addr = target.address();
        const auto* bytes   = reinterpret_cast<const std::uint8_t*>(src_addr);

        // Format the first 16 bytes as a hex string. Avoid spdlog's
        // variadic format-string path on a runtime-constructed buffer.
        char hex[16 * 3 + 1] = { 0 };
        for (std::size_t i = 0; i < 16; ++i) {
            const auto b = bytes[i];
            const auto hi = "0123456789abcdef"[(b >> 4) & 0xF];
            const auto lo = "0123456789abcdef"[b & 0xF];
            hex[i * 3 + 0] = hi;
            hex[i * 3 + 1] = lo;
            hex[i * 3 + 2] = (i == 15) ? '\0' : ' ';
        }

        SKSE::log::info(
            "DIAGNOSTIC r5: AddSkillExperience entry at 0x{:x} starts with: {}",
            src_addr, hex);
        SKSE::log::info(
            "DIAGNOSTIC r5: hook NOT installed this round — game runs unhooked.");
    }
}  // namespace SkillXPNotify
