#include "pch.h"

#include "CSFIntegration.h"
#include "Config.h"
#include "Hook.h"
#include "HotkeyReload.h"

#include <spdlog/sinks/basic_file_sink.h>

namespace
{
    void InitLogging()
    {
        auto path = SKSE::log::log_directory();
        if (!path) {
            return;
        }
        *path /= std::string{ SKSE::PluginDeclaration::GetSingleton()->GetName() } + ".log";

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto logger = std::make_shared<spdlog::logger>("global", std::move(sink));
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(logger));
        spdlog::set_pattern("[%Y-%m-%d %T.%e] [%l] %v");
    }
}

namespace
{
    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg)
    {
        if (!a_msg) {
            return;
        }
        switch (a_msg->type) {
            case SKSE::MessagingInterface::kInputLoaded:
                // First moment the input device manager is up; install
                // the hotkey listener now so subsequent F11 presses
                // (or whatever the user configured) trigger reloads.
                SkillXPNotify::HotkeyReload::Install();
                break;

            case SKSE::MessagingInterface::kPostLoadGame:
            case SKSE::MessagingInterface::kNewGame:
                // Natural game-state moments where players reasonably
                // expect any tweaks they made to take effect — re-read
                // the .ini from disk so edits between sessions apply
                // without restarting the game.
                SkillXPNotify::Config::Load();
                SKSE::log::info(
                    "Config reloaded due to {} message.",
                    a_msg->type == SKSE::MessagingInterface::kNewGame
                        ? "kNewGame"
                        : "kPostLoadGame");
                break;

            default:
                break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    InitLogging();

    SKSE::log::info("SkillXPNotify loaded.");
    SkillXPNotify::Config::Load();
    SkillXPNotify::Hook::Install();
    SkillXPNotify::CSFIntegration::Register();

    if (auto* msg = SKSE::GetMessagingInterface()) {
        msg->RegisterListener(OnSKSEMessage);
    } else {
        SKSE::log::warn(
            "SKSE messaging interface unavailable — game-state reloads "
            "and hotkey will not be wired.");
    }

    return true;
}
