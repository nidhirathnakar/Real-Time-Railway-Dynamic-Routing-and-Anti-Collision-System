#ifndef COLLISION_DETECTOR_H
#define COLLISION_DETECTOR_H

#include <string>
#include <vector>

#include "Train.h"

enum class SafetyEvent { SAFE, NEAR_MISS, COLLISION };

struct CollisionEvent {
    SafetyEvent type{SafetyEvent::SAFE};
    int firstTrain{-1};
    int secondTrain{-1};
    double separation{};
};

class CollisionDetector {
public:
    explicit CollisionDetector(double nearMissDistance = 5000.0, double collisionDistance = 1000.0);
    std::vector<CollisionEvent> inspect(const std::vector<TrainTelemetry>& telemetry) const;

private:
    double nearMissDistance;
    double collisionDistance;
};

#endif