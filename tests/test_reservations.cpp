#include <cassert>
#include <atomic>
#include <thread>
#include <vector>
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

		ReservationManager concurrentReservations;
		constexpr int contenderCount = 2;
		std::atomic<int> ready{0};
		std::atomic<bool> go{false};
		std::atomic<int> successes{0};
		std::vector<std::thread> contenders;
		for (int trainId = 1; trainId <= contenderCount; ++trainId) {
			contenders.emplace_back([&, trainId] {
				++ready;
				while (!go.load()) std::this_thread::yield();
				if (concurrentReservations.tryReserve(trainId, {"shared"})) ++successes;
			});
		}
		while (ready.load() != contenderCount) std::this_thread::yield();
		go.store(true);
		for (auto& contender : contenders) contender.join();
		assert(successes.load() == 1);
		assert(!concurrentReservations.tryReserve(99, {"shared"}));
}
