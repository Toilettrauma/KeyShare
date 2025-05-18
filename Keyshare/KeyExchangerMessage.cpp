#include "KeyExchangerMessage.hpp"
#include <cassert>

namespace detail {
	KeyExchangeEvent::KeyExchangeEvent() : keyboard() {

	}
	KeyExchangeEvent::KeyExchangeEvent(const KeyboardMessage& keyboad_message) : keyboard(keyboad_message) {

	}
	KeyExchangeEvent::KeyExchangeEvent(const MouseMessage& mouse_message) : mouse(mouse_message) {

	}
};

KeyExchangeMessage::KeyExchangeMessage() noexcept : _magic(magic), _event_type(EventType::Unknown), _event() {

}

KeyExchangeMessage::KeyExchangeMessage(const KeyboardMessage& keyboard_message) noexcept : _magic(magic), _event_type(EventType::Keyboard), _event(keyboard_message) {

}

KeyExchangeMessage::KeyExchangeMessage(const MouseMessage& mouse_message) noexcept : _magic(magic), _event_type(EventType::Mouse), _event(mouse_message) {

}

KeyExchangeMessage& KeyExchangeMessage::operator=(const KeyExchangeMessage& message) noexcept {
	assert(message._magic == KeyExchangeMessage::magic);
	_event_type = message._event_type;
	_event = message._event;
	return *this;
}

KeyExchangeMessage::EventType KeyExchangeMessage::get_event_type() const noexcept {
	return _event_type;
}

KeyboardMessage KeyExchangeMessage::get_keyboard_message() const {
	assert(_event_type == EventType::Keyboard);
	return _event.keyboard;
}

MouseMessage KeyExchangeMessage::get_mouse_message() const {
	assert(_event_type == EventType::Mouse);
	return _event.mouse;
}
