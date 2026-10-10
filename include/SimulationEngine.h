#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <chrono>

#include "Scenario.h"

class SimulationEngine {
public:
    int run(std::chrono::milliseconds step = std::chrono::milliseconds(1000), int ticks = 5000);
    int run(const ScenarioConfig& scenario, std::chrono::milliseconds step = std::chrono::milliseconds(1000), int ticks = 5000);
};

#endif