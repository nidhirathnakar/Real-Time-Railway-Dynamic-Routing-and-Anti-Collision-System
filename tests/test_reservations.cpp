#include <cassert>
#include "ReservationManager.h"

int main() {
	ReservationManager reservations;
	assert(reservations.tryReserve(1, {"A", "B", "C", "D"}));
	reservations.release(1, {"C", "D"});
	const auto partial = reservations.snapshot();
	assert(partial.size() == 2 && partial.at("A") == 1 && partial.at("B") == 1);
	assert(!partial.contains("C") && !partial.contains("D"));

	assert(reservations.tryReserve(2, {"E", "F"}));
	reservations.release(1, {"A", "E", "missing"});
	const auto isolated = reservations.snapshot();
	assert(isolated.size() == 3 && isolated.at("B") == 1 && isolated.at("E") == 2 && isolated.at("F") == 2);

	reservations.release(2, {"E", "E", "F", "F"});
	assert(reservations.snapshot().size() == 1 && reservations.snapshot().at("B") == 1);

	assert(reservations.tryReserve(1, {"C", "D"}));
	reservations.release(1);
	assert(reservations.snapshot().empty());

	assert(reservations.tryReserve(1, {"B3"}));
	assert(!reservations.tryReserve(2, {"B3"}));
	reservations.release(1);
	assert(reservations.tryReserve(2, {"B3"}));
}
