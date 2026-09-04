#ifndef SWITCH_H
#define SWITCH_H

#include <shared_mutex>
#include <string>
#include <unordered_set>

class Switch {
public:
    Switch(std::string id, std::string incoming, std::unordered_set<std::string> outgoing);
    bool setPosition(const std::string& outgoing, bool protectedAreaOccupied);
    bool accepts(const std::string& outgoing) const;
    std::string getPosition() const;

private:
    std::string id;
    std::string incoming;
    std::unordered_set<std::string> outgoing;
    std::string position;
    mutable std::shared_mutex mutex;
};

#endif