#include "TrackBlock.h"

#include <stdexcept>
#include <utility>

TrackBlock::TrackBlock(std::string blockId, std::string blockStart, std::string blockEnd,
                       double blockLength, double blockMaximumSpeed)
    : id(std::move(blockId)), start(std::move(blockStart)), end(std::move(blockEnd)),
      length(blockLength), maximumSpeed(blockMaximumSpeed) {
    if (id.empty() || start.empty() || end.empty() || length <= 0.0 || maximumSpeed <= 0.0) {
        throw std::invalid_argument("Invalid track block configuration");
    }
}

const std::string& TrackBlock::getId() const { return id; }
const std::string& TrackBlock::getStart() const { return start; }
const std::string& TrackBlock::getEnd() const { return end; }
double TrackBlock::getLength() const { return length; }
double TrackBlock::getMaximumSpeed() const { return maximumSpeed; }
BlockState TrackBlock::getState() const { return state; }
int TrackBlock::getReservedTrain() const { return reservedTrain; }
int TrackBlock::getOccupyingTrain() const { return occupyingTrain; }
void TrackBlock::setState(BlockState newState) { state = newState; }
void TrackBlock::setReservedTrain(int trainId) { reservedTrain = trainId; }
void TrackBlock::setOccupyingTrain(int trainId) { occupyingTrain = trainId; }