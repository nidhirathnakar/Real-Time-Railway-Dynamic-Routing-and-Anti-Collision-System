#include "Train.h"

#include <iostream>

Train::Train(
    int id,
    const std::string& start,
    const std::string& destination,
    double velocity
)
    : id(id),
      start(start),
      destination(destination),
      velocity(velocity) {
}

void Train::displayInfo() const {

    std::cout << "\nTrain ID       : " << id;
    std::cout << "\nStart          : " << start;
    std::cout << "\nDestination    : " << destination;
    std::cout << "\nVelocity       : "
              << velocity
              << " m/s\n";
}