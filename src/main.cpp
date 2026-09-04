#include <atomic>
#include <exception>
#include <iostream>

#include "SimulationEngine.h"

int main() {
    try {
        std::cout << "========================================\n"
                  << "   RAILWAY ANTI-COLLISION SIMULATOR\n"
                  << "========================================\n";
        SimulationEngine().run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Simulation error: " << error.what() << '\n';
        return 1;
    }
}