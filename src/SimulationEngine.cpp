#include "SimulationEngine.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_set>
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
#include "Scenario.h"
#include "Switch.h"
#include "TelemetryQueue.h"
#include "Train.h"

namespace {

std::string routeText(const Route& route) {
    std::ostringstream output;

    for (std::size_t index = 0; index < route.nodes.size(); ++index) {
        if (index > 0) {
            output << " -> ";
        }

        output << route.nodes[index];
    }

    return output.str();
}

std::vector<std::string> routeBlocks(const Route& route) {
    std::vector<std::string> result;

    for (std::size_t index = 1; index < route.nodes.size(); ++index) {
        result.push_back(
            route.nodes[index - 1] + "->" + route.nodes[index]
        );
    }

    return result;
}

} // namespace

int SimulationEngine::run(
    std::chrono::milliseconds step,
    int ticks
) {
    return run(defaultScenario(), step, ticks);
}

int SimulationEngine::run(
    const ScenarioConfig& scenario,
    std::chrono::milliseconds step,
    int ticks
) {
    if (step.count() <= 0 || ticks <= 0) {
        throw std::invalid_argument(
            "Simulation step and ticks must be positive"
        );
    }

    const auto started = std::chrono::steady_clock::now();

    Logger logger;
    Metrics metrics;
    RailwayGraph graph;

    // ---------------------------------------------------------
    // 1. Build the railway graph
    // ---------------------------------------------------------

    for (const auto& node : scenario.nodes) {
        graph.addNode(node);
    }

    for (const auto& track : scenario.tracks) {
        graph.addTrack(
            track.from,
            track.to,
            track.distance
        );
    }

    graph.displayGraph();

    RouteEngine routes(graph);
    BlockManager blocks;

    // ---------------------------------------------------------
    // 2. Create track blocks
    // ---------------------------------------------------------

    for (const auto& track : scenario.tracks) {
        blocks.addBlock(
            TrackBlock(
                track.from + "->" + track.to,
                track.from,
                track.to,
                track.distance,
                30.0
            )
        );
    }

    for (const auto& blockedBlock : scenario.blockedBlocks) {
        blocks.setBlocked(blockedBlock, true);
    }

    ReservationManager reservations;

    InterlockingController interlocking(
        blocks,
        reservations
    );

    // ---------------------------------------------------------
    // 3. Initialize the railway switch, if configured
    // ---------------------------------------------------------

    std::unique_ptr<Switch> junction;

    if (!scenario.switches.empty()) {
        const auto& switchConfig = scenario.switches.front();

        junction = std::make_unique<Switch>(
            switchConfig.id,
            switchConfig.incoming,
            std::unordered_set<std::string>(
                switchConfig.outgoing.begin(),
                switchConfig.outgoing.end()
            )
        );
    }

    CollisionDetector detector;

    if (scenario.trains.empty()) {
        throw std::runtime_error(
            "Scenario must contain at least one train"
        );
    }

    // ---------------------------------------------------------
    // 4. Create trains and calculate their initial routes
    // ---------------------------------------------------------

    std::vector<std::unique_ptr<Train>> trains;
    std::vector<Route> routesForTrains;

    for (const auto& config : scenario.trains) {
        auto train = std::make_unique<Train>(
            config.id,
            config.start,
            config.destination,
            config.velocity
        );

        const auto route = routes.calculate(
            config.start,
            config.destination
        );

        if (!route) {
            throw std::runtime_error(
                "Scenario route calculation failed"
            );
        }

        train->setRoute(*route);

        routesForTrains.push_back(*route);
        trains.push_back(std::move(train));
    }

    const auto firstRoute = routesForTrains.front();

    // ---------------------------------------------------------
    // 5. Reserve the first train's route
    // ---------------------------------------------------------

    if (!interlocking.authorize(
            trains.front()->getId(),
            routeBlocks(firstRoute))) {

        metrics.recordFailedReservation();

    } else {
        metrics.recordSuccessfulReservation();
    }

    // ---------------------------------------------------------
    // 6. Calculate an alternative route for the second train
    // ---------------------------------------------------------

    std::optional<Route> secondRoute;

    if (trains.size() > 1) {
        const auto preferredSecondRoute = routes.calculate(
            scenario.trains[1].start,
            scenario.trains[1].destination
        );

        secondRoute = routes.calculate(
            scenario.trains[1].start,
            scenario.trains[1].destination,
            {},
            {"A->B"}
        );

        if (preferredSecondRoute &&
            !interlocking.authorize(
                trains[1]->getId(),
                routeBlocks(*preferredSecondRoute))) {

            metrics.recordFailedReservation();
            metrics.recordBlockedRouteAttempt();
        }

        if (!secondRoute ||
            !interlocking.authorize(
                trains[1]->getId(),
                routeBlocks(*secondRoute))) {

            metrics.recordFailedReservation();

        } else {
            trains[1]->replaceRemainingRoute(*secondRoute);

            metrics.recordSuccessfulReservation();

            routesForTrains[1] = *secondRoute;
        }
    }

    // Reserve routes for any additional trains.
    for (std::size_t index = 2;
         index < trains.size();
         ++index) {

        if (!interlocking.authorize(
                trains[index]->getId(),
                routeBlocks(routesForTrains[index]))) {

            metrics.recordFailedReservation();

        } else {
            metrics.recordSuccessfulReservation();
        }
    }

    metrics.setTotalTrains(
        static_cast<int>(trains.size())
    );

    // ---------------------------------------------------------
    // 7. Display route calculation
    // ---------------------------------------------------------

    std::cout
        << "\n========================================\n"
        << "ROUTE CALCULATION\n"
        << "========================================\n"
        << "Train " << trains.front()->getId()
        << "\nStart       : "
        << scenario.trains.front().start
        << "\nDestination : "
        << trains.front()->getDestination()
        << "\nRoute       : "
        << routeText(firstRoute)
        << "\nDistance    : "
        << firstRoute.distance / 1000.0
        << " km\n"
        << "========================================\n";

    logger.route(
        "Train " +
        std::to_string(trains.front()->getId()) +
        ": " +
        routeText(firstRoute)
    );

    if (secondRoute) {
        logger.route(
            "Train " +
            std::to_string(trains[1]->getId()) +
            " rerouted: " +
            routeText(*secondRoute)
        );
    }

    if (junction) {
        logger.info(
            "Switch " +
            scenario.switches.front().id +
            " locked on " +
            junction->getPosition()
        );
    }

    // ---------------------------------------------------------
    // 8. Set up the telemetry queue and consumer
    // ---------------------------------------------------------

    TelemetryQueue queue(128);

    std::vector<Telemetry> latest;

    // Only the consumer thread modifies these completion IDs.
    // They are read after consumer.join(), so there is no
    // concurrent access to the set.
    std::unordered_set<int> completedTrainIds;

    std::jthread consumer([&](std::stop_token) {
        std::map<int, Telemetry> current;

        std::map<int, std::string> lastPrintedBlock;

        std::unordered_set<std::string> reportedEvents;

        std::map<int, std::string> previousBlocks;

        while (const auto event = queue.pop()) {
            const auto latency =
                std::chrono::duration_cast<
                    std::chrono::microseconds
                >(
                    std::chrono::steady_clock::now() -
                    event->timestamp
                );

            metrics.telemetryLatency(latency);

            // Release the previous block when the train changes blocks.
            const auto previous =
                previousBlocks.find(event->id);

            if (previous != previousBlocks.end() &&
                previous->second != event->block &&
                !previous->second.empty()) {

                blocks.releaseOccupancy(
                    previous->second,
                    event->id
                );
            }

            // Record occupancy of the current block.
            if (!event->block.empty()) {
                blocks.occupyBlock(
                    event->block,
                    event->id
                );

                previousBlocks[event->id] = event->block;

            } else {
                previousBlocks.erase(event->id);
            }

            current[event->id] = *event;

            // Remember completed trains. The metrics are updated
            // after the queue has been drained and the thread joined.
            if (event->completed) {
                completedTrainIds.insert(event->id);
            }

            // Build the latest telemetry snapshot for collision detection.
            latest.clear();

            for (const auto& [unused, sample] : current) {
                latest.push_back(sample);
            }

            // -------------------------------------------------
            // Collision and near-miss detection
            // -------------------------------------------------

            for (const auto& collision : detector.inspect(latest)) {
                const auto eventKey =
                    std::to_string(collision.firstTrain) +
                    ":" +
                    std::to_string(collision.secondTrain);

                if (reportedEvents.insert(eventKey).second) {
                    if (collision.type == SafetyEvent::COLLISION) {
                        metrics.recordCollision();

                        logger.safety(
                            "Collision detected"
                        );

                    } else {
                        metrics.recordNearMiss();

                        logger.safety(
                            "Near miss detected"
                        );
                    }
                }
            }

            // -------------------------------------------------
            // Emergency braking metrics
            // -------------------------------------------------

            if (event->brakingStarted) {
                metrics.recordEmergencyBrake();

                logger.safety(
                    "Train " +
                    std::to_string(event->id) +
                    " braking"
                );
            }

            // -------------------------------------------------
            // Display meaningful telemetry updates
            // -------------------------------------------------

            if (lastPrintedBlock[event->id] != event->block ||
                event->brakingStarted ||
                event->completed) {

                std::cout
                    << "Telemetry train "
                    << event->id
                    << " block="
                    << (
                        event->block.empty()
                            ? "yard"
                            : event->block
                    )
                    << " position="
                    << std::fixed
                    << std::setprecision(1)
                    << event->position
                    << " m velocity="
                    << event->velocity
                    << " m/s\n";

                lastPrintedBlock[event->id] = event->block;
            }
        }
    });

    // ---------------------------------------------------------
    // 9. Initialize simulation state
    // ---------------------------------------------------------

    std::vector<bool> trainActive(
        trains.size(),
        true
    );

    bool runtimeRerouted = false;

    bool train2Complete = trains.size() < 2;

    int executedTicks = 0;

    // IMPORTANT:
    // Never silently increase the caller's tick limit.
    // This preserves engine.run(step, ticks) semantics and
    // ensures engine.run(1ms, 1) executes at most one tick.

    for (; executedTicks < ticks &&
           std::any_of(
               trainActive.begin(),
               trainActive.end(),
               [](bool active) {
                   return active;
               }
           );
         ++executedTicks) {

        // -----------------------------------------------------
        // 10. Run a single-train scenario
        // -----------------------------------------------------

        if (trains.size() == 1 &&
            trainActive[0] &&
            trains[0]->isNextBlockAvailable(blocks)) {

            const auto event = trains[0]->update(
                step.count() / 1000.0,
                100.0
            );

            queue.push(event);

            trainActive[0] = !event.completed;
        }

        // -----------------------------------------------------
        // 11. Move the second train first
        // -----------------------------------------------------

        if (trains.size() > 1 &&
            trainActive[1] &&
            trains[1]->isNextBlockAvailable(blocks)) {

            const auto event = trains[1]->update(
                step.count() / 1000.0,
                100.0
            );

            queue.push(event);

            trainActive[1] = !event.completed;

            if (event.completed && !train2Complete) {
                interlocking.releaseBlocks(
                    trains[1]->getId(),
                    routeBlocks(routesForTrains[1])
                );

                train2Complete = true;

                logger.info(
                    "Train " +
                    std::to_string(trains[1]->getId()) +
                    " released its completed route"
                );
            }
        }

        // -----------------------------------------------------
        // 12. Runtime obstruction and dynamic rerouting
        // -----------------------------------------------------

        if (train2Complete &&
            trainActive[0] &&
            trains.size() > 1) {

            if (!runtimeRerouted) {
                // Introduce the runtime obstruction.
                blocks.setBlocked(
                    "A->B",
                    true
                );

                logger.warning(
                    "Runtime obstruction: block A->B is BLOCKED"
                );

                // Check whether the first train can enter its next block.
                if (!trains[0]->isNextBlockAvailable(blocks)) {
                    trains[0]->invalidateMovementAuthority(
                        trains[0]->getCurrentBlock()
                    );

                    // Calculate an alternative route that avoids A->B.
                    const auto candidate = routes.calculate(
                        trains[0]->getCurrentNode(),
                        trains[0]->getDestination(),
                        {},
                        {"A->B"}
                    );

                    if (candidate &&
                        interlocking.authorizeReplacementRoute(
                            trains[0]->getId(),
                            *candidate,
                            trains[0]->getCurrentNode(),
                            trains[0]->getDestination())) {

                        std::vector<std::string> obsolete;

                        const auto oldRoute =
                            trains[0]->getRemainingRoute();

                        const auto candidateBlocks =
                            routeBlocks(*candidate);

                        const std::unordered_set<std::string>
                            replacementBlocks(
                                candidateBlocks.begin(),
                                candidateBlocks.end()
                            );

                        // Determine which old route blocks are no
                        // longer required, retaining the current block.
                        for (std::size_t index = 1;
                             index < oldRoute.size();
                             ++index) {

                            const std::string block =
                                oldRoute[index - 1] +
                                "->" +
                                oldRoute[index];

                            if (block !=
                                    trains[0]->getCurrentBlock() &&
                                !replacementBlocks.contains(block)) {

                                obsolete.push_back(block);
                            }
                        }

                        interlocking.releaseBlocks(
                            trains[0]->getId(),
                            obsolete
                        );

                        trains[0]->replaceRemainingRoute(
                            *candidate
                        );

                        routesForTrains[0] = *candidate;

                        runtimeRerouted = true;

                        metrics.recordRouteChange();

                        metrics.recordInterlockingConflictPrevented();

                        logger.route(
                            "Train 1 runtime route change: " +
                            routeText(*candidate)
                        );
                    }
                }
            }

            // -------------------------------------------------
            // 13. Move the first train after rerouting
            // -------------------------------------------------

            if (runtimeRerouted) {
                const bool available =
                    trains[0]->isNextBlockAvailable(blocks);

                if (!available) {
                    std::cerr
                        << "[DEBUG] Train 1 waiting"
                        << " currentBlock="
                        << trains[0]->getCurrentBlock()
                        << " nextBlock="
                        << trains[0]->getNextBlock()
                        << " routeIndex="
                        << trains[0]->getRouteIndex()
                        << " position="
                        << trains[0]->getPositionOnEdge()
                        << '\n';

                } else {
                    const auto event = trains[0]->update(
                        step.count() / 1000.0,
                        30.0
                    );

                    queue.push(event);

                    trainActive[0] = !event.completed;
                }
            }
        }

        // -----------------------------------------------------
        // 14. Validate block availability for active trains
        // -----------------------------------------------------

        for (std::size_t index = 0;
             index < trains.size();
             ++index) {

            if (trainActive[index] &&
                !trains[index]->isNextBlockAvailable(blocks)) {

                trains[index]->invalidateMovementAuthority(
                    trains[index]->getCurrentBlock()
                );
            }
        }

        metrics.tick();
    }

    // ---------------------------------------------------------
    // 15. Drain telemetry before reading completion results
    // ---------------------------------------------------------

    queue.close();

    consumer.join();

    // Each completed train is recorded once, using its actual
    // completion telemetry event.
    for (const int trainId : completedTrainIds) {
        metrics.recordTrainCompleted();

        logger.info(
            "Train " +
            std::to_string(trainId) +
            " completed its journey"
        );
    }

    // ---------------------------------------------------------
    // 16. Release route reservations and count completed trains
    // ---------------------------------------------------------

    int completedCount = 0;

    for (std::size_t index = 0;
         index < trains.size();
         ++index) {

        if (!trainActive[index]) {
            ++completedCount;
        }

        interlocking.release(
            trains[index]->getId(),
            routeBlocks(routesForTrains[index])
        );
    }

    // ---------------------------------------------------------
    // 17. Print final metrics and safety status
    // ---------------------------------------------------------

    metrics.print(
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(
            std::chrono::steady_clock::now() - started
        )
    );

    const bool allCompleted =
        completedCount == static_cast<int>(trains.size());

    std::cout
        << "System Status     : "
        << (
            allCompleted
                ? "SAFE"
                : "INCOMPLETE - NOT SAFE"
        )
        << '\n';

    return executedTicks;
}