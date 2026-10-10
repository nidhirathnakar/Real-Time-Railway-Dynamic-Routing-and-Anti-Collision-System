#ifndef RESERVATION_MANAGER_H
#define RESERVATION_MANAGER_H

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class ReservationManager {
public:
    bool tryReserve(int trainId, const std::vector<std::string>& blocks);
    void release(int trainId, const std::vector<std::string>& blocks);
    void release(int trainId);
    std::unordered_map<std::string, int> snapshot() const;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, int> owners;
};

#endif