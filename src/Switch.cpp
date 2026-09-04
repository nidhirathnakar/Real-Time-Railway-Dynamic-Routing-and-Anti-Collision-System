#include "Switch.h"

#include <mutex>
#include <stdexcept>
#include <utility>

Switch::Switch(std::string switchId, std::string switchIncoming, std::unordered_set<std::string> switchOutgoing)
    : id(std::move(switchId)), incoming(std::move(switchIncoming)), outgoing(std::move(switchOutgoing)) {
    if (id.empty() || incoming.empty() || outgoing.empty()) throw std::invalid_argument("Invalid switch configuration");
    position = *outgoing.begin();
}

bool Switch::setPosition(const std::string& newPosition, bool protectedAreaOccupied) {
    std::unique_lock lock(mutex);
    if (protectedAreaOccupied || !outgoing.contains(newPosition)) return false;
    position = newPosition;
    return true;
}

bool Switch::accepts(const std::string& requested) const {
    std::shared_lock lock(mutex);
    return position == requested;
}

std::string Switch::getPosition() const {
    std::shared_lock lock(mutex);
    return position;
}