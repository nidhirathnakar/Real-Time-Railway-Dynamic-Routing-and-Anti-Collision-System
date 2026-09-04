#include "TelemetryQueue.h"

#include <stdexcept>

TelemetryQueue::TelemetryQueue(std::size_t queueCapacity) : capacity(queueCapacity) {
    if (capacity == 0) throw std::invalid_argument("Telemetry queue capacity must be positive");
}

bool TelemetryQueue::push(Telemetry event) {
    std::unique_lock lock(mutex);
    space.wait(lock, [&] { return closed || events.size() < capacity; });
    if (closed) return false;
    events.push(std::move(event));
    ready.notify_one();
    return true;
}

std::optional<Telemetry> TelemetryQueue::pop() {
    std::unique_lock lock(mutex);
    ready.wait(lock, [&] { return closed || !events.empty(); });
    if (events.empty()) return std::nullopt;
    Telemetry event = std::move(events.front());
    events.pop();
    space.notify_one();
    return event;
}

void TelemetryQueue::close() {
    std::lock_guard lock(mutex);
    closed = true;
    ready.notify_all();
    space.notify_all();
}