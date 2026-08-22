#ifndef RAILWAY_GRAPH_H
#define RAILWAY_GRAPH_H

#include <string>
#include <unordered_map>
#include <vector>

struct Edge {
    std::string destination;
    double distance;
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

    void displayGraph() const;
};

#endif