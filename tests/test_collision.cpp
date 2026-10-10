#include <cassert>
#include "CollisionDetector.h"
#include "Metrics.h"
int main() {
	TrainTelemetry first; first.id = 1; first.block = "B3"; first.position = 2000;
	TrainTelemetry second = first; second.id = 2; second.position = 5000;
	Metrics metrics;
	assert(CollisionDetector().inspect({first, second}).front().type == SafetyEvent::NEAR_MISS);
	metrics.recordNearMiss();
	second.position = 2500;
	assert(CollisionDetector().inspect({first, second}).front().type == SafetyEvent::COLLISION);
	metrics.recordCollision();
	assert(metrics.nearMissCount() == 1);
	assert(metrics.collisionCount() == 1);
	assert(metrics.collisionPreventedCount() == 0);
	assert(metrics.interlockingConflictPreventedCount() == 0);
}
