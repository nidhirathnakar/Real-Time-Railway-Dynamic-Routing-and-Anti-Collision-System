#include <cassert>
#include "RailwayGraph.h"
#include "RouteEngine.h"
int main() { RailwayGraph graph; for (const auto& node : {"A", "B", "C", "F"}) graph.addNode(node); graph.addTrack("A", "B", 10000); graph.addTrack("B", "C", 10000); graph.addTrack("C", "F", 10000); RouteEngine engine(graph); const auto route = engine.calculate("A", "F"); assert(route && route->distance == 30000.0); assert(!engine.calculate("X", "F")); }
