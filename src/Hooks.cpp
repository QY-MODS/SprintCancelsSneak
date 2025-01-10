#include "Hooks.h"

RE::ButtonEvent* Hooks::CreateSneakEvent(const RE::INPUT_DEVICE a_device) {
	const auto control_map = RE::ControlMap::GetSingleton();
	const auto user_events = RE::UserEvents::GetSingleton();
	const auto key = control_map->GetMappedKey(user_events->sneak,a_device);
	auto sneak_event = RE::ButtonEvent::Create(a_device,user_events->sneak,key,1.f,0.f);
	if (!sneak_event) logger::error("Failed to create sneak_event");
	return sneak_event;
}

void Hooks::UpdateSneakEvents()
{
	const auto control_map = RE::ControlMap::GetSingleton();
	const auto user_events = RE::UserEvents::GetSingleton();
	for (auto& [device, event] : sneak_events) {
		event->idCode = control_map->GetMappedKey(user_events->sneak,device);
	}
}

void Hooks::Install()
{
	PlayerInputHandlerHook<RE::SprintHandler>::InstallHook(RE::VTABLE_SprintHandler[0]);
	ControlsChangedHook::InstallHook(RE::VTABLE_Journal_SystemTab[1]);
}

void Hooks::PlayerInputHandlerHook<RE::SneakHandler>::ProcessButton_Hook(RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data) {

    ProcessButton(this, a_event, a_data);
};

void Hooks::PlayerInputHandlerHook<RE::SprintHandler>::ProcessButton_Hook(RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
{
	if (!a_event) return ProcessButton(this, a_event, a_data);
	const auto player = RE::PlayerCharacter::GetSingleton();
	if (a_event->IsUp() || !player->IsMoving()) {
		sprint_held_start_time.reset();
		ProcessButton(this, a_event, a_data);
		return;
	}

	if (a_event->IsDown()) {
		sprint_held_start_time.emplace(std::chrono::high_resolution_clock::now());
	}
	if (!sprint_held_start_time.has_value()) {
		ProcessButton(this, a_event, a_data);
		return;
	}
	if (player->IsSneaking()) {
	    const auto end_time = std::chrono::high_resolution_clock::now();
	    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - sprint_held_start_time.value());
	    if (duration.count()>200 && !player->AsActorState()->IsSprinting()) {
		    logger::trace("Sending sneak event");
            if (const auto device = a_event->GetDevice(); sneak_events.contains(device)) {
				sprint_held_start_time.reset();
		        const auto player_controls = RE::PlayerControls::GetSingleton();
		        const auto sneak_event = sneak_events.at(device);
		        player_controls->sneakHandler->ProcessButton(sneak_event,a_data);
			    return this->ProcessButton_Hook(a_event,a_data);
		    }
	    }
	}
	ProcessButton(this, a_event, a_data);

}
template <typename Handler>
void Hooks::PlayerInputHandlerHook<Handler>::InstallHook(const REL::VariantID& varID) {
	REL::Relocation vTable(varID);
    ProcessButton = vTable.write_vfunc(0x4, &Hooks::PlayerInputHandlerHook<Handler>::ProcessButton_Hook);
}

RE::BSEventNotifyControl Hooks::ControlsChangedHook::ProcessEvent_Hook(const RE::BSGamerProfileEvent* a_event, RE::BSTEventSource<RE::BSGamerProfileEvent>* a_eventSource)
{
	logger::info("ControlsChangedHook::ProcessEvent_Hook");
	UpdateSneakEvents();
	return ProcessEvent(this, a_event, a_eventSource);
}

void Hooks::ControlsChangedHook::InstallHook(const REL::VariantID& varID)
{
	REL::Relocation vTable(varID);
	ProcessEvent = vTable.write_vfunc(0x1, &Hooks::ControlsChangedHook::ProcessEvent_Hook);
}
