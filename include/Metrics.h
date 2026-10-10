#ifndef METRICS_H
#define METRICS_H

#include <atomic>
#include <chrono>
#include <iostream>

class Metrics {
public:
    void recordTrainCompleted() { ++completed; }
    void recordCollision() { ++collisions; }
    void recordCollisionPrevented() { ++collisionsPrevented; }
    void recordInterlockingConflictPrevented() { ++interlockingConflictsPrevented; }
    void recordNearMiss() { ++nearMisses; }
    void recordEmergencyBrake() { ++emergencyBrakes; }
    void recordRouteChange() { ++routeChanges; }
    void recordBlockedRouteAttempt() { ++blockedRoutes; }
    void recordSuccessfulReservation() { ++successfulReservations; }
    void recordFailedReservation() { ++failedReservations; }
    void tick() { ++ticks; }
    void telemetryLatency(std::chrono::microseconds latency);
    void setTotalTrains(int count) { totalTrains = count; }
    int collisionCount() const { return collisions.load(); }
    int collisionPreventedCount() const { return collisionsPrevented.load(); }
    int interlockingConflictPreventedCount() const { return interlockingConflictsPrevented.load(); }
    int nearMissCount() const { return nearMisses.load(); }
    int emergencyBrakeCount() const { return emergencyBrakes.load(); }
    void print(std::chrono::milliseconds duration) const;

private:
    std::atomic<int> totalTrains{0}, completed{0}, collisions{0}, collisionsPrevented{0};
    std::atomic<int> interlockingConflictsPrevented{0};
    std::atomic<int> nearMisses{0}, emergencyBrakes{0}, routeChanges{0}, blockedRoutes{0};
    std::atomic<int> successfulReservations{0}, failedReservations{0}, ticks{0}, telemetryEvents{0};
    std::atomic<long long> totalLatency{0}, maximumLatency{0};
};

#endif