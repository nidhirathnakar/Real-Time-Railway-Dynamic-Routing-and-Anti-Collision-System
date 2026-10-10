#ifndef SCENARIO_H
#define SCENARIO_H

#include <string>
#include <vector>

struct ScenarioTrack {
    std::string from;
    std::string to;
    double distance;
};

struct ScenarioTrain {
    int id;
    std::string start;
    std::string destination;
    double velocity;
};

struct ScenarioSwitch {
    std::string id;
    std::string incoming;
    std::vector<std::string> outgoing;
};

struct ScenarioConfig {
    std::vector<std::string> nodes;
    std::vector<ScenarioTrack> tracks;
    std::vector<ScenarioTrain> trains;
    std::vector<ScenarioSwitch> switches;
    std::vector<std::string> blockedBlocks;
};

ScenarioConfig defaultScenario();
ScenarioConfig followingTrainsScenario();
ScenarioConfig switchConflictScenario();
ScenarioConfig scenarioByName(const std::string& name);

#endif
