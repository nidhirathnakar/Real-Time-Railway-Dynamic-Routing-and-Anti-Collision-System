#include <chrono>
#include <cassert>
#include <stdexcept>
#include "BlockManager.h"
#include "InterlockingController.h"
#include "RailwayGraph.h"
#include "ReservationManager.h"
#include "RouteEngine.h"
#include "Train.h"
#include "SimulationEngine.h"
#include "Scenario.h"

int main() {
	const auto scenario = defaultScenario();
	const auto selectedDefault = scenarioByName("");
	assert(selectedDefault.trains.size() == scenario.trains.size());
	assert(selectedDefault.trains[0].id == scenario.trains[0].id && selectedDefault.trains[0].destination == scenario.trains[0].destination);
	const auto namedDefault = scenarioByName("default");
	assert(namedDefault.tracks.size() == scenario.tracks.size() && namedDefault.nodes == scenario.nodes);
	assert(scenarioByName("following").trains[1].velocity == 10.0);
	assert(scenarioByName("switch-conflict").blockedBlocks[0] == "A->B");
	bool invalidScenarioRejected = false;
	try { scenarioByName("unknown"); } catch (const std::invalid_argument&) { invalidScenarioRejected = true; }
	assert(invalidScenarioRejected);
	assert(scenario.nodes.size() == 6);
	assert(scenario.tracks.size() == 6);
	assert(scenario.trains.size() == 2);
	assert(scenario.trains[0].id == 1 && scenario.trains[0].start == "A" && scenario.trains[0].destination == "F");
	assert(scenario.trains[1].id == 2 && scenario.trains[1].start == "A" && scenario.trains[1].destination == "F");
	assert(scenario.tracks[0].from == "A" && scenario.tracks[0].to == "B" && scenario.tracks[0].distance == 10000.0);
	assert(scenario.tracks[1].from == "A" && scenario.tracks[1].to == "D" && scenario.tracks[1].distance == 15000.0);
	assert(scenario.switches.size() == 1 && scenario.switches[0].incoming == "A");

	const auto following = followingTrainsScenario();
	assert(following.trains.size() == 2);
	assert(following.trains[0].start == "A" && following.trains[0].destination == "F");
	assert(following.trains[1].start == "A" && following.trains[1].destination == "F");
	assert(following.trains[0].velocity > following.trains[1].velocity);

	const auto conflict = switchConflictScenario();
	assert(conflict.trains.size() == 2);
	assert(conflict.trains[0].destination == "F" && conflict.trains[1].destination == "F");
	assert(conflict.blockedBlocks.size() == 1 && conflict.blockedBlocks[0] == "A->B");
	assert(conflict.switches.size() == 1 && conflict.switches[0].incoming == "A");
	RailwayGraph conflictGraph;
	for (const auto& node : conflict.nodes) conflictGraph.addNode(node);
	for (const auto& track : conflict.tracks) conflictGraph.addTrack(track.from, track.to, track.distance);
	RouteEngine conflictRoutes(conflictGraph);
	const auto conflictRoute = conflictRoutes.calculate("A", "F", {}, {"A->B"});
	assert(conflictRoute && conflictRoute->nodes == std::vector<std::string>({"A", "D", "E", "F"}));

	RailwayGraph graph;
	for (const auto& node : {"A", "B", "C", "D", "E", "F"}) graph.addNode(node);
	graph.addTrack("A", "B", 10000.0); graph.addTrack("B", "C", 10000.0); graph.addTrack("C", "F", 10000.0);
	graph.addTrack("A", "D", 15000.0); graph.addTrack("D", "E", 10000.0); graph.addTrack("E", "F", 10000.0);
	RouteEngine routes(graph);
	const auto primary = routes.calculate("A", "F");
	const auto alternate = routes.calculate("A", "F", {}, {"A->B"});
	assert(primary && alternate);

	BlockManager blocks;
	for (const auto& [from, to, distance] : {std::tuple{"A", "B", 10000.0}, {"B", "C", 10000.0}, {"C", "F", 10000.0}, {"A", "D", 15000.0}, {"D", "E", 10000.0}, {"E", "F", 10000.0}}) {
		blocks.addBlock(TrackBlock(std::string(from) + "->" + to, from, to, distance, 30.0));
	}
	ReservationManager reservations;
	InterlockingController interlocking(blocks, reservations);
	Train train(1, "A", "F", 20.0);
	train.setRoute(*primary);
	assert(interlocking.authorize(1, {"A->B", "B->C", "C->F"}));
	blocks.setBlocked("A->B", true);
	assert(!train.isNextBlockAvailable(blocks));
	assert(interlocking.authorizeReplacementRoute(1, *alternate, "A", "F"));
	interlocking.releaseBlocks(1, {"B->C", "C->F"});
	train.replaceRemainingRoute(*alternate);
	assert(train.getCurrentNode() == "A");
	assert(train.getPositionOnEdge() == 0.0);
	assert(train.getCurrentBlock() == "A->D");
	assert(reservations.snapshot().at("A->B") == 1);
	assert(!reservations.snapshot().contains("B->C"));
	assert(train.update(1.0, 100000.0).block == "A->D");

	SimulationEngine engine;
	assert(engine.run(std::chrono::milliseconds(1), 1) == 1);
}
