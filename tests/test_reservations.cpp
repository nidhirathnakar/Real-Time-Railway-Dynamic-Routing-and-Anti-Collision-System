#include <cassert>
#include "ReservationManager.h"
int main() { ReservationManager reservations; assert(reservations.tryReserve(1, {"B3"})); assert(!reservations.tryReserve(2, {"B3"})); reservations.release(1); assert(reservations.tryReserve(2, {"B3"})); }
