#include "Train.h"

#include "BlockManager.h"

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

std::string Train::getCurrentNode() const {
    std::lock_guard lock(stateMutex);
    if (route.empty()) return start;
    return route[std::min(routeIndex, route.size() - 1)];
}

std::string Train::getCurrentBlock() const {
    std::lock_guard lock(stateMutex);
    if (route.empty() || routeIndex + 1 >= route.size() || routeIndex >= edgeDistances.size()) return {};
    return route[routeIndex] + "->" + route[routeIndex + 1];
}

std::string Train::getNextBlock() const {
    std::lock_guard lock(stateMutex);
    const auto nextIndex = routeIndex + 1;
    if (route.empty() || nextIndex + 1 >= route.size() || nextIndex >= edgeDistances.size()) return {};
    return route[nextIndex] + "->" + route[nextIndex + 1];
}

std::string Train::getDestination() const {
    std::lock_guard lock(stateMutex);
    return destination;
}

double Train::getPositionOnEdge() const {
    std::lock_guard lock(stateMutex);
    return positionOnEdge;
}

std::size_t Train::getRouteIndex() const {
    std::lock_guard lock(stateMutex);
    return routeIndex;
}

std::vector<std::string> Train::getRemainingRoute() const {
    std::lock_guard lock(stateMutex);
    if (route.empty()) return {};
    const auto currentIndex = std::min(routeIndex, route.size() - 1);
    return {route.begin() + static_cast<std::ptrdiff_t>(currentIndex), route.end()};
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

void Train::replaceRemainingRoute(const Route& replacementRoute) {
    if (replacementRoute.nodes.empty() || replacementRoute.edgeDistances.size() + 1 != replacementRoute.nodes.size()) {
        throw std::invalid_argument("Replacement route must contain matching nodes and edge distances");
    }

    std::lock_guard lock(stateMutex);
    if (state == TrainState::ARRIVED) {
        throw std::logic_error("An arrived train cannot be rerouted");
    }

    const auto currentIndex = std::min(routeIndex, route.empty() ? 0U : route.size() - 1);
    const auto currentNode = route.empty() ? start : route[currentIndex];
    if (replacementRoute.nodes.front() != currentNode) {
        throw std::invalid_argument("Replacement route must start at the train's current node");
    }

    // A branch switch is supported at a node boundary. Mid-edge changes must
    // retain the current edge, which this graph cannot safely infer otherwise.
    if (positionOnEdge > 0.0 && !route.empty() && currentIndex + 1 < route.size() &&
        replacementRoute.nodes.size() < 2 ||
        positionOnEdge > 0.0 && !route.empty() && currentIndex + 1 < route.size() &&
        replacementRoute.nodes[1] != route[currentIndex + 1]) {
        throw std::invalid_argument("Mid-edge branch replacement is not supported");
    }

    route = replacementRoute.nodes;
    edgeDistances = replacementRoute.edgeDistances;
    routeIndex = 0;
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

bool Train::isNextBlockAvailable(const BlockManager& blocks) const {
    std::string nextBlock;
    {
        std::lock_guard lock(stateMutex);
        if (!route.empty() && routeIndex < edgeDistances.size() && routeIndex + 1 < route.size()) {
            nextBlock = route[routeIndex] + "->" + route[routeIndex + 1];
        }
    }
    return nextBlock.empty() || blocks.isAvailable(nextBlock, id);
}