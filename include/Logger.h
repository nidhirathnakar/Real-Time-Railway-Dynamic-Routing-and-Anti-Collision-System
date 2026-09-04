#ifndef LOGGER_H
#define LOGGER_H

#include <mutex>
#include <string>

class Logger {
public:
    void info(const std::string& message);
    void route(const std::string& message);
    void warning(const std::string& message);
    void safety(const std::string& message);

private:
    void write(const char* level, const std::string& message);
    std::mutex mutex;
};

#endif