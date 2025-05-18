#include "KeyEventHandler.hpp"

const HHOOK invalid_hook_value = static_cast<HHOOK>(INVALID_HANDLE_VALUE);
const HWND message_window = reinterpret_cast<HWND>(-1);

KeyEventHandler::HandlerFunction KeyEventHandler::_handler;
HHOOK KeyEventHandler::_keyboard_hook = invalid_hook_value;
HHOOK KeyEventHandler::_mouse_hook = invalid_hook_value;

void KeyEventHandler::enable_keyboard_hook() {
	if (_keyboard_hook == invalid_hook_value) {
		_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyEventHandler::keyboard_hook_procedure, nullptr, 0);
	}
}

void KeyEventHandler::enable_mouse_hook() {
	if (_mouse_hook == invalid_hook_value) {
		_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, &KeyEventHandler::mouse_hook_procedure, nullptr, 0);
	}
}

void KeyEventHandler::disable_keyboard_hook() {
	if (_keyboard_hook != invalid_hook_value) {
		UnhookWindowsHookEx(_keyboard_hook);
		_keyboard_hook = invalid_hook_value;
	}
}

void KeyEventHandler::disable_mouse_hook() {
	if (_mouse_hook != invalid_hook_value) {
		UnhookWindowsHookEx(_mouse_hook);
		_mouse_hook = invalid_hook_value;
	}
}

int KeyEventHandler::process() {
	MSG msg;
	return GetMessage(&msg, message_window, 0, 0);
}


void KeyEventHandler::set_handler(const HandlerFunction& handler) {
	_handler = handler;
}

LRESULT KeyEventHandler::keyboard_hook_procedure(int code, WPARAM w_param, LPARAM l_param) {
	if (code < 0 || !_handler) {
		return CallNextHookEx(_keyboard_hook, code, w_param, l_param);
	}

	KeyboardMessage message;
	switch (w_param) {
	case WM_KEYDOWN:
		message.key_state = KeyboardKeyState::KeyDown;
		break;
	case WM_KEYUP:
		message.key_state = KeyboardKeyState::KeyUp;
		break;
	default:
		message.key_state = KeyboardKeyState::Unknown;
		break;
	}

	LPKBDLLHOOKSTRUCT keyboard_hook = reinterpret_cast<LPKBDLLHOOKSTRUCT>(l_param);
	message.key_code = static_cast<KeyboardKeyCode>(keyboard_hook->vkCode);
	if (keyboard_hook->flags & LLKHF_EXTENDED) {
		message.modifiers[KeyboardModifiers::extended] = true;
	}
	if (keyboard_hook->flags & LLKHF_ALTDOWN) {
		message.modifiers[KeyboardModifiers::alt] = true;
	}

	_handler(KeyExchangeMessage(message));

	return CallNextHookEx(_keyboard_hook, code, w_param, l_param);
}
LRESULT KeyEventHandler::mouse_hook_procedure(int code, WPARAM w_param, LPARAM l_param) {
	if (code < 0 || !_handler) {
		return CallNextHookEx(_mouse_hook, code, w_param, l_param);
	}

	MouseMessage message;

	LPMSLLHOOKSTRUCT mouse_hook = reinterpret_cast<LPMSLLHOOKSTRUCT>(l_param);
	switch (w_param) {
	case WM_LBUTTONDOWN:
		message.event = MouseEvent::ButtonDown;
		message.button = MouseButtonCode::LeftButton;
		break;
	case WM_LBUTTONUP:
		message.event = MouseEvent::ButtonUp;
		message.button = MouseButtonCode::LeftButton;
		break;
	case WM_RBUTTONDOWN:
		message.event = MouseEvent::ButtonUp;
		message.button = MouseButtonCode::RightButton;
		break;
	case WM_RBUTTONUP:
		message.event = MouseEvent::ButtonUp;
		message.button = MouseButtonCode::RightButton;
		break;
	case WM_MOUSEMOVE:
		message.event = MouseEvent::MouseMove;
		message.pos.x = mouse_hook->pt.x;
		message.pos.y = mouse_hook->pt.y;
		break;
	case WM_MOUSEWHEEL:
		message.event = MouseEvent::MouseWheel;
		message.wheel_delta = HIWORD(mouse_hook->mouseData);
		break;
	default:
		break;
	}

	_handler(KeyExchangeMessage(message));

	return CallNextHookEx(_mouse_hook, code, w_param, l_param);
}
