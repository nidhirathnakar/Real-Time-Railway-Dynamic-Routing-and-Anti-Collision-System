#ifndef INTERLOCKING_CONTROLLER_H
#define INTERLOCKING_CONTROLLER_H

#include <string>
#include <vector>
#include <mutex>

#include "BlockManager.h"
#include "RailwayGraph.h"
#include "ReservationManager.h"

class InterlockingController {
public:
    InterlockingController(BlockManager& blocks, ReservationManager& reservations);
    bool authorize(int trainId, const std::vector<std::string>& blockIds);
    bool authorizeReplacementRoute(
        int trainId,
        const Route& candidateRoute,
        const std::string& currentNode,
        const std::string& destination
    );
    void releaseBlocks(int trainId, const std::vector<std::string>& blockIds);
    void release(int trainId, const std::vector<std::string>& blockIds);

private:
    BlockManager& blocks;
    ReservationManager& reservations;
    std::mutex transactionMutex;
};

#endif