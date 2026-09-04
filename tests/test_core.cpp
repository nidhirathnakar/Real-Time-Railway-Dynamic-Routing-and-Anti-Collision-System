#include <cassert>
#include <cmath>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "RailwayGraph.h"
#include "BlockManager.h"
#include "BrakingModel.h"
#include "CollisionDetector.h"
#include "ReservationManager.h"
#include "RouteEngine.h"
#include "Switch.h"
#include "TelemetryQueue.h"
#include "Train.h"

int main() {
    RailwayGraph graph;
    for (const auto& node : {"A", "B", "C", "D", "E"}) {
        graph.addNode(node);
    }
    graph.addTrack("A", "B", 2000.0);
    graph.addTrack("B", "E", 2000.0);
    graph.addTrack("A", "C", 1000.0);
    graph.addTrack("C", "D", 1000.0);
    graph.addTrack("D", "E", 1000.0);

    RouteEngine routeEngine(graph);
    const auto shortest = routeEngine.calculate("A", "E");
    const std::vector<std::string> expectedShortest{"A", "C", "D", "E"};
    assert(shortest && shortest->nodes == expectedShortest);
    assert(std::abs(shortest->distance - 3000.0) < 0.001);

    const auto rerouted = routeEngine.calculate("A", "E", {}, {"A->C"});
    const std::vector<std::string> expectedReroute{"A", "B", "E"};
    assert(rerouted && rerouted->nodes == expectedReroute);
    assert(!routeEngine.calculate("unknown", "E"));
    assert(!routeEngine.calculate("A", "E", {}, {"A->B", "A->C"}));

    ReservationManager reservations;
    assert(reservations.tryReserve(1, {"A->B", "B->E"}));
    assert(!reservations.tryReserve(2, {"B->E"}));
    assert(reservations.tryReserve(1, {"A->B"}));
    reservations.release(1);
    assert(reservations.tryReserve(2, {"B->E"}));

    Train train(7, "A", "E", 20.0);
    train.setRoute(*shortest);
    assert(std::abs(Train::stoppingDistance(20.0, 1.0, 8.0) - 45.0) < 0.001);
    const auto braking = train.update(0.25, 10.0);
    assert(braking.emergencyBraking);
    assert(braking.velocity < 20.0);
    const auto moving = train.update(0.25, 1000.0);
    assert(moving.position > 0.0);
    assert(moving.block == "A->C");
    assert(moving.state == TrainState::MOVING);
    assert(train.getCurrentNode() == "A");
    assert(train.getCurrentBlock() == "A->C");
    assert(train.getNextBlock() == "C->D");
    assert(train.getDestination() == "E");
    assert(train.getRouteIndex() == 0);
    assert(train.getRemainingRoute() == expectedShortest);

    const auto positionBeforeReplacement = train.getPositionOnEdge();
    train.replaceRemainingRoute(*shortest);
    assert(train.getPositionOnEdge() == positionBeforeReplacement);
    assert(train.getCurrentNode() == "A");
    assert(train.getCurrentBlock() == "A->C");
    const auto afterReplacement = train.update(0.25, 1000.0);
    assert(afterReplacement.position > positionBeforeReplacement);

    Train boundaryTrain(8, "A", "F", 20.0);
    const auto boundaryRoute = graph.shortestPath("A", "E");
    assert(boundaryRoute);
    boundaryTrain.setRoute(*boundaryRoute);
    const auto alternateBoundaryRoute = Route{{"A", "B", "E"}, {2000.0, 2000.0}, 4000.0};
    boundaryTrain.replaceRemainingRoute(alternateBoundaryRoute);
    assert(boundaryTrain.getCurrentNode() == "A");
    assert(boundaryTrain.getCurrentBlock() == "A->B");
    assert(boundaryTrain.getPositionOnEdge() == 0.0);
    const auto boundaryMovement = boundaryTrain.update(0.25, 1000.0);
    assert(boundaryMovement.block == "A->B");
    assert(boundaryMovement.position > 0.0);

    BlockManager obstructionBlocks;
    obstructionBlocks.addBlock(TrackBlock("A->B", "A", "B", 10000.0, 30.0));
    obstructionBlocks.addBlock(TrackBlock("A->D", "A", "D", 15000.0, 30.0));
    Train obstructionTrain(10, "A", "F", 20.0);
    const Route preferredRoute{{"A", "B", "C", "F"}, {10000.0, 10000.0, 10000.0}, 30000.0};
    obstructionTrain.setRoute(preferredRoute);
    assert(obstructionTrain.getCurrentNode() == "A");
    assert(obstructionTrain.getCurrentBlock() == "A->B");
    assert(obstructionTrain.isNextBlockAvailable(obstructionBlocks));
    assert(obstructionBlocks.reserveBlock("A->B", 10));
    const auto positionBeforeObstruction = obstructionTrain.getPositionOnEdge();
    const auto routeBeforeObstruction = obstructionTrain.getRemainingRoute();
    obstructionBlocks.setBlocked("A->B", true);
    assert(!obstructionTrain.isNextBlockAvailable(obstructionBlocks));
    if (obstructionTrain.isNextBlockAvailable(obstructionBlocks)) {
        obstructionTrain.update(1.0, 100000.0);
    }
    assert(obstructionTrain.getPositionOnEdge() == positionBeforeObstruction);
    assert(obstructionTrain.getRemainingRoute() == routeBeforeObstruction);
    assert(obstructionBlocks.getReservedTrain("A->B") == 10);
    assert(obstructionBlocks.isAvailable("A->D"));
    obstructionBlocks.setBlocked("A->B", false);
    assert(obstructionTrain.isNextBlockAvailable(obstructionBlocks));
    assert(obstructionTrain.update(0.25, 100000.0).position > positionBeforeObstruction);

    Train arrivingTrain(9, "A", "E", 20.0);
    arrivingTrain.setRoute(*shortest);
    TrainTelemetry arrival;
    for (int updateCount = 0; updateCount < 200 && !arrival.completed; ++updateCount) {
        arrival = arrivingTrain.update(1.0, 100000.0);
    }
    assert(arrival.completed);
    assert(arrival.state == TrainState::ARRIVED);
    assert(arrival.velocity == 0.0);
    assert(arrival.block.empty());
    const auto afterArrival = arrivingTrain.update(1.0, 100000.0);
    assert(afterArrival.completed && afterArrival.velocity == 0.0 && afterArrival.position == 0.0);

    BlockManager blockManager;
    blockManager.addBlock(TrackBlock("B1", "A", "B", 2.0, 20.0));
    assert(blockManager.reserveBlock("B1", 1));
    assert(!blockManager.reserveBlock("B1", 2));
    assert(blockManager.occupyBlock("B1", 1));
    assert(blockManager.isOccupied("B1"));
    blockManager.releaseOccupancy("B1", 1);
    assert(blockManager.isAvailable("B1"));

    Switch junction("S1", "A", {"B", "D"});
    assert(junction.setPosition("B", false));
    assert(!junction.setPosition("D", true));
    assert(junction.accepts("B"));

    BrakingModel brakingModel(2.0, 1.0);
    assert(std::abs(brakingModel.stoppingDistance(20.0) - 240.0) < 0.001);
    assert(brakingModel.requiresBraking(20.0, 100.0));

    TrainTelemetry first;
    first.id = 1; first.node = "A"; first.block = "A->B"; first.position = 2000.0;
    TrainTelemetry second = first;
    second.id = 2; second.position = 5000.0;
    const auto events = CollisionDetector().inspect({first, second});
    assert(events.size() == 1 && events.front().type == SafetyEvent::NEAR_MISS);

    TelemetryQueue queue(1);
    assert(queue.push(first));
    assert(queue.pop()->id == 1);
    queue.close();
    assert(!queue.push(second));

    std::cout << "All core railway tests passed\n";
}
