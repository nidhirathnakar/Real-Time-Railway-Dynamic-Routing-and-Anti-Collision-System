#include <chrono>
#include "SimulationEngine.h"
int main() { SimulationEngine engine; engine.run(std::chrono::milliseconds(1), 1); }
