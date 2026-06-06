#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "drone_exceptions.hpp"

using namespace std;

class Vehicle {
public:
    string name;

    Vehicle(string n, float battery) {
        name = n;
        battery_level = battery;
        status = "idle";
        flight_log.push_back("Vehicle " + name + " created with battery " + to_string(battery));
    }

    virtual ~Vehicle() {}

    virtual string get_info() = 0;

    void drain_battery(float amount) {
        if (battery_level <= 0) {
            throw BatteryDepletedError(name);
        }
        battery_level = battery_level - amount;
        if (battery_level < 0) {
            battery_level = 0;
        }
        flight_log.push_back("Battery drained by " + to_string(amount) + " now at " + to_string(battery_level));
    }

    void charge_battery(float amount, int duration_seconds) {
        if (status != "charging") {
            throw InvalidStateError(name, status, "charging");
        }
        battery_level = battery_level + amount;
        if (battery_level > 100) {
            battery_level = 100;
        }
        flight_log.push_back("Battery charged for " + to_string(duration_seconds) + " seconds now at " + to_string(battery_level));
    }

    bool is_critical() {
        if (battery_level < 20) {
            return true;
        }
        return false;
    }

    void start_charging() {
        set_status("charging");
    }

    void stop_charging() {
        set_status("idle");
    }

    string get_flight_log() {
        string log = "Flight log for " + name + "\n";
        for (int i = 0; i < flight_log.size(); i++) {
            log = log + flight_log[i] + "\n";
        }
        return log;
    }

    float get_battery() {
        return battery_level;
    }

    string get_status() {
        return status;
    }

    string get_name() {
        return name;
    }

protected:
    void set_status(string new_status) {
        if (new_status != "idle" && new_status != "flying" && new_status != "charging") {
            throw InvalidStateError(name, status, new_status);
        }
        string old_status = status;
        status = new_status;
        flight_log.push_back("Status changed from " + old_status + " to " + status);
    }

    void log_event(string event) {
        flight_log.push_back(event);
    }

private:
    float battery_level;
    string status;
    vector<string> flight_log;
};
