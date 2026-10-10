#include "InterlockingController.h"

#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace {

std::vector<std::string> blocksFor(const Route& route) {
    std::vector<std::string> blocks;
    for (std::size_t index = 1; index < route.nodes.size(); ++index) {
        blocks.push_back(route.nodes[index - 1] + "->" + route.nodes[index]);
    }
    return blocks;
}

}

InterlockingController::InterlockingController(BlockManager& manager, ReservationManager& reservationManager)
    : blocks(manager), reservations(reservationManager) {
}

void InterlockingController::registerSwitch(Switch& railwaySwitch) {
    std::lock_guard transactionLock(transactionMutex);
    switches[railwaySwitch.getIncoming()] = &railwaySwitch;
}

bool InterlockingController::switchesAcceptRouteLocked(const Route& route) const {
    for (std::size_t index = 1; index < route.nodes.size(); ++index) {
        const auto switchIt = switches.find(route.nodes[index - 1]);
        if (switchIt != switches.end() && !switchIt->second->accepts(route.nodes[index])) return false;
    }
    return true;
}

bool InterlockingController::authorizeBlocksLocked(int trainId, const std::vector<std::string>& blockIds) {
    for (const auto& block : blockIds) {
        if (!blocks.isAvailable(block, trainId)) return false;
    }
    const auto reservationSnapshot = reservations.snapshot();
    if (!reservations.tryReserve(trainId, blockIds)) return false;
    std::vector<std::string> newReservations;
    for (const auto& block : blockIds) {
        if (!reservationSnapshot.contains(block) &&
            std::find(newReservations.begin(), newReservations.end(), block) == newReservations.end()) {
            newReservations.push_back(block);
        }
    }
    std::vector<std::string> newBlockReservations;
    for (const auto& block : blockIds) {
        try {
            if (!blocks.reserveBlock(block, trainId)) {
                reservations.release(trainId, newReservations);
                for (const auto& reservedBlock : newBlockReservations) blocks.releaseBlock(reservedBlock, trainId);
                return false;
            }
            if (!reservationSnapshot.contains(block) &&
                std::find(newBlockReservations.begin(), newBlockReservations.end(), block) == newBlockReservations.end()) {
                newBlockReservations.push_back(block);
            }
        } catch (const std::out_of_range&) {
            reservations.release(trainId, newReservations);
            for (const auto& reservedBlock : newBlockReservations) blocks.releaseBlock(reservedBlock, trainId);
            return false;
        }
    }
    return true;
}

bool InterlockingController::authorize(int trainId, const std::vector<std::string>& blockIds) {
    std::lock_guard transactionLock(transactionMutex);
    return authorizeBlocksLocked(trainId, blockIds);
}

bool InterlockingController::authorizeRoute(int trainId, const Route& route) {
    std::lock_guard transactionLock(transactionMutex);
    if (route.nodes.empty() || route.edgeDistances.size() + 1 != route.nodes.size() ||
        !switchesAcceptRouteLocked(route)) return false;
    return authorizeBlocksLocked(trainId, blocksFor(route));
}

bool InterlockingController::authorizeReplacementRoute(
    int trainId,
    const Route& candidateRoute,
    const std::string& currentNode,
    const std::string& destination
) {
    std::lock_guard transactionLock(transactionMutex);
    if (candidateRoute.nodes.empty() || candidateRoute.edgeDistances.size() + 1 != candidateRoute.nodes.size() ||
        candidateRoute.nodes.front() != currentNode || candidateRoute.nodes.back() != destination) {
        return false;
    }
    if (!switchesAcceptRouteLocked(candidateRoute)) return false;

    const auto candidateBlocks = blocksFor(candidateRoute);
    const auto reservationSnapshot = reservations.snapshot();
    std::vector<std::string> newReservations;
    std::vector<std::string> newBlockReservations;
    for (const auto& blockId : candidateBlocks) {
        try {
            if (!blocks.isAvailable(blockId, trainId)) return false;
        } catch (const std::out_of_range&) {
            return false;
        }
    }

    if (!reservations.tryReserve(trainId, candidateBlocks)) return false;
    for (const auto& blockId : candidateBlocks) {
        if (!reservationSnapshot.contains(blockId) &&
            std::find(newReservations.begin(), newReservations.end(), blockId) == newReservations.end()) {
            newReservations.push_back(blockId);
        }
    }
    for (const auto& blockId : candidateBlocks) {
        try {
            if (!blocks.reserveBlock(blockId, trainId)) {
                reservations.release(trainId, newReservations);
                for (const auto& reservedBlock : newBlockReservations) blocks.releaseBlock(reservedBlock, trainId);
                return false;
            }
            if (blocks.getReservedTrain(blockId) == trainId && !blocks.isOccupied(blockId) &&
                !reservationSnapshot.contains(blockId) &&
                std::find(newBlockReservations.begin(), newBlockReservations.end(), blockId) == newBlockReservations.end()) {
                newBlockReservations.push_back(blockId);
            }
        } catch (const std::out_of_range&) {
            reservations.release(trainId, newReservations);
            for (const auto& reservedBlock : newBlockReservations) blocks.releaseBlock(reservedBlock, trainId);
            return false;
        }
    }
    return true;
}

void InterlockingController::releaseBlocks(int trainId, const std::vector<std::string>& blockIds) {
    std::lock_guard transactionLock(transactionMutex);
    for (const auto& blockId : blockIds) {
        try {
            if (blocks.isOccupied(blockId) && blocks.getReservedTrain(blockId) == trainId) {
                blocks.releaseOccupancy(blockId, trainId);
            } else if (blocks.isReserved(blockId) && blocks.getReservedTrain(blockId) == trainId) {
                blocks.releaseBlock(blockId, trainId);
            }
        } catch (const std::out_of_range&) {
            continue;
        }
    }
    reservations.release(trainId, blockIds);
}


void InterlockingController::release(
    int trainId,
    const std::vector<std::string>& blockIds
) {
    std::lock_guard transactionLock(transactionMutex);

    for (const auto& block : blockIds) {
        try {
            if (blocks.isOccupied(block) &&
                blocks.getReservedTrain(block) == trainId) {
                blocks.releaseOccupancy(block, trainId);
            } else if (blocks.isReserved(block) &&
                       blocks.getReservedTrain(block) == trainId) {
                blocks.releaseBlock(block, trainId);
            }
        } catch (const std::out_of_range&) {
            continue;
        }
    }

    reservations.release(trainId, blockIds);
}
