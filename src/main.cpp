#include <iostream>

#include "RailwayGraph.h"
#include "Train.h"

int main() {

    std::cout << "========================================\n";
    std::cout << "   RAILWAY ANTI-COLLISION SIMULATOR\n";
    std::cout << "========================================\n";

    RailwayGraph railway;

    // Railway nodes
    railway.addNode("A");
    railway.addNode("B");
    railway.addNode("C");
    railway.addNode("D");
    railway.addNode("E");
    railway.addNode("F");

    // Railway tracks
    railway.addTrack("A", "B", 10);
    railway.addTrack("A", "D", 15);

    railway.addTrack("B", "C", 10);

    railway.addTrack("C", "F", 10);

    railway.addTrack("D", "E", 10);

    railway.addTrack("E", "F", 10);

    // Display railway
    railway.displayGraph();

    // Create train
    Train train1(
        1,
        "A",
        "F",
        20.0
    );

    train1.displayInfo();

    std::cout << "\nSystem Status : READY\n";

    return 0;
}