#ifndef TELEMETRY_QUEUE_H
#define TELEMETRY_QUEUE_H

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

#include "Telemetry.h"

class TelemetryQueue {
public:
    explicit TelemetryQueue(std::size_t capacity = 256);
    bool push(Telemetry event);
    std::optional<Telemetry> pop();
    void close();

private:
    const std::size_t capacity;
    std::queue<Telemetry> events;
    bool closed{false};
    std::mutex mutex;
    std::condition_variable ready;
    std::condition_variable space;
};

#endif