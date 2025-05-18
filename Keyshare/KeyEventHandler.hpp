#pragma once
#include "KeyExchangerMessage.hpp"
#include <functional>
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

class KeyEventHandler {
public:
	using HandlerFunction = std::function<void(KeyExchangeMessage message)>;

	static void enable_keyboard_hook();
	static void enable_mouse_hook();
	static void disable_keyboard_hook();
	static void disable_mouse_hook();

	static int process();

	static void set_handler(const HandlerFunction& handler);
private:
	static LRESULT keyboard_hook_procedure(int code, WPARAM w_param, LPARAM l_param);
	static LRESULT mouse_hook_procedure(int code, WPARAM w_param, LPARAM l_param);

	static HandlerFunction _handler;
	static HHOOK _keyboard_hook;
	static HHOOK _mouse_hook;
};

