#include <cassert>
#include <cmath>
#include "BrakingModel.h"
int main() { BrakingModel model(2.0, 1.0); assert(std::abs(model.stoppingDistance(20.0) - 240.0) < 0.001); assert(model.requiresBraking(20.0, 100.0)); }
