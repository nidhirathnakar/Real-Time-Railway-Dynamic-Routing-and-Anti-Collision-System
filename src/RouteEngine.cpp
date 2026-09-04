#include "RouteEngine.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>

RouteEngine::RouteEngine(const RailwayGraph& railwayGraph) : graph(railwayGraph) {
}

std::optional<Route> RouteEngine::calculate(
    const std::string& start,
    const std::string& destination,
    const std::unordered_set<std::string>& blockedNodes,
    const std::unordered_set<std::string>& blockedEdges
) const {
    if (!graph.hasNode(start) || !graph.hasNode(destination) ||
        blockedNodes.contains(start) || blockedNodes.contains(destination)) {
        return std::nullopt;
    }

    using QueueEntry = std::pair<double, std::string>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;
    std::unordered_map<std::string, double> distances;
    std::unordered_map<std::string, std::string> previous;
    distances[start] = 0.0;
    frontier.emplace(0.0, start);

    while (!frontier.empty()) {
        const auto [distance, node] = frontier.top();
        frontier.pop();
        if (distance != distances[node]) {
            continue;
        }
        if (node == destination) {
            break;
        }

        for (const auto& edge : graph.getNeighbors(node)) {
            const std::string edgeKey = node + "->" + edge.destination;
            if (blockedNodes.contains(edge.destination) || blockedEdges.contains(edgeKey)) {
                continue;
            }
            const double candidate = distance + edge.distance;
            if (!distances.contains(edge.destination) || candidate < distances[edge.destination]) {
                distances[edge.destination] = candidate;
                previous[edge.destination] = node;
                frontier.emplace(candidate, edge.destination);
            }
        }
    }

    if (!distances.contains(destination) || !std::isfinite(distances[destination])) {
        return std::nullopt;
    }

    Route route{{}, {}, distances[destination]};
    for (std::string node = destination;; node = previous.at(node)) {
        route.nodes.push_back(node);
        if (node == start) {
            break;
        }
    }
    std::reverse(route.nodes.begin(), route.nodes.end());
    for (std::size_t index = 1; index < route.nodes.size(); ++index) {
        for (const auto& edge : graph.getNeighbors(route.nodes[index - 1])) {
            if (edge.destination == route.nodes[index]) {
                route.edgeDistances.push_back(edge.distance);
                break;
            }
        }
    }
    return route;
}