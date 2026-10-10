#include "Scenario.h"

#include <stdexcept>

ScenarioConfig defaultScenario() {
    return {
        {"A", "B", "C", "D", "E", "F"},
        {
            {"A", "B", 10000.0},
            {"A", "D", 15000.0},
            {"B", "C", 10000.0},
            {"C", "F", 10000.0},
            {"D", "E", 10000.0},
            {"E", "F", 10000.0}
        },
        {
            {1, "A", "F", 20.0},
            {2, "A", "F", 18.0}
        },
        {
            {"A-switch", "A", {"B", "D"}}
        },
        {}
    };
}

ScenarioConfig followingTrainsScenario() {
    auto scenario = defaultScenario();
    scenario.trains = {
        {1, "A", "F", 20.0},
        {2, "A", "F", 10.0}
    };
    return scenario;
}

ScenarioConfig switchConflictScenario() {
    auto scenario = defaultScenario();
    scenario.trains = {
        {1, "A", "F", 20.0},
        {2, "A", "F", 18.0}
    };
    scenario.blockedBlocks = {"A->B"};
    return scenario;
}

ScenarioConfig scenarioByName(const std::string& name) {
    if (name.empty() || name == "default") return defaultScenario();
    if (name == "following") return followingTrainsScenario();
    if (name == "switch-conflict") return switchConflictScenario();
    throw std::invalid_argument("Unknown scenario: " + name);
}
