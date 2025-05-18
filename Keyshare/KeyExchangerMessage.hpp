#pragma once
#include "KeyExchangerTypes.hpp"

namespace detail {
	union KeyExchangeEvent {
		KeyExchangeEvent();
		KeyExchangeEvent(const KeyboardMessage& keyboad_message);
		KeyExchangeEvent(const MouseMessage& mouse_message);

		KeyboardMessage keyboard;
		MouseMessage mouse;
	};
};

class KeyExchangeMessage {
public:
	static constexpr int magic = 0xFFAAFFAA;
	static constexpr size_t max_key_count = static_cast<size_t>(KeyboardKeyCode::KeyboardKeyCode_MAX);

	enum class EventType {
		Unknown = 0,
		Keyboard = 1,
		Mouse = 2
	};

	KeyExchangeMessage() noexcept;
	KeyExchangeMessage(const KeyboardMessage& keyboard_message) noexcept;
	KeyExchangeMessage(const MouseMessage& mouse_message) noexcept;

	KeyExchangeMessage& operator=(const KeyExchangeMessage& message) noexcept;

	EventType get_event_type() const noexcept;

	KeyboardMessage get_keyboard_message() const;
	MouseMessage get_mouse_message() const;

private:
	const int _magic;
	EventType _event_type;
	detail::KeyExchangeEvent _event;
};

