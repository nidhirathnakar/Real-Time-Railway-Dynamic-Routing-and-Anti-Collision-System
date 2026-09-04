#include "RailwayGraph.h"

#include "RouteEngine.h"

#include <stdexcept>
#include <iostream>

void RailwayGraph::addNode(const std::string& node) {
    adjacencyList[node];
}

void RailwayGraph::addTrack(
    const std::string& from,
    const std::string& to,
    double distance
) {
    if (from.empty() || to.empty() || distance <= 0.0) {
        throw std::invalid_argument("Track endpoints must be non-empty and distance must be positive");
    }

    adjacencyList[to];
    adjacencyList[from].push_back({
        to,
        distance
    });
}

bool RailwayGraph::hasNode(const std::string& node) const {
    return adjacencyList.contains(node);
}

const std::vector<Edge>& RailwayGraph::getNeighbors(const std::string& node) const {
    static const std::vector<Edge> empty;
    const auto iterator = adjacencyList.find(node);
    return iterator == adjacencyList.end() ? empty : iterator->second;
}

std::optional<Route> RailwayGraph::shortestPath(
    const std::string& start,
    const std::string& destination,
    const std::unordered_set<std::string>& blockedNodes,
    const std::unordered_set<std::string>& blockedEdges
) const {
    return RouteEngine(*this).calculate(start, destination, blockedNodes, blockedEdges);
}

void RailwayGraph::displayGraph() const {

    std::cout << "\nRailway Network:\n";

    for (const auto& [node, edges] : adjacencyList) {

        std::cout << node << " -> ";

        for (const auto& edge : edges) {

            std::cout
                << edge.destination
                << " ("
                << edge.distance / 1000.0
                << " km) ";
        }

        std::cout << '\n';
    }
}