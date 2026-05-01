#include "pch.h"

#include "CSFIntegration.h"

#include <cstring>

namespace SkillXPNotify::CSFIntegration
{
    namespace
    {
        // CSF's SKSE messaging contract — defined here directly rather
        // than vendoring its headers, so we don't pull CSF's project
        // structure into our build. The struct layout MUST match
        // `CustomSkills::detail::CustomSkillsInterface` in
        // https://github.com/Exit-9B/CustomSkills (MIT licensed,
        // include/CustomSkills/Stubs.h).
        //
        // If CSF bumps this layout, kSupportedInterfaceVersion below
        // protects us — a mismatched version is logged and the proxy is
        // discarded rather than dereferenced.

        constexpr const char*  kSenderName            = "CustomSkills";
        constexpr std::uint32_t kInterfaceMessageType = 0;  // CSF::kCustomSkillsInterface
        constexpr std::uint32_t kSupportedVersion     = 1;  // CSF::CustomSkillsInterface::kVersion

        struct CSFInterfaceProxy
        {
            std::uint32_t interface_version;
            void  (*ShowStatsMenu)(const char* a_skill_id);
            void  (*AdvanceSkill)(const char* a_skill_id, float a_magnitude);
            void  (*IncrementSkill)(const char* a_skill_id, std::uint32_t a_count);
            void* (*GetEventDispatcher)(std::uint32_t a_dispatcher_id);
        };
        static_assert(sizeof(CSFInterfaceProxy) == 0x28,
                      "CSF interface ABI changed — bump kSupportedVersion + audit fields");

        // Stashed for future per-XP integration. v1.2.0 only verifies
        // presence; v2.x will subscribe to a per-XP event once one
        // exists upstream (the existing SkillIncreaseEvent fires on
        // rank-up only and would duplicate CSF's own notification).
        const CSFInterfaceProxy* g_csf = nullptr;

        void OnMessage(SKSE::MessagingInterface::Message* a_msg)
        {
            if (!a_msg || !a_msg->sender) {
                return;
            }
            // Per-sender RegisterListener already filters this, but be
            // defensive in case SKSE's filter ever weakens.
            if (std::strcmp(a_msg->sender, kSenderName) != 0) {
                return;
            }
            if (a_msg->type != kInterfaceMessageType) {
                return;
            }

            const auto* proxy =
                static_cast<const CSFInterfaceProxy*>(a_msg->data);
            if (!proxy) {
                SKSE::log::warn(
                    "CSF: dispatch arrived but data was null; ignoring.");
                return;
            }

            // Version gate — refuse to use a proxy whose ABI we don't
            // know. We'd rather log + skip than crash dereferencing
            // function pointers at unknown offsets.
            if (proxy->interface_version != kSupportedVersion) {
                SKSE::log::warn(
                    "CSF detected with interface v{}, but plugin built "
                    "against v{}; declining integration to avoid ABI mismatch. "
                    "Update SkillXPNotify when this version is supported.",
                    proxy->interface_version, kSupportedVersion);
                return;
            }

            g_csf = proxy;
            SKSE::log::info(
                "CSF detected (interface v{}). Per-XP integration is not "
                "yet implemented — pending an upstream event that fires "
                "on every Skill::Advance() call (the existing "
                "SkillIncreaseEvent is rank-up-only). See "
                "https://github.com/Exit-9B/CustomSkills",
                g_csf->interface_version);
        }
    }  // namespace

    void Register()
    {
        auto* msg = SKSE::GetMessagingInterface();
        if (!msg) {
            SKSE::log::warn(
                "CSF: SKSE messaging interface unavailable; CSF detection "
                "disabled (this should never happen in a normal SKSE load).");
            return;
        }
        if (!msg->RegisterListener(kSenderName, OnMessage)) {
            SKSE::log::warn(
                "CSF: failed to register messaging listener; CSF detection "
                "disabled. Plugin still works for vanilla skills.");
            return;
        }
        SKSE::log::info(
            "CSF: messaging listener registered. Will activate if "
            "Custom Skills Framework is loaded in this process.");
    }
}  // namespace SkillXPNotify::CSFIntegration
