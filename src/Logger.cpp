#include "Logger.h"

#include <iostream>

void Logger::write(const char* level, const std::string& message) {
    std::lock_guard lock(mutex);
    std::cout << '[' << level << "] " << message << '\n';
}
void Logger::info(const std::string& message) { write("INFO", message); }
void Logger::route(const std::string& message) { write("ROUTE", message); }
void Logger::warning(const std::string& message) { write("WARNING", message); }
void Logger::safety(const std::string& message) { write("SAFETY", message); }