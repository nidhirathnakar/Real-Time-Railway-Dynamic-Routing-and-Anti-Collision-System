#include "BlockManager.h"

#include <mutex>
#include <stdexcept>

void BlockManager::addBlock(TrackBlock newBlock) {
    std::unique_lock lock(mutex);
    blocks.insert_or_assign(newBlock.getId(), std::move(newBlock));
}

TrackBlock& BlockManager::block(const std::string& id) {
    const auto iterator = blocks.find(id);
    if (iterator == blocks.end()) throw std::out_of_range("Unknown block: " + id);
    return iterator->second;
}

const TrackBlock& BlockManager::block(const std::string& id) const {
    const auto iterator = blocks.find(id);
    if (iterator == blocks.end()) throw std::out_of_range("Unknown block: " + id);
    return iterator->second;
}

bool BlockManager::reserveBlock(const std::string& id, int trainId) {
    std::unique_lock lock(mutex);
    auto& selected = block(id);
    if (selected.getState() == BlockState::BLOCKED || selected.getState() == BlockState::CLOSED ||
        (selected.getReservedTrain() != -1 && selected.getReservedTrain() != trainId) ||
        (selected.getOccupyingTrain() != -1 && selected.getOccupyingTrain() != trainId)) return false;
    selected.setReservedTrain(trainId);
    if (selected.getState() == BlockState::FREE) selected.setState(BlockState::RESERVED);
    return true;
}

bool BlockManager::occupyBlock(const std::string& id, int trainId) {
    std::unique_lock lock(mutex);
    auto& selected = block(id);
    if (selected.getReservedTrain() != trainId || selected.getOccupyingTrain() != -1) return false;
    selected.setOccupyingTrain(trainId);
    selected.setState(BlockState::OCCUPIED);
    return true;
}

void BlockManager::releaseBlock(const std::string& id, int trainId) {
    std::unique_lock lock(mutex);
    auto& selected = block(id);
    if (selected.getReservedTrain() == trainId && selected.getOccupyingTrain() == -1) {
        selected.setReservedTrain(-1);
        selected.setState(BlockState::FREE);
    }
}

void BlockManager::releaseOccupancy(const std::string& id, int trainId) {
    std::unique_lock lock(mutex);
    auto& selected = block(id);
    if (selected.getOccupyingTrain() == trainId) {
        selected.setOccupyingTrain(-1);
        selected.setReservedTrain(-1);
        selected.setState(BlockState::FREE);
    }
}

bool BlockManager::isAvailable(const std::string& id, int trainId) const {
    std::shared_lock lock(mutex);
    const auto& selected = block(id);
    return selected.getState() != BlockState::BLOCKED && selected.getState() != BlockState::CLOSED &&
           (selected.getReservedTrain() == -1 || selected.getReservedTrain() == trainId) &&
           (selected.getOccupyingTrain() == -1 || selected.getOccupyingTrain() == trainId);
}
void BlockManager::setBlocked(const std::string& id, bool blocked) {
    std::unique_lock lock(mutex);
    auto& selected = block(id);
    if (blocked) {
        selected.setState(BlockState::BLOCKED);
        return;
    }
    selected.setState(selected.getOccupyingTrain() != -1 ? BlockState::OCCUPIED
                    : selected.getReservedTrain() != -1 ? BlockState::RESERVED
                    : BlockState::FREE);
}
bool BlockManager::isBlocked(const std::string& id) const {
    std::shared_lock lock(mutex);
    return block(id).getState() == BlockState::BLOCKED;
}
bool BlockManager::isOccupied(const std::string& id) const { std::shared_lock lock(mutex); return block(id).getState() == BlockState::OCCUPIED; }
bool BlockManager::isReserved(const std::string& id) const { std::shared_lock lock(mutex); return block(id).getReservedTrain() != -1; }
int BlockManager::getReservedTrain(const std::string& id) const { std::shared_lock lock(mutex); return block(id).getReservedTrain(); }
BlockState BlockManager::getBlockState(const std::string& id) const { std::shared_lock lock(mutex); return block(id).getState(); }

std::vector<std::string> BlockManager::occupiedBlocks() const {
    std::shared_lock lock(mutex);
    std::vector<std::string> result;
    for (const auto& [id, selected] : blocks) if (selected.getState() == BlockState::OCCUPIED) result.push_back(id);
    return result;
}