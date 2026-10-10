#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#include "SimulationEngine.h"

int main(int argc, char* argv[]) {
    try {
        std::cout
            << "========================================\n"
            << "   RAILWAY ANTI-COLLISION SIMULATOR\n"
            << "========================================\n";

        const std::string scenarioName =
            argc > 1 ? argv[1] : "default";

        // Preserve the existing 1-second simulation step.
        // Give the following scenario enough ticks for both trains.
        const int tickLimit =
            (scenarioName == "following") ? 13000 : 5000;

        SimulationEngine().run(
            scenarioByName(scenarioName),
            std::chrono::milliseconds(1000),
            tickLimit
        );

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr
            << "Simulation error: "
            << error.what()
            << '\n';

        return 1;
    }
}