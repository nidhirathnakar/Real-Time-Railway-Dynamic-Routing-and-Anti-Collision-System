#include <cassert>
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
}
