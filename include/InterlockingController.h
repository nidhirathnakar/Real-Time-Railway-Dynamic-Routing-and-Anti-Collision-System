#ifndef INTERLOCKING_CONTROLLER_H
#define INTERLOCKING_CONTROLLER_H

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

#include "BlockManager.h"
#include "RailwayGraph.h"
#include "ReservationManager.h"
#include "Switch.h"

class InterlockingController {
public:
    InterlockingController(BlockManager& blocks, ReservationManager& reservations);
    void registerSwitch(Switch& railwaySwitch);
    bool authorize(int trainId, const std::vector<std::string>& blockIds);
    bool authorizeRoute(int trainId, const Route& route);
    bool authorizeReplacementRoute(
        int trainId,
        const Route& candidateRoute,
        const std::string& currentNode,
        const std::string& destination
    );
    void releaseBlocks(int trainId, const std::vector<std::string>& blockIds);
    void release(int trainId, const std::vector<std::string>& blockIds);

private:
    bool authorizeBlocksLocked(int trainId, const std::vector<std::string>& blockIds);
    bool switchesAcceptRouteLocked(const Route& route) const;
    BlockManager& blocks;
    ReservationManager& reservations;
    std::mutex transactionMutex;
    std::unordered_map<std::string, Switch*> switches;
};

#endif