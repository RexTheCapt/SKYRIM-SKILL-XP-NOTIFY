#include "pch.h"

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

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    InitLogging();

    SKSE::log::info("SkillXPNotify loaded — milestone 3 skeleton, no hooks installed yet.");
    return true;
}
