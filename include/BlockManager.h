#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "TrackBlock.h"

class BlockManager {
public:
    void addBlock(TrackBlock block);
    bool reserveBlock(const std::string& blockId, int trainId);
    bool occupyBlock(const std::string& blockId, int trainId);
    void releaseBlock(const std::string& blockId, int trainId);
    void releaseOccupancy(const std::string& blockId, int trainId);
    bool isAvailable(const std::string& blockId, int trainId = -1) const;
    bool isOccupied(const std::string& blockId) const;
    bool isReserved(const std::string& blockId) const;
    int getReservedTrain(const std::string& blockId) const;
    BlockState getBlockState(const std::string& blockId) const;
    std::vector<std::string> occupiedBlocks() const;

private:
    TrackBlock& block(const std::string& blockId);
    const TrackBlock& block(const std::string& blockId) const;
    mutable std::shared_mutex mutex;
    std::unordered_map<std::string, TrackBlock> blocks;
};

#endif