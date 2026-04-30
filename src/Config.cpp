#include "pch.h"

#include "Config.h"

// Use the lib's REX::W32 abstraction so this TU stays out of <windows.h>
// territory (which would clobber std::min/max etc.).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace SkillXPNotify::Config
{
    namespace
    {
        Settings g_settings;

        // Skill name → index lookup. Lower-cased, hyphen-stripped so
        // either "OneHanded", "one-handed", "One Handed" all hit the
        // same entry.
        const std::unordered_map<std::string, int>& SkillIndexLookup()
        {
            static const std::unordered_map<std::string, int> kMap = {
                { "onehanded",   0 },
                { "twohanded",   1 },
                { "archery",     2 },
                { "marksman",    2 },
                { "block",       3 },
                { "smithing",    4 },
                { "heavyarmor",  5 },
                { "lightarmor",  6 },
                { "pickpocket",  7 },
                { "lockpicking", 8 },
                { "sneak",       9 },
                { "alchemy",    10 },
                { "speech",     11 },
                { "speechcraft",11 },
                { "alteration", 12 },
                { "conjuration",13 },
                { "destruction",14 },
                { "illusion",   15 },
                { "restoration",16 },
                { "enchanting", 17 },
            };
            return kMap;
        }

        std::string Normalise(std::string_view a_in)
        {
            std::string out;
            out.reserve(a_in.size());
            for (char c : a_in) {
                if (c == ' ' || c == '-' || c == '_' || c == '\t') {
                    continue;
                }
                out.push_back(static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c))));
            }
            return out;
        }

        // Trim ASCII whitespace + comments (`;` or `#`) from a line.
        std::string_view TrimAndStripComment(std::string_view a_in)
        {
            // Strip comment first so a `#` inside a value still terminates.
            const auto hash      = a_in.find_first_of(";#");
            std::string_view sv  = (hash == std::string_view::npos)
                                       ? a_in
                                       : a_in.substr(0, hash);
            while (!sv.empty() &&
                   std::isspace(static_cast<unsigned char>(sv.front()))) {
                sv.remove_prefix(1);
            }
            while (!sv.empty() &&
                   std::isspace(static_cast<unsigned char>(sv.back()))) {
                sv.remove_suffix(1);
            }
            return sv;
        }

        bool ParseBool(std::string_view a_in, bool a_default)
        {
            const auto norm = Normalise(a_in);
            if (norm == "1" || norm == "true" || norm == "yes" ||
                norm == "on") {
                return true;
            }
            if (norm == "0" || norm == "false" || norm == "no" ||
                norm == "off") {
                return false;
            }
            return a_default;
        }

        // Resolve the path of *this* DLL on disk and return the same
        // directory with `SkillXPNotify.ini` appended. Done via
        // GetModuleHandleEx with GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        // pointed at a code address inside us.
        std::filesystem::path IniPath()
        {
            HMODULE hMod = nullptr;
            if (!GetModuleHandleExW(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(&IniPath),
                    &hMod) ||
                !hMod) {
                return {};
            }

            wchar_t buf[MAX_PATH * 2] = { 0 };
            const auto n = GetModuleFileNameW(hMod, buf,
                                              static_cast<DWORD>(std::size(buf)));
            if (n == 0 || n >= std::size(buf)) {
                return {};
            }

            std::filesystem::path p(buf);
            p.replace_extension(L".ini");
            return p;
        }

        void ApplyKv(std::string_view a_section,
                     std::string_view a_key,
                     std::string_view a_value)
        {
            const auto section = Normalise(a_section);
            const auto key     = Normalise(a_key);

            if (section == "throttle") {
                if (key == "intervalms" || key == "throttlems" ||
                    key == "windowms") {
                    try {
                        g_settings.throttle_ms.store(
                            std::stoi(std::string{ a_value }));
                    } catch (...) {
                    }
                    return;
                }
                if (key == "postemitguardms" || key == "guardms") {
                    try {
                        g_settings.post_emit_guard_ms.store(
                            std::stoi(std::string{ a_value }));
                    } catch (...) {
                    }
                    return;
                }
                return;
            }

            if (section == "skip") {
                const auto& map = SkillIndexLookup();
                const auto  it  = map.find(key);
                if (it == map.end()) {
                    return;
                }
                g_settings.skip[it->second].store(ParseBool(a_value, false));
                return;
            }
        }
    }  // namespace

    Settings& Get()
    {
        return g_settings;
    }

    void Load()
    {
        const auto path = IniPath();
        if (path.empty()) {
            SKSE::log::warn(
                "Config: could not resolve plugin DLL path; defaults in use.");
            return;
        }

        std::ifstream f(path);
        if (!f) {
            SKSE::log::info(
                "Config: no SkillXPNotify.ini next to DLL; defaults in use "
                "(throttle_ms={}, post_emit_guard_ms={}).",
                g_settings.throttle_ms.load(),
                g_settings.post_emit_guard_ms.load());
            return;
        }

        std::string section;
        std::string line;
        while (std::getline(f, line)) {
            const auto sv = TrimAndStripComment(line);
            if (sv.empty()) {
                continue;
            }
            if (sv.front() == '[' && sv.back() == ']') {
                section = std::string{ sv.substr(1, sv.size() - 2) };
                continue;
            }
            const auto eq = sv.find('=');
            if (eq == std::string_view::npos) {
                continue;
            }
            const auto key   = TrimAndStripComment(sv.substr(0, eq));
            const auto value = TrimAndStripComment(sv.substr(eq + 1));
            ApplyKv(section, key, value);
        }

        // Summarise what we ended up with so a confused user can read
        // the log and see why their .ini didn't take effect.
        std::string skip_list;
        for (int i = 0; i < 18; ++i) {
            if (g_settings.skip[i].load()) {
                if (!skip_list.empty()) skip_list += ", ";
                skip_list += std::to_string(i);
            }
        }
        SKSE::log::info(
            "Config: loaded SkillXPNotify.ini — throttle_ms={}, "
            "post_emit_guard_ms={}, skip-indices=[{}].",
            g_settings.throttle_ms.load(),
            g_settings.post_emit_guard_ms.load(),
            skip_list.empty() ? "(none)" : skip_list.c_str());
    }
}  // namespace SkillXPNotify::Config
