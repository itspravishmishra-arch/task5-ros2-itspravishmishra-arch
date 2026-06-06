#pragma once
#include "vehicle.hpp"

using namespace std;

class Drone : public Vehicle {
public:
    float altitude;
    float max_altitude;

    Drone(string name, float max_alt, float spd, float battery) : Vehicle(name, battery) {
        altitude = 0;
        max_altitude = max_alt;
        speed = spd;
    }

    virtual ~Drone() {}

    void take_off(float target_altitude) {
        if (target_altitude > max_altitude) {
            throw AltitudeError(name, target_altitude, max_altitude);
        }
        set_status("flying");
        altitude = target_altitude;
        log_event("Took off to " + to_string(altitude));
    }

    void land() {
        altitude = 0;
        set_status("idle");
        log_event("Landed");
    }

    void emergency_stop() {
        log_event("Emergency stop called draining 30 battery");
        drain_battery(30);
        land();
    }

    float get_altitude() {
        return altitude;
    }

    float get_speed() {
        return speed;
    }

    string get_info() {
        string info = "Drone name: " + name + "\n";
        info = info + "Status: " + get_status() + "\n";
        info = info + "Battery: " + to_string(get_battery()) + "\n";
        info = info + "Altitude: " + to_string(altitude) + "\n";
        info = info + "Max altitude: " + to_string(max_altitude) + "\n";
        info = info + "Speed: " + to_string(speed) + "\n";
        return info;
    }

private:
    float speed;
};
