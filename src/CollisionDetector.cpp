#include "CollisionDetector.h"

#include <cmath>
#include <stdexcept>

CollisionDetector::CollisionDetector(double nearMiss, double collision)
    : nearMissDistance(nearMiss), collisionDistance(collision) {
    if (collision < 0.0 || nearMiss < collision) throw std::invalid_argument("Invalid collision thresholds");
}

std::vector<CollisionEvent> CollisionDetector::inspect(const std::vector<TrainTelemetry>& samples) const {
    std::vector<CollisionEvent> events;
    for (std::size_t first = 0; first < samples.size(); ++first) {
        for (std::size_t second = first + 1; second < samples.size(); ++second) {
            if (samples[first].block.empty() || samples[first].block != samples[second].block) continue;
            const double separation = std::abs(samples[first].position - samples[second].position);
            if (separation <= nearMissDistance) {
                events.push_back({separation <= collisionDistance ? SafetyEvent::COLLISION : SafetyEvent::NEAR_MISS,
                                  samples[first].id, samples[second].id, separation});
            }
        }
    }
    return events;
}