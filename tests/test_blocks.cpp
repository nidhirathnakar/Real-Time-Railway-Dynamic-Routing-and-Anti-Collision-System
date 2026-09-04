#include <cassert>
#include "BlockManager.h"
int main() { BlockManager blocks; blocks.addBlock(TrackBlock("B1", "A", "B", 10000, 30)); assert(blocks.reserveBlock("B1", 1)); assert(!blocks.reserveBlock("B1", 2)); assert(blocks.occupyBlock("B1", 1)); assert(blocks.isOccupied("B1")); blocks.releaseOccupancy("B1", 1); assert(blocks.isAvailable("B1")); }
