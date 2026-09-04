#ifndef TRACK_BLOCK_H
#define TRACK_BLOCK_H

#include <string>

enum class BlockState { FREE, RESERVED, OCCUPIED, BLOCKED, CLOSED };

class TrackBlock {
public:
    TrackBlock(std::string id, std::string start, std::string end,
               double length, double maximumSpeed);
    const std::string& getId() const;
    const std::string& getStart() const;
    const std::string& getEnd() const;
    double getLength() const;
    double getMaximumSpeed() const;
    BlockState getState() const;
    int getReservedTrain() const;
    int getOccupyingTrain() const;
    void setState(BlockState state);
    void setReservedTrain(int trainId);
    void setOccupyingTrain(int trainId);

private:
    std::string id;
    std::string start;
    std::string end;
    double length;
    double maximumSpeed;
    BlockState state{BlockState::FREE};
    int reservedTrain{-1};
    int occupyingTrain{-1};
};

#endif