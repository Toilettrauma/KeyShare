#include "Logger.hpp"
#include <iostream>

void Logger::log_info(std::string_view msg) {
	std::cerr << "[INFO] " << msg << std::endl;
}

void Logger::log_error(std::string_view msg) {
	std::cerr << "[ERROR] " << msg << std::endl;
}
