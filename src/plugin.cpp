#include "Hooks.h"
#include "logger.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start

        Hooks::player_animsink = new AnimationEventSink();

        Hooks::sneak_events[RE::INPUT_DEVICE::kKeyboard] = Hooks::CreateSneakEvent(RE::INPUT_DEVICE::kKeyboard);
        Hooks::sneak_events[RE::INPUT_DEVICE::kMouse] = Hooks::CreateSneakEvent(RE::INPUT_DEVICE::kMouse);
        Hooks::sneak_events[RE::INPUT_DEVICE::kGamepad] = Hooks::CreateSneakEvent(RE::INPUT_DEVICE::kGamepad);
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
        if (RE::PlayerCharacter::GetSingleton()->AddAnimationGraphEventSink(Hooks::player_animsink)) {
			logger::info("AnimationGraphEventSink added to PlayerCharacter");
		}
		/*else {
			logger::error("Failed to add AnimationGraphEventSink to PlayerCharacter");
        }*/
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