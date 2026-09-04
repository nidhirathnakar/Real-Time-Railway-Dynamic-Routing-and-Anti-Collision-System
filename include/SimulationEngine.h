#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <chrono>

class SimulationEngine {
public:
    void run(std::chrono::milliseconds step = std::chrono::milliseconds(1000), int ticks = 5000);
};

#endif