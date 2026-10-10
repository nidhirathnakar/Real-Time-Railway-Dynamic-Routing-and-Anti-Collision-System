#include <cassert>
#include <atomic>
#include <barrier>
#include <string>
#include <thread>
#include <vector>

#include "InterlockingController.h"
#include "Metrics.h"

namespace {

Route primaryRoute() {
	return {{"A", "B", "C", "F"}, {10000.0, 10000.0, 10000.0}, 30000.0};
}

Route alternateRoute() {
	return {{"A", "D", "E", "F"}, {15000.0, 10000.0, 10000.0}, 35000.0};
}

void addScenarioBlocks(BlockManager& blocks) {
	blocks.addBlock(TrackBlock("A->B", "A", "B", 10000.0, 30.0));
	blocks.addBlock(TrackBlock("B->C", "B", "C", 10000.0, 30.0));
	blocks.addBlock(TrackBlock("C->F", "C", "F", 10000.0, 30.0));
	blocks.addBlock(TrackBlock("A->D", "A", "D", 15000.0, 30.0));
	blocks.addBlock(TrackBlock("D->E", "D", "E", 10000.0, 30.0));
	blocks.addBlock(TrackBlock("E->F", "E", "F", 10000.0, 30.0));
}

}

int main() {
	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		Switch junction("A-switch", "A", {"B", "D"});
		assert(junction.setPosition("B", false));
		controller.registerSwitch(junction);
		assert(controller.authorizeRoute(1, primaryRoute()));
		assert(reservations.snapshot().at("A->B") == 1);
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		Switch junction("A-switch", "A", {"B", "D"});
		assert(junction.setPosition("D", false));
		controller.registerSwitch(junction);
		assert(!controller.authorizeRoute(1, primaryRoute()));
		assert(reservations.snapshot().empty());
		assert(!junction.setPosition("B", true));
		assert(junction.getPosition() == "D");
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		Switch junction("A-switch", "A", {"B", "D"});
		assert(junction.setPosition("B", false));
		controller.registerSwitch(junction);
		assert(controller.authorizeRoute(1, primaryRoute()));
		assert(junction.setPosition("D", false));
		assert(controller.authorizeReplacementRoute(1, alternateRoute(), "A", "F"));
		const auto state = reservations.snapshot();
		assert(state.size() == 6 && state.at("A->B") == 1 && state.at("A->D") == 1);
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		assert(controller.authorize(1, {"A->B", "B->C", "C->F"}));
		assert(controller.authorize(2, {"E->F"}));
		assert(!controller.authorizeReplacementRoute(1, alternateRoute(), "A", "F"));
		const auto state = reservations.snapshot();
		assert(state.size() == 4 && state.at("A->B") == 1 && state.at("E->F") == 2);
		assert(!state.contains("A->D") && !state.contains("D->E"));
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		blocks.setBlocked("D->E", true);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		assert(controller.authorize(1, {"A->B", "B->C", "C->F"}));
		assert(!controller.authorizeReplacementRoute(1, alternateRoute(), "A", "F"));
		assert(reservations.snapshot().size() == 3);
		assert(!reservations.snapshot().contains("A->D"));
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		assert(!controller.authorizeReplacementRoute(1, {}, "A", "F"));
		assert(!controller.authorizeReplacementRoute(1, primaryRoute(), "B", "F"));
		assert(reservations.snapshot().empty());
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		assert(controller.authorize(1, {"A->B"}));
		assert(controller.authorize(2, {"E->F"}));
		assert(!controller.authorizeReplacementRoute(1, alternateRoute(), "A", "F"));
		const auto state = reservations.snapshot();
		assert(state.at("A->B") == 1 && state.at("E->F") == 2);
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		blocks.addBlock(TrackBlock("B3", "A", "B", 10000.0, 30.0));
		assert(controller.authorize(1, {"B3"}));
		assert(!controller.authorize(2, {"B3"}));
	}

	{
		BlockManager blocks;
		addScenarioBlocks(blocks);
		ReservationManager reservations;
		InterlockingController controller(blocks, reservations);
		assert(controller.authorize(1, {"A->B", "B->C", "C->F"}));
		constexpr int contenderCount = 6;
		std::barrier start(contenderCount);
		std::atomic<int> successes{0};
		std::vector<std::jthread> contenders;
		for (int trainId = 2; trainId < contenderCount + 2; ++trainId) {
			contenders.emplace_back([&, trainId](std::stop_token) {
				start.arrive_and_wait();
				if (controller.authorizeReplacementRoute(trainId, alternateRoute(), "A", "F")) ++successes;
			});
		}
		for (auto& contender : contenders) contender.join();
		assert(successes.load() == 1);
		assert(reservations.snapshot().size() == 6);
		Metrics metrics;
		metrics.recordInterlockingConflictPrevented();
		assert(metrics.interlockingConflictPreventedCount() == 1);
		assert(metrics.collisionPreventedCount() == 0);
	}
}
