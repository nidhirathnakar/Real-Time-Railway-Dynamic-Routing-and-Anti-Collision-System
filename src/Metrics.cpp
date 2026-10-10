#include "Metrics.h"

void Metrics::telemetryLatency(std::chrono::microseconds latency) {
    ++telemetryEvents;
    totalLatency += latency.count();
    auto current = maximumLatency.load();
    while (current < latency.count() && !maximumLatency.compare_exchange_weak(current, latency.count())) {}
}

void Metrics::print(std::chrono::milliseconds duration) const {
    const auto events = telemetryEvents.load();
    const double average = events == 0 ? 0.0 : static_cast<double>(totalLatency.load()) / events / 1000.0;
    std::cout << "\n========================================\nSIMULATION METRICS\n========================================\n"
              << "Trains                 : " << totalTrains.load() << '\n'
              << "Completed              : " << completed.load() << '\n'
              << "Collisions             : " << collisions.load() << '\n'
              << "Collisions Prevented   : " << collisionsPrevented.load() << '\n'
              << "Interlocking Conflicts Prevented : " << interlockingConflictsPrevented.load() << '\n'
              << "Near Misses            : " << nearMisses.load() << '\n'
              << "Emergency Brakes       : " << emergencyBrakes.load() << '\n'
              << "Route Changes          : " << routeChanges.load() << '\n'
              << "Blocked Route Attempts  : " << blockedRoutes.load() << '\n'
              << "Successful Reservations : " << successfulReservations.load() << '\n'
              << "Failed Reservations     : " << failedReservations.load() << '\n'
              << "Average Telemetry Lat. : " << average << " ms\n"
              << "Max Telemetry Latency  : " << maximumLatency.load() / 1000.0 << " ms\n"
              << "Simulation Ticks       : " << ticks.load() << '\n'
              << "Simulation Duration    : " << duration.count() << " ms\n"
              << "========================================\n";
}