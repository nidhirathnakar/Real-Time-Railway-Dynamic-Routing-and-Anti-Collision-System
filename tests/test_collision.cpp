#include <cassert>
#include "CollisionDetector.h"
int main() { TrainTelemetry first; first.id = 1; first.block = "B3"; first.position = 2000; TrainTelemetry second = first; second.id = 2; second.position = 5000; assert(CollisionDetector().inspect({first, second}).front().type == SafetyEvent::NEAR_MISS); second.position = 2500; assert(CollisionDetector().inspect({first, second}).front().type == SafetyEvent::COLLISION); }
