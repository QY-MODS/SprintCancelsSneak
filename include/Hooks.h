#pragma once
#include <shared_mutex>


struct AnimationEventSink final : RE::BSTEventSink<RE::BSAnimationGraphEvent> {
	AnimationEventSink() = default;
    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;

};


namespace Hooks {

	void Install();

	inline std::atomic<bool> block_sprint_inputs = false;

    inline bool is_classic_sprinting_redone_installed = false;
    inline AnimationEventSink* player_animsink = nullptr;

	inline std::map<RE::INPUT_DEVICE, RE::ButtonEvent*> sneak_events;
	inline std::optional<std::chrono::high_resolution_clock::time_point> move_start_time;
	inline std::optional<std::chrono::high_resolution_clock::time_point> sprint_held_start_time;
	inline std::shared_mutex move_mutex;

	struct InputHook {
		static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event);
		static inline REL::Relocation<decltype(thunk)> func;

		static bool ProcessInput(RE::InputEvent* event);
	};

	template <typename Handler>
    class PlayerInputHandlerHook : public Handler {
        using ProcessButton_t = decltype(&Handler::ProcessButton);
    public:
        static inline REL::Relocation<ProcessButton_t> ProcessButton;
        void ProcessButton_Hook(RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);
        static void InstallHook(const REL::VariantID& varID);
    };

    RE::ButtonEvent* CreateSneakEvent(RE::INPUT_DEVICE a_device);
    RE::ButtonEvent* CreateSprintEvent(RE::INPUT_DEVICE a_device);


	class MovementHandlerHook:
    public PlayerInputHandlerHook<RE::MovementHandler>
    {
		using ProcessThumbstick_t = decltype(&RE::MovementHandler::ProcessThumbstick);
		static inline REL::Relocation<ProcessThumbstick_t> ProcessThumbstick;
		void ProcessThumbstick_Hook(RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data);
	public:
		static void InstallHook(const REL::VariantID& varID=RE::VTABLE_MovementHandler[0]);
        static void SetIsMoving(bool is_moving=true);
		static bool GetIsMoving();
        static float GetMoveDuration();
	};
};