#ifndef TRAIN_H
#define TRAIN_H

#include <string>

class Train {
private:
    int id;
    std::string start;
    std::string destination;
    double velocity;

public:
    Train(
        int id,
        const std::string& start,
        const std::string& destination,
        double velocity
    );

    void displayInfo() const;
};

#endif