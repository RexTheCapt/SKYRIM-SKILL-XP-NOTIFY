#pragma once

namespace SkillXPNotify::HotkeyReload
{
    // Register a BSTEventSink<RE::InputEvent*> on the live
    // BSInputDeviceManager so a configurable key (default F11, DI 0x57)
    // re-runs Config::Load() and pops a corner-notification confirmation.
    //
    // Must be called after SKSE's kInputLoaded message has fired —
    // before that, BSInputDeviceManager isn't ready yet.
    void Install();
}
