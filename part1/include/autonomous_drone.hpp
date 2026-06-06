#pragma once
#include "mission_drone.hpp"
#include <cmath>

using namespace std;

class AutonomousDrone : public MissionDrone {
public:
    string ai_mode;
    tuple<float, float, float> home_position;

    AutonomousDrone(string name, string mission, vector<tuple<float, float, float>> wps, tuple<float, float, float> home, float max_alt, float spd, float battery) : MissionDrone(name, mission, wps, max_alt, spd, battery) {
        ai_mode = "manual";
        home_position = home;
    }

    virtual ~AutonomousDrone() {}

    void set_ai_mode(string mode) {
        if (mode != "manual" && mode != "auto" && mode != "return_home") {
            throw invalid_argument("Mode must be manual auto or return_home");
        }
        ai_mode = mode;
        log_event("AI mode changed to " + mode);
        if (mode == "return_home") {
            waypoints.insert(waypoints.begin() + current_waypoint_index, home_position);
            log_event("Inserted home position as next waypoint");
        }
    }

    void detect_obstacle(tuple<float, float, float> position, string severity) {
        float x = get<0>(position);
        float y = get<1>(position);
        float z = get<2>(position);
        string entry = "Obstacle at x=" + to_string(x) + " y=" + to_string(y) + " z=" + to_string(z) + " severity=" + severity;
        obstacle_log.push_back(entry);
        log_event(entry);
        if (severity == "high") {
            log_event("High severity obstacle calling emergency stop");
            emergency_stop();
        }
    }

    vector<tuple<float, float, float>> auto_replan(vector<tuple<float, float, float>> obstacles) {
        vector<tuple<float, float, float>> safe_waypoints;
        for (int i = 0; i < waypoints.size(); i++) {
            bool blocked = false;
            for (int j = 0; j < obstacles.size(); j++) {
                float dx = get<0>(waypoints[i]) - get<0>(obstacles[j]);
                float dy = get<1>(waypoints[i]) - get<1>(obstacles[j]);
                float dz = get<2>(waypoints[i]) - get<2>(obstacles[j]);
                float dist = sqrt(dx*dx + dy*dy + dz*dz);
                if (dist < 5) {
                    blocked = true;
                }
            }
            if (blocked == false) {
                safe_waypoints.push_back(waypoints[i]);
            }
        }
        return safe_waypoints;
    }

    vector<string> get_obstacle_log() {
        return obstacle_log;
    }

    string get_info() {
        string info = "AutonomousDrone name: " + name + "\n";
        info = info + "Mission: " + mission_name + "\n";
        info = info + "AI mode: " + ai_mode + "\n";
        info = info + "Status: " + get_status() + "\n";
        info = info + "Battery: " + to_string(get_battery()) + "\n";
        info = info + "Altitude: " + to_string(altitude) + "\n";
        float hx = get<0>(home_position);
        float hy = get<1>(home_position);
        float hz = get<2>(home_position);
        info = info + "Home: x=" + to_string(hx) + " y=" + to_string(hy) + " z=" + to_string(hz) + "\n";
        info = info + "Waypoints done: " + to_string(current_waypoint_index) + " out of " + to_string(waypoints.size()) + "\n";
        info = info + "Obstacles detected: " + to_string(obstacle_log.size()) + "\n";
        return info;
    }

private:
    vector<string> obstacle_log;
};
