#include "ReservationManager.h"

#include <mutex>

bool ReservationManager::tryReserve(int trainId, const std::vector<std::string>& blocks) {
    std::unique_lock lock(mutex);
    for (const auto& block : blocks) {
        const auto owner = owners.find(block);
        if (owner != owners.end() && owner->second != trainId) {
            return false;
        }
    }
    for (const auto& block : blocks) {
        owners[block] = trainId;
    }
    return true;
}

void ReservationManager::release(int trainId) {
    std::unique_lock lock(mutex);
    for (auto iterator = owners.begin(); iterator != owners.end();) {
        if (iterator->second == trainId) {
            iterator = owners.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

std::unordered_map<std::string, int> ReservationManager::snapshot() const {
    std::shared_lock lock(mutex);
    return owners;
}