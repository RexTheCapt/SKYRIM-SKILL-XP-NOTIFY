#pragma once

namespace SkillXPNotify::Notify
{
    // Post a formatted "+<delta> <SkillName> -> <pct>%" corner notification
    // for the given player-skill ActorValue. Bounces through the SKSE task
    // interface so the actual `RE::SendHUDMessage::ShowHUDMessage` call lands
    // on the game's main thread (UI calls from arbitrary threads can crash
    // the engine). No-op if `a_skill` isn't one of the 18 player skills.
    void Dispatch(RE::ActorValue a_skill, float a_delta, float a_pct);
}
