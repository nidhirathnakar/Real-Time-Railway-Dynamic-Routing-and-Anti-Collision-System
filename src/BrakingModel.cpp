#include "BrakingModel.h"

#include <stdexcept>

BrakingModel::BrakingModel(double reaction, double deceleration)
    : reactionTime(reaction), brakingDeceleration(deceleration) {
    if (reaction < 0.0 || deceleration <= 0.0) throw std::invalid_argument("Invalid braking model");
}

double BrakingModel::stoppingDistance(double velocity) const {
    if (velocity < 0.0) throw std::invalid_argument("Velocity cannot be negative");
    return velocity * reactionTime + velocity * velocity / (2.0 * brakingDeceleration);
}
bool BrakingModel::requiresBraking(double velocity, double distance) const {
    return distance <= stoppingDistance(velocity);
}
double BrakingModel::deceleration() const { return brakingDeceleration; }