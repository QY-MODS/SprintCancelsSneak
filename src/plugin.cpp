#include "Hooks.h"
#include "logger.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start
        Hooks::sneak_events[RE::INPUT_DEVICE::kKeyboard] = Hooks::CreateSneakEvent(RE::INPUT_DEVICE::kKeyboard);
        Hooks::sneak_events[RE::INPUT_DEVICE::kMouse] = Hooks::CreateSneakEvent(RE::INPUT_DEVICE::kMouse);
        Hooks::sneak_events[RE::INPUT_DEVICE::kGamepad] = Hooks::CreateSneakEvent(RE::INPUT_DEVICE::kGamepad);

		Hooks::sprint_events[RE::INPUT_DEVICE::kKeyboard] = Hooks::CreateSprintEvent(RE::INPUT_DEVICE::kKeyboard);
		Hooks::sprint_events[RE::INPUT_DEVICE::kMouse] = Hooks::CreateSprintEvent(RE::INPUT_DEVICE::kMouse);
		Hooks::sprint_events[RE::INPUT_DEVICE::kGamepad] = Hooks::CreateSprintEvent(RE::INPUT_DEVICE::kGamepad);
    }

}
SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	Hooks::Install();
    return true;
}