#ifndef TRAIN_H
#define TRAIN_H

#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

#include "RailwayGraph.h"
#include "BrakingModel.h"

class BlockManager;

enum class TrainState { STOPPED, MOVING, BRAKING, ARRIVED };

struct TrainTelemetry {
    int id{};
    std::string node;
    std::string block;
    double position{};
    double velocity{};
    double stoppingDistance{};
    bool emergencyBraking{};
    bool brakingStarted{};
    bool completed{};
    TrainState state{TrainState::STOPPED};
    double movementAuthority{};
    std::chrono::steady_clock::time_point timestamp;
};

class Train {
public:
    Train(
        int id,
        const std::string& start,
        const std::string& destination,
        double velocity,
        double reactionTime = 1.0,
        double emergencyDeceleration = 8.0
    );

    int getId() const;
    void setRoute(const Route& route);
    void replaceRemainingRoute(const Route& route);
    std::string getCurrentNode() const;
    std::string getCurrentBlock() const;
    std::string getNextBlock() const;
    std::string getDestination() const;
    double getPositionOnEdge() const;
    std::size_t getRouteIndex() const;
    std::vector<std::string> getRemainingRoute() const;
    bool isNextBlockAvailable(const BlockManager& blocks) const;
    TrainTelemetry update(double deltaSeconds, double distanceToObstacle);
    static double stoppingDistance(double velocity, double reactionTime, double brakingDeceleration);
    void displayInfo() const;

private:
    int id;
    std::string start;
    std::string destination;
    std::atomic<double> velocity;
    std::atomic<bool> emergencyBraking{false};
    bool previousBraking{};
    mutable std::mutex stateMutex;
    std::vector<std::string> route;
    std::vector<double> edgeDistances;
    std::size_t routeIndex{};
    double positionOnEdge{};
    TrainState state{TrainState::STOPPED};
    BrakingModel brakingModel;
};

#endif