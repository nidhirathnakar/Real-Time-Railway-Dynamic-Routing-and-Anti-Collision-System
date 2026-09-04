#include <cassert>
#include "InterlockingController.h"
int main() { BlockManager blocks; blocks.addBlock(TrackBlock("B3", "A", "B", 10000, 30)); ReservationManager reservations; InterlockingController controller(blocks, reservations); assert(controller.authorize(1, {"B3"})); assert(!controller.authorize(2, {"B3"})); controller.release(1, {"B3"}); assert(controller.authorize(2, {"B3"})); }
