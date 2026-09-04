#include "InterlockingController.h"

InterlockingController::InterlockingController(BlockManager& manager, ReservationManager& reservationManager)
    : blocks(manager), reservations(reservationManager) {
}

bool InterlockingController::authorize(int trainId, const std::vector<std::string>& blockIds) {
    std::lock_guard transactionLock(transactionMutex);
    for (const auto& block : blockIds) {
        if (!blocks.isAvailable(block, trainId)) return false;
    }
    if (!reservations.tryReserve(trainId, blockIds)) return false;
    for (const auto& block : blockIds) {
        if (!blocks.reserveBlock(block, trainId)) {
            reservations.release(trainId);
            return false;
        }
    }
    return true;
}

void InterlockingController::release(int trainId, const std::vector<std::string>& blockIds) {
    for (const auto& block : blockIds) {
        if (blocks.isOccupied(block) && blocks.getReservedTrain(block) == trainId) {
            blocks.releaseOccupancy(block, trainId);
        } else if (blocks.isReserved(block) && blocks.getReservedTrain(block) == trainId) {
            blocks.releaseBlock(block, trainId);
        }
    }
    reservations.release(trainId);
}