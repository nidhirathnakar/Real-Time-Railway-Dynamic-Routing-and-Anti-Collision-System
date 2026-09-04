#ifndef ROUTE_ENGINE_H
#define ROUTE_ENGINE_H

#include <optional>
#include <string>
#include <unordered_set>

#include "RailwayGraph.h"

class RouteEngine {
public:
    explicit RouteEngine(const RailwayGraph& graph);

    std::optional<Route> calculate(
        const std::string& start,
        const std::string& destination,
        const std::unordered_set<std::string>& blockedNodes = {},
        const std::unordered_set<std::string>& blockedEdges = {}
    ) const;

private:
    const RailwayGraph& graph;
};

#endif