#ifndef BRAKING_MODEL_H
#define BRAKING_MODEL_H

class BrakingModel {
public:
    BrakingModel(double reactionTime, double deceleration);
    double stoppingDistance(double velocity) const;
    bool requiresBraking(double velocity, double distanceToObstacle) const;
    double deceleration() const;

private:
    double reactionTime;
    double brakingDeceleration;
};

#endif