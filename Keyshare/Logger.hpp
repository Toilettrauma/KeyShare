#pragma once
#include <string_view>

class Logger {
public:
	static void log_info(std::string_view msg);
	static void log_error(std::string_view msg);
};

