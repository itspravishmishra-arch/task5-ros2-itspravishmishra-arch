#pragma once
#include "drone.hpp"
#include <tuple>
#include <vector>

using namespace std;

class MissionDrone : public Drone {
public:
    string mission_name;
    vector<tuple<float, float, float>> waypoints;
    int current_waypoint_index;

    MissionDrone(string name, string mission, vector<tuple<float, float, float>> wps, float max_alt, float spd, float battery) : Drone(name, max_alt, spd, battery) {
        mission_name = mission;
        waypoints = wps;
        current_waypoint_index = 0;
    }

    virtual ~MissionDrone() {}

    tuple<float, float, float> next_waypoint() {
        if (mission_complete()) {
            throw out_of_range("No more waypoints mission is done");
        }
        drain_battery(1.5);
        tuple<float, float, float> wp = waypoints[current_waypoint_index];
        visited_waypoints.push_back(make_pair(wp, "visited"));
        log_event("Reached waypoint " + to_string(current_waypoint_index));
        current_waypoint_index = current_waypoint_index + 1;
        return wp;
    }

    void skip_waypoint(string reason) {
        if (mission_complete()) {
            return;
        }
        tuple<float, float, float> wp = waypoints[current_waypoint_index];
        visited_waypoints.push_back(make_pair(wp, "skipped because " + reason));
        log_event("Skipped waypoint " + to_string(current_waypoint_index) + " reason: " + reason);
        current_waypoint_index = current_waypoint_index + 1;
    }

    bool mission_complete() {
        if (current_waypoint_index >= waypoints.size()) {
            return true;
        }
        return false;
    }

    string mission_summary() {
        string summary = "Mission name: " + mission_name + "\n";
        summary = summary + "Total waypoints: " + to_string(waypoints.size()) + "\n";
        summary = summary + "Completed: " + to_string(current_waypoint_index) + "\n";
        summary = summary + "Battery left: " + to_string(get_battery()) + "\n";
        summary = summary + "Visited waypoints:\n";
        for (int i = 0; i < visited_waypoints.size(); i++) {
            float x = get<0>(visited_waypoints[i].first);
            float y = get<1>(visited_waypoints[i].first);
            float z = get<2>(visited_waypoints[i].first);
            summary = summary + "  " + to_string(i) + " x=" + to_string(x) + " y=" + to_string(y) + " z=" + to_string(z) + " status=" + visited_waypoints[i].second + "\n";
        }
        return summary;
    }

    string get_info() {
        string info = "MissionDrone name: " + name + "\n";
        info = info + "Mission: " + mission_name + "\n";
        info = info + "Status: " + get_status() + "\n";
        info = info + "Battery: " + to_string(get_battery()) + "\n";
        info = info + "Altitude: " + to_string(altitude) + "\n";
        info = info + "Waypoints done: " + to_string(current_waypoint_index) + " out of " + to_string(waypoints.size()) + "\n";
        return info;
    }

private:
    vector<pair<tuple<float, float, float>, string>> visited_waypoints;
};
