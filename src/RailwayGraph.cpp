#include "RailwayGraph.h"

#include <iostream>

void RailwayGraph::addNode(const std::string& node) {
    adjacencyList[node];
}

void RailwayGraph::addTrack(
    const std::string& from,
    const std::string& to,
    double distance
) {
    adjacencyList[from].push_back({
        to,
        distance
    });
}

void RailwayGraph::displayGraph() const {

    std::cout << "\nRailway Network:\n";

    for (const auto& [node, edges] : adjacencyList) {

        std::cout << node << " -> ";

        for (const auto& edge : edges) {

            std::cout
                << edge.destination
                << " ("
                << edge.distance
                << " km) ";
        }

        std::cout << '\n';
    }
}