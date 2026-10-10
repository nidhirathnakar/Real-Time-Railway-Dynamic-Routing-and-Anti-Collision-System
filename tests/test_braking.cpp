#include <cassert>
#include <cmath>
#include "BrakingModel.h"
#include "Metrics.h"
#include "Train.h"
int main() {
	BrakingModel model(2.0, 1.0);
	assert(std::abs(model.stoppingDistance(20.0) - 240.0) < 0.001);
	assert(model.requiresBraking(20.0, 100.0));
	Train train(1, "A", "B", 20.0);
	train.setRoute(Route{{"A", "B"}, {10000.0}, 10000.0});
	Metrics metrics;
	const auto first = train.update(0.1, 0.0);
	const auto second = train.update(0.1, 0.0);
	assert(first.brakingStarted && !second.brakingStarted);
	metrics.recordEmergencyBrake();
	assert(metrics.emergencyBrakeCount() == 1);
}
