#pragma once

namespace SkillXPNotify::CSFIntegration
{
    // Register a SKSE messaging listener for the "CustomSkills" sender.
    // Call once from SKSEPlugin_Load. If Custom Skills Framework
    // (https://github.com/Exit-9B/CustomSkills) is loaded in this
    // process, it dispatches a `kCustomSkillsInterface` message during
    // its own load; we receive the proxy pointer and stash it for
    // future per-XP integration work. If CSF isn't loaded, the message
    // never arrives and this whole module stays dormant — no static
    // link to CSF, no DLL dependency, no risk to users without CSF.
    void Register();
}
