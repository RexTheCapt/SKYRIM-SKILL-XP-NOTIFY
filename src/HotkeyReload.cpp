#include "pch.h"

#include "HotkeyReload.h"

#include "Config.h"

#include <RE/B/BSInputDeviceManager.h>
#include <RE/B/ButtonEvent.h>
#include <RE/I/InputEvent.h>
#include <RE/S/SendHUDMessage.h>

namespace SkillXPNotify::HotkeyReload
{
    namespace
    {
        // BSTEventSink lives forever; using a function-local static
        // avoids static-init order surprises with the lib's RE singletons.
        class Sink final : public RE::BSTEventSink<RE::InputEvent*>
        {
        public:
            static Sink& Get()
            {
                static Sink s;
                return s;
            }

            RE::BSEventNotifyControl ProcessEvent(
                RE::InputEvent* const* a_events,
                RE::BSTEventSource<RE::InputEvent*>*) override
            {
                if (!a_events) {
                    return RE::BSEventNotifyControl::kContinue;
                }
                const auto target = Config::Get().reload_key_code.load();
                if (target == 0) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                // Walk the linked list of input events for this frame.
                for (auto* e = *a_events; e; e = e->next) {
                    if (e->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
                        continue;
                    }
                    const auto* btn = e->AsButtonEvent();
                    if (!btn || !btn->IsDown()) {
                        // Only fire on the rising edge of the press, not
                        // every held-down frame.
                        continue;
                    }
                    if (static_cast<int>(btn->GetIDCode()) != target) {
                        continue;
                    }

                    Config::Load();

                    if (auto* task = SKSE::GetTaskInterface()) {
                        task->AddTask([]() {
                            RE::SendHUDMessage::ShowHUDMessage(
                                "SkillXPNotify: config reloaded.",
                                /*soundToPlay=*/nullptr,
                                /*cancelIfAlreadyQueued=*/true);
                        });
                    }
                    break;
                }
                return RE::BSEventNotifyControl::kContinue;
            }

        private:
            Sink()                       = default;
            ~Sink() override             = default;
            Sink(const Sink&)            = delete;
            Sink& operator=(const Sink&) = delete;
        };
    }  // namespace

    void Install()
    {
        auto* mgr = RE::BSInputDeviceManager::GetSingleton();
        if (!mgr) {
            SKSE::log::warn(
                "HotkeyReload: BSInputDeviceManager singleton unavailable, "
                "hotkey not bound.");
            return;
        }
        mgr->AddEventSink(&Sink::Get());
        SKSE::log::info(
            "HotkeyReload: bound to DI key 0x{:x} (0 = disabled).",
            Config::Get().reload_key_code.load());
    }
}  // namespace SkillXPNotify::HotkeyReload
