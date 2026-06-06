#pragma once
#include <stdexcept>
#include <string>

using namespace std;

class DroneException : public runtime_error {
public:
    DroneException(string msg) : runtime_error(msg) {}
};

class BatteryDepletedError : public DroneException {
public:
    BatteryDepletedError(string drone_name) : DroneException("Battery is empty on drone " + drone_name) {}
};

class InvalidStateError : public DroneException {
public:
    InvalidStateError(string drone_name, string current_state, string required_state) : DroneException("Drone " + drone_name + " is in state " + current_state + " but needs to be in state " + required_state) {}
};

class AltitudeError : public DroneException {
public:
    AltitudeError(string drone_name, float requested, float max_allowed) : DroneException("Drone " + drone_name + " cannot go to altitude " + to_string(requested) + " because max is " + to_string(max_allowed)) {}
};
