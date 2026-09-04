#include <cassert>
#include "RailwayGraph.h"
int main() { RailwayGraph graph; graph.addNode("A"); graph.addTrack("A", "B", 10000.0); assert(graph.hasNode("B")); assert(graph.getNeighbors("A").size() == 1); }
