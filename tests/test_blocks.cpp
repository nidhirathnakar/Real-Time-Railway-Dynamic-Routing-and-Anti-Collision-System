#include <cassert>
#include <barrier>
#include <thread>
#include <vector>
#include "BlockManager.h"

int main() {
	BlockManager blocks;
	blocks.addBlock(TrackBlock("A->B", "A", "B", 10000, 30));
	blocks.addBlock(TrackBlock("A->D", "A", "D", 15000, 30));

	assert(blocks.isAvailable("A->B"));
	assert(blocks.isAvailable("A->D"));
	assert(blocks.reserveBlock("A->B", 1));
	assert(blocks.getReservedTrain("A->B") == 1);

	blocks.setBlocked("A->B", true);
	assert(blocks.isBlocked("A->B"));
	assert(!blocks.isAvailable("A->B"));
	assert(blocks.getReservedTrain("A->B") == 1);
	assert(blocks.isAvailable("A->D"));

	blocks.setBlocked("A->B", false);
	assert(!blocks.isBlocked("A->B"));
	assert(blocks.isAvailable("A->B", 1));
	assert(blocks.getReservedTrain("A->B") == 1);

	blocks.setBlocked("A->B", true);
	blocks.setBlocked("A->B", false);
	assert(blocks.isAvailable("A->B", 1));

	BlockManager concurrentBlocks;
	concurrentBlocks.addBlock(TrackBlock("stress", "A", "B", 10000, 30));
	constexpr int readerCount = 4;
	constexpr int rounds = 100;
	std::barrier phaseStart(readerCount + 1);
	std::barrier phaseCheck(readerCount + 1);
	std::barrier phaseEnd(readerCount + 1);
	std::vector<std::jthread> readers;
	for (int reader = 0; reader < readerCount; ++reader) {
		readers.emplace_back([&](std::stop_token) {
			for (int round = 0; round < rounds; ++round) {
				phaseStart.arrive_and_wait();
				phaseCheck.arrive_and_wait();
				assert(concurrentBlocks.isBlocked("stress") == (round % 2 == 0));
				phaseEnd.arrive_and_wait();
			}
		});
	}
	std::jthread blocker([&](std::stop_token) {
		for (int round = 0; round < rounds; ++round) {
			phaseStart.arrive_and_wait();
			concurrentBlocks.setBlocked("stress", round % 2 == 0);
			phaseCheck.arrive_and_wait();
			phaseEnd.arrive_and_wait();
		}
	});
}
