#ifndef RAILWAY_GRAPH_H
#define RAILWAY_GRAPH_H

#include <string>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

struct Edge {
    std::string destination;
    double distance;
};

struct Route {
    std::vector<std::string> nodes;
    std::vector<double> edgeDistances;
    double distance{};
};

class RailwayGraph {
private:
    std::unordered_map<std::string, std::vector<Edge>> adjacencyList;

public:
    void addNode(const std::string& node);

    void addTrack(
        const std::string& from,
        const std::string& to,
        double distance
    );

    bool hasNode(const std::string& node) const;
    const std::vector<Edge>& getNeighbors(const std::string& node) const;

    std::optional<Route> shortestPath(
        const std::string& start,
        const std::string& destination,
        const std::unordered_set<std::string>& blockedNodes = {},
        const std::unordered_set<std::string>& blockedEdges = {}
    ) const;

    void displayGraph() const;
};

#endif