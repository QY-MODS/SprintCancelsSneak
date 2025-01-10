#pragma once

namespace Hooks {

	void Install();

	inline std::map<RE::INPUT_DEVICE, RE::ButtonEvent*> sneak_events;
	inline std::optional<std::chrono::high_resolution_clock::time_point> sprint_held_start_time;

	template <typename Handler>
    class PlayerInputHandlerHook : public Handler {
        using ProcessButton_t = decltype(&Handler::ProcessButton);
    public:
        static inline REL::Relocation<ProcessButton_t> ProcessButton;
        void ProcessButton_Hook(RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);
        static void InstallHook(const REL::VariantID& varID);
    };

    RE::ButtonEvent* CreateSneakEvent(RE::INPUT_DEVICE a_device);
    void UpdateSneakEvents();

    class ControlsChangedHook : public RE::Journal_SystemTab {
        using ProcessMessage_t = RE::BSEventNotifyControl(RE::Journal_SystemTab::*)(const RE::BSGamerProfileEvent*, RE::BSTEventSource<RE::BSGamerProfileEvent>*);
        static inline REL::Relocation<ProcessMessage_t> ProcessEvent;
        RE::BSEventNotifyControl ProcessEvent_Hook(const RE::BSGamerProfileEvent* a_event, RE::BSTEventSource<RE::BSGamerProfileEvent>* a_eventSource);
    public:
        static void InstallHook(const REL::VariantID& varID);
    };

};