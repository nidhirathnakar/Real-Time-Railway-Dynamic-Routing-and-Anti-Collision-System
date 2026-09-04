#include "Train.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

Train::Train(
    int id,
    const std::string& start,
    const std::string& destination,
    double velocity,
    double reactionTime,
    double emergencyDeceleration
)
    : id(id),
      start(start),
      destination(destination),
    velocity(velocity),
    brakingModel(reactionTime, emergencyDeceleration) {
    if (id < 0 || velocity < 0.0) {
        throw std::invalid_argument("Train id and velocity must be non-negative");
    }
}

int Train::getId() const {
    return id;
}

void Train::setRoute(const Route& selectedRoute) {
    if (selectedRoute.nodes.empty()) {
        throw std::invalid_argument("A train route cannot be empty");
    }
    std::lock_guard lock(stateMutex);
    route = selectedRoute.nodes;
    edgeDistances = selectedRoute.edgeDistances;
    routeIndex = 0;
    positionOnEdge = 0.0;
}

double Train::stoppingDistance(double currentVelocity, double reactionTime, double brakingDeceleration) {
    if (currentVelocity <= 0.0) {
        return 0.0;
    }
    if (reactionTime < 0.0 || brakingDeceleration <= 0.0) {
        throw std::invalid_argument("Reaction time must be non-negative and braking must be positive");
    }
    return currentVelocity * reactionTime +
           (currentVelocity * currentVelocity) / (2.0 * brakingDeceleration);
}

TrainTelemetry Train::update(double deltaSeconds, double distanceToObstacle) {
    if (deltaSeconds < 0.0 || distanceToObstacle < 0.0) {
        throw std::invalid_argument("Time step and obstacle distance must be non-negative");
    }
    const double currentVelocity = velocity.load();
    {
        std::lock_guard lock(stateMutex);
        if (state == TrainState::ARRIVED) {
            return {id, destination, "", 0.0, 0.0, 0.0, false, false, true, TrainState::ARRIVED, 0.0,
                    std::chrono::steady_clock::now()};
        }
    }
    const double safeDistance = brakingModel.stoppingDistance(currentVelocity);
    const bool mustBrake = distanceToObstacle <= safeDistance;
    emergencyBraking.store(mustBrake);
    const bool brakingStarted = mustBrake && !previousBraking;
    previousBraking = mustBrake;

    double travelVelocity = currentVelocity;
    if (mustBrake) {
        travelVelocity = std::max(0.0, currentVelocity - brakingModel.deceleration() * deltaSeconds);
        velocity.store(travelVelocity);
    }

    std::lock_guard lock(stateMutex);
    double remaining = ((currentVelocity + travelVelocity) / 2.0) * deltaSeconds;
    while (routeIndex < edgeDistances.size() && remaining >= edgeDistances[routeIndex] - positionOnEdge) {
        remaining -= edgeDistances[routeIndex] - positionOnEdge;
        positionOnEdge = 0.0;
        ++routeIndex;
    }
    if (routeIndex < edgeDistances.size()) positionOnEdge += remaining;
    const bool completed = !route.empty() && routeIndex >= edgeDistances.size();
    if (completed) {
        velocity.store(0.0);
    }
    const std::size_t nodeIndex = std::min(routeIndex, route.empty() ? 0U : route.size() - 1);
    const std::string node = route.empty() ? start : route[nodeIndex];
    const std::string block = completed || route.empty() || nodeIndex + 1 >= route.size()
        ? ""
        : route[nodeIndex] + "->" + route[nodeIndex + 1];
    state = completed ? TrainState::ARRIVED : mustBrake ? TrainState::BRAKING : velocity.load() > 0.0 ? TrainState::MOVING : TrainState::STOPPED;
    return {id, node, block, positionOnEdge, velocity.load(), safeDistance, mustBrake, brakingStarted, completed, state, safeDistance,
            std::chrono::steady_clock::now()};
}

void Train::displayInfo() const {

    std::cout << "\nTrain ID       : " << id;
    std::cout << "\nStart          : " << start;
    std::cout << "\nDestination    : " << destination;
    std::cout << "\nVelocity       : "
              << velocity
              << " m/s\n";
}