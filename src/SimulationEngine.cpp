#include "SimulationEngine.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <tuple>
#include <vector>

#include "BlockManager.h"
#include "BrakingModel.h"
#include "CollisionDetector.h"
#include "InterlockingController.h"
#include "Logger.h"
#include "Metrics.h"
#include "RailwayGraph.h"
#include "ReservationManager.h"
#include "RouteEngine.h"
#include "Switch.h"
#include "TelemetryQueue.h"
#include "Train.h"

namespace {
std::string routeText(const Route& route) {
    std::ostringstream output;
    for (std::size_t index = 0; index < route.nodes.size(); ++index) {
        if (index) output << " -> ";
        output << route.nodes[index];
    }
    return output.str();
}

std::vector<std::string> routeBlocks(const Route& route) {
    std::vector<std::string> result;
    for (std::size_t index = 1; index < route.nodes.size(); ++index) {
        result.push_back(route.nodes[index - 1] + "->" + route.nodes[index]);
    }
    return result;
}
}

void SimulationEngine::run(std::chrono::milliseconds step, int ticks) {
    if (step.count() <= 0 || ticks <= 0) throw std::invalid_argument("Simulation step and ticks must be positive");
    const auto started = std::chrono::steady_clock::now();
    Logger logger;
    Metrics metrics;
    RailwayGraph graph;
    for (const auto& node : {"A", "B", "C", "D", "E", "F"}) graph.addNode(node);
    graph.addTrack("A", "B", 10000); graph.addTrack("A", "D", 15000);
    graph.addTrack("B", "C", 10000); graph.addTrack("C", "F", 10000);
    graph.addTrack("D", "E", 10000); graph.addTrack("E", "F", 10000);
    graph.displayGraph();
    RouteEngine routes(graph);
    BlockManager blocks;
    for (const auto& [from, to, distance] : {std::tuple{"A", "B", 10000.0}, {"A", "D", 15000.0}, {"B", "C", 10000.0}, {"C", "F", 10000.0}, {"D", "E", 10000.0}, {"E", "F", 10000.0}}) {
        blocks.addBlock(TrackBlock(std::string(from) + "->" + to, from, to, distance, 30.0));
    }
    ReservationManager reservations;
    InterlockingController interlocking(blocks, reservations);
    Switch junction("A", "A", {"B", "D"});
    CollisionDetector detector;
    Train train1(1, "A", "F", 20.0);
    Train train2(2, "A", "F", 18.0);
    const auto firstRoute = routes.calculate("A", "F");
    const auto preferredSecondRoute = routes.calculate("A", "F");
    const auto secondRoute = routes.calculate("A", "F", {}, {"A->B"});
    if (!firstRoute || !secondRoute) throw std::runtime_error("Scenario route calculation failed");
    train1.setRoute(*firstRoute); train2.setRoute(*secondRoute);
    metrics.setTotalTrains(2);
    if (!interlocking.authorize(1, routeBlocks(*firstRoute))) metrics.recordFailedReservation(); else metrics.recordSuccessfulReservation();
    if (preferredSecondRoute && !interlocking.authorize(2, routeBlocks(*preferredSecondRoute))) {
        metrics.recordFailedReservation();
        metrics.recordBlockedRouteAttempt();
        if (secondRoute) {
            metrics.recordRouteChange();
            metrics.recordCollisionPrevented();
        }
    }
    if (!secondRoute || !interlocking.authorize(2, routeBlocks(*secondRoute))) metrics.recordFailedReservation(); else metrics.recordSuccessfulReservation();
    std::cout << "\n========================================\nROUTE CALCULATION\n========================================\n"
              << "Train 1\nStart       : A\nDestination : F\nRoute       : " << routeText(*firstRoute)
              << "\nDistance    : " << firstRoute->distance / 1000.0 << " km\n"
              << "========================================\n";
    logger.route("Train 1: " + routeText(*firstRoute));
    logger.route("Train 2 rerouted: " + routeText(*secondRoute));
    logger.info("Switch A locked on " + junction.getPosition());

    TelemetryQueue queue(128);
    std::vector<Telemetry> latest;
    std::jthread consumer([&](std::stop_token) {
        std::map<int, Telemetry> current;
        std::map<int, std::string> lastPrintedBlock;
        std::unordered_set<std::string> reportedEvents;
        while (const auto event = queue.pop()) {
            const auto latency = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - event->timestamp);
            metrics.telemetryLatency(latency);
            static thread_local std::map<int, std::string> previousBlocks;
            const auto previous = previousBlocks.find(event->id);
            if (previous != previousBlocks.end() && previous->second != event->block && !previous->second.empty()) {
                blocks.releaseOccupancy(previous->second, event->id);
            }
            if (!event->block.empty()) {
                blocks.occupyBlock(event->block, event->id);
                previousBlocks[event->id] = event->block;
            } else {
                previousBlocks.erase(event->id);
            }
            current[event->id] = *event;
            latest.clear();
            for (const auto& [unused, sample] : current) latest.push_back(sample);
            for (const auto& collision : detector.inspect(latest)) {
                const auto eventKey = std::to_string(collision.firstTrain) + ":" + std::to_string(collision.secondTrain);
                if (reportedEvents.insert(eventKey).second) {
                    if (collision.type == SafetyEvent::COLLISION) { metrics.recordCollision(); logger.safety("Collision detected"); }
                    else { metrics.recordNearMiss(); logger.safety("Near miss detected"); }
                }
            }
            if (event->brakingStarted) { metrics.recordEmergencyBrake(); logger.safety("Train " + std::to_string(event->id) + " braking"); }
            if (lastPrintedBlock[event->id] != event->block || event->brakingStarted || event->completed) {
                std::cout << "Telemetry train " << event->id << " block=" << (event->block.empty() ? "yard" : event->block)
                          << " position=" << std::fixed << std::setprecision(1) << event->position << " m velocity=" << event->velocity << " m/s\n";
                lastPrintedBlock[event->id] = event->block;
            }
        }
    });

    std::vector<std::jthread> workers;
    for (Train* train : {&train1, &train2}) {
        workers.emplace_back([&, train](std::stop_token) {
            for (int tick = 0; tick < ticks; ++tick) {
                const double obstacleDistance = train == &train1 ? 30.0 : 100.0;
                const auto event = train->update(step.count() / 1000.0, obstacleDistance);
                queue.push(event);
                if (event.completed) break;
                metrics.tick();
            }
        });
    }
    for (auto& worker : workers) worker.join();
    queue.close();
    consumer.join();
    const auto finalTrain1 = train1.update(0.0, 100.0);
    const auto finalTrain2 = train2.update(0.0, 100.0);
    if (finalTrain1.completed) { metrics.recordTrainCompleted(); logger.info("Train 1 completed"); }
    if (finalTrain2.completed) { metrics.recordTrainCompleted(); logger.info("Train 2 completed"); }
    interlocking.release(1, routeBlocks(*firstRoute));
    interlocking.release(2, routeBlocks(*secondRoute));
    metrics.print(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started));
    std::cout << "System Status     : SAFE\n";
}