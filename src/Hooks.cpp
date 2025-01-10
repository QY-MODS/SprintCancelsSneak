#include "Hooks.h"

bool Hooks::InputHook::ProcessInput(RE::InputEvent* event)
{
	if (auto button_event = event->AsButtonEvent()) {
		//logger::info("ButtonEvent: {}", button_event->userEvent.c_str());
	}
    return false;
}

RE::ButtonEvent* Hooks::CreateSneakEvent(const RE::INPUT_DEVICE a_device) {
	const auto control_map = RE::ControlMap::GetSingleton();
	const auto user_events = RE::UserEvents::GetSingleton();
	const auto key = control_map->GetMappedKey(user_events->sneak,a_device);
	auto sneak_event = RE::ButtonEvent::Create(a_device,user_events->sneak,key,1.f,0.f);
	if (!sneak_event) logger::error("Failed to create sneak_event");
	return sneak_event;
}

RE::ButtonEvent* Hooks::CreateSprintEvent(const RE::INPUT_DEVICE a_device) {
	const auto control_map = RE::ControlMap::GetSingleton();
	const auto user_events = RE::UserEvents::GetSingleton();
	const auto key = control_map->GetMappedKey(user_events->sprint, a_device);
	auto sprint_event = RE::ButtonEvent::Create(a_device, user_events->sprint, key, 1.f, 0.1f);
	if (!sprint_event) logger::error("Failed to create sprint_event");
	return sprint_event;
}

void Hooks::MovementHandlerHook::InstallHook(const REL::VariantID& varID) {
	REL::Relocation vTable(varID);
    ProcessButton = vTable.write_vfunc(0x4, &Hooks::PlayerInputHandlerHook<RE::MovementHandler>::ProcessButton_Hook);
	ProcessThumbstick = vTable.write_vfunc(0x2, &Hooks::MovementHandlerHook::ProcessThumbstick_Hook);
}

void Hooks::MovementHandlerHook::SetIsMoving(const bool is_moving)
{
	if (GetIsMoving() == is_moving) return;

	std::unique_lock lock(move_mutex);
	if (is_moving) {
	    move_start_time.emplace(std::chrono::high_resolution_clock::now());
	}
	else {
		move_start_time.reset();
	}
}

bool Hooks::MovementHandlerHook::GetIsMoving()
{
	return GetMoveDuration() > 0.f;
}

float Hooks::MovementHandlerHook::GetMoveDuration()
{
	std::shared_lock lock(move_mutex);
	if (move_start_time.has_value()) {
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - move_start_time.value());
		return duration.count() / 1000.f;
	}
	return 0.f;
}

void Hooks::Install()
{
    auto& trampoline = SKSE::GetTrampoline();
    constexpr size_t size_per_hook = 14;
	trampoline.create(size_per_hook*1);

    const REL::Relocation target3{REL::RelocationID(67315, 68617)};
    InputHook::func = trampoline.write_call<5>(target3.address() + 0x7B, InputHook::thunk);

	static std::string classic_sprinting_redone_path = "Data/SKSE/Plugins/ClassicSprintingRedone.dll";
	is_classic_sprinting_redone_installed = std::filesystem::exists(classic_sprinting_redone_path);
	logger::info("ClassicSprintingRedone installed: {}", is_classic_sprinting_redone_installed);

	//PlayerInputHandlerHook<RE::SneakHandler>::InstallHook(RE::VTABLE_SneakHandler[0]);
	PlayerInputHandlerHook<RE::SprintHandler>::InstallHook(RE::VTABLE_SprintHandler[0]);
	MovementHandlerHook::InstallHook();
}

void Hooks::InputHook::thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event) {

	if (!a_dispatcher || !a_event) {
		return func(a_dispatcher, a_event);
	}

    auto first = *a_event;
    auto last = *a_event;
    size_t length = 0;

    for (auto current = *a_event; current; current = current->next) {
		ProcessInput(current);
    }
    func(a_dispatcher, a_event);
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
		logger::trace("Sprint held duration: {}", duration.count());
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

template <>
void Hooks::PlayerInputHandlerHook<RE::MovementHandler>::ProcessButton_Hook(RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data) {

	MovementHandlerHook::SetIsMoving(a_event->IsDown() || !a_event->IsUp());
	ProcessButton(this, a_event, a_data);
}

void Hooks::MovementHandlerHook::ProcessThumbstick_Hook(RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data) {
	SetIsMoving(fabs(a_event->xValue) > 0.f || fabs(a_event->yValue) > 0.f);
    Hooks::MovementHandlerHook::ProcessThumbstick(this, a_event, a_data);
}

template <typename Handler>
void Hooks::PlayerInputHandlerHook<Handler>::InstallHook(const REL::VariantID& varID) {
	REL::Relocation vTable(varID);
    ProcessButton = vTable.write_vfunc(0x4, &Hooks::PlayerInputHandlerHook<Handler>::ProcessButton_Hook);
}

RE::BSEventNotifyControl AnimationEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
{
	//logger::trace("AnimationEventSink::ProcessEvent: {}", a_event->tag);

   // if (a_event) {
   //     if (const RE::BSFixedString& eventTag = a_event->tag;
   //         eventTag == "FootLeft" || eventTag == "FootRight") {
   //         logger::trace("AnimationEventSink::ProcessEvent:");
   //         logger::trace("Removing event sink");
			//RE::PlayerCharacter::GetSingleton()->RemoveAnimationGraphEventSink(this);
   //     }
   // }
    return RE::BSEventNotifyControl::kContinue;
}