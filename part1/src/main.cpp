#include <iostream>
#include <vector>
#include "vehicle.hpp"
#include "drone.hpp"
#include "mission_drone.hpp"
#include "autonomous_drone.hpp"
#include "drone_exceptions.hpp"

using namespace std;

int main() {
    cout << "Creating drones..." << endl;

    Drone* alpha = new Drone("Alpha", 100, 15, 85);

    vector<tuple<float, float, float>> beta_waypoints;
    beta_waypoints.push_back(make_tuple(10, 5, 20));
    beta_waypoints.push_back(make_tuple(30, 10, 25));
    beta_waypoints.push_back(make_tuple(50, 0, 15));
    MissionDrone* beta = new MissionDrone("Beta", "SurveyRun", beta_waypoints, 120, 12, 90);

    vector<tuple<float, float, float>> gamma_waypoints;
    gamma_waypoints.push_back(make_tuple(5, 5, 30));
    gamma_waypoints.push_back(make_tuple(15, 10, 40));
    gamma_waypoints.push_back(make_tuple(25, 15, 50));
    gamma_waypoints.push_back(make_tuple(35, 20, 60));
    gamma_waypoints.push_back(make_tuple(45, 25, 55));
    tuple<float, float, float> home = make_tuple(0, 0, 0);
    AutonomousDrone* gamma = new AutonomousDrone("Gamma", "PatrolMission", gamma_waypoints, home, 150, 20, 100);

    cout << endl;
    cout << "--- Polymorphism Demo ---" << endl;

    vector<Vehicle*> fleet;
    fleet.push_back(alpha);
    fleet.push_back(beta);
    fleet.push_back(gamma);

    for (int i = 0; i < fleet.size(); i++) {
        cout << fleet[i]->get_info() << endl;
    }

    cout << "--- Private member access demo ---" << endl;
    cout << "Getting battery through getter: " << alpha->get_battery() << endl;
    cout << "Getting status through getter: " << alpha->get_status() << endl;
    // alpha->battery_level = 50;  this would not compile because battery_level is private
    // alpha->status = "flying";   this would not compile because status is private
    cout << endl;

    cout << "--- drain_battery and BatteryDepletedError demo ---" << endl;
    Drone* temp = new Drone("TempDrone", 80, 10, 5);
    try {
        temp->drain_battery(5);
        cout << "Drained 5 battery ok" << endl;
        temp->drain_battery(1);
    } catch (BatteryDepletedError e) {
        cout << "Caught BatteryDepletedError: " << e.what() << endl;
    } catch (DroneException e) {
        cout << "Caught DroneException: " << e.what() << endl;
    }
    cout << endl;

    cout << "--- take_off and AltitudeError demo ---" << endl;
    try {
        alpha->take_off(90);
        cout << "Alpha took off to 90" << endl;
        alpha->take_off(200);
    } catch (AltitudeError e) {
        cout << "Caught AltitudeError: " << e.what() << endl;
    } catch (DroneException e) {
        cout << "Caught DroneException: " << e.what() << endl;
    }
    cout << endl;

    cout << "--- charge_battery and InvalidStateError demo ---" << endl;
    try {
        alpha->charge_battery(10, 60);
    } catch (InvalidStateError e) {
        cout << "Caught InvalidStateError: " << e.what() << endl;
    } catch (DroneException e) {
        cout << "Caught DroneException: " << e.what() << endl;
    }

    alpha->land();
    alpha->start_charging();
    try {
        alpha->charge_battery(15, 120);
        cout << "Charged alpha successfully battery is now " << alpha->get_battery() << endl;
    } catch (DroneException e) {
        cout << "Caught DroneException: " << e.what() << endl;
    }
    alpha->stop_charging();
    cout << endl;

    cout << "--- detect_obstacle low severity demo ---" << endl;
    gamma->take_off(40);
    cout << "Gamma took off to 40" << endl;
    gamma->detect_obstacle(make_tuple(100, 200, 50), "low");
    cout << "Low obstacle logged gamma still flying" << endl;
    cout << endl;

    cout << "--- Full AutonomousDrone mission ---" << endl;

    vector<tuple<float, float, float>> mission_waypoints;
    mission_waypoints.push_back(make_tuple(5, 5, 30));
    mission_waypoints.push_back(make_tuple(15, 10, 40));
    mission_waypoints.push_back(make_tuple(25, 15, 50));
    mission_waypoints.push_back(make_tuple(35, 20, 60));
    mission_waypoints.push_back(make_tuple(45, 25, 55));
    tuple<float, float, float> m1_home = make_tuple(0, 0, 0);
    AutonomousDrone* m1 = new AutonomousDrone("Gamma-M1", "FullPatrol", mission_waypoints, m1_home, 150, 20, 100);

    m1->set_ai_mode("auto");
    m1->take_off(30);
    cout << "Mission drone took off" << endl;

    int hop = 0;
    while (m1->mission_complete() == false) {
        if (hop == 2) {
            cout << "Injecting high severity obstacle at hop 2" << endl;
            try {
                m1->detect_obstacle(make_tuple(25, 15, 50), "high");
            } catch (BatteryDepletedError e) {
                cout << "Caught BatteryDepletedError during emergency stop: " << e.what() << endl;
                break;
            }

            vector<tuple<float, float, float>> obstacles;
            obstacles.push_back(make_tuple(25, 15, 50));
            vector<tuple<float, float, float>> safe = m1->auto_replan(obstacles);
            cout << "Safe waypoints after replan: " << safe.size() << endl;

            if (m1->get_battery() > 0 && m1->get_status() != "flying") {
                m1->take_off(30);
                cout << "Relaunched after emergency stop" << endl;
            }
        }

        try {
            tuple<float, float, float> wp = m1->next_waypoint();
            float x = get<0>(wp);
            float y = get<1>(wp);
            float z = get<2>(wp);
            cout << "Waypoint " << hop << " reached x=" << x << " y=" << y << " z=" << z << " battery=" << m1->get_battery() << endl;
        } catch (BatteryDepletedError e) {
            cout << "Caught BatteryDepletedError: " << e.what() << endl;
            break;
        } catch (exception e) {
            cout << "Caught exception: " << e.what() << endl;
            break;
        }
        hop = hop + 1;
    }

    if (m1->get_status() == "flying") {
        m1->land();
        cout << "Mission drone landed" << endl;
    }

    cout << endl;
    cout << m1->mission_summary() << endl;

    cout << "--- Flight log ---" << endl;
    cout << m1->get_flight_log() << endl;

    cout << "--- Battery critical check ---" << endl;
    for (int i = 0; i < fleet.size(); i++) {
        cout << fleet[i]->get_name() << " battery=" << fleet[i]->get_battery();
        if (fleet[i]->is_critical()) {
            cout << " CRITICAL";
        } else {
            cout << " ok";
        }
        cout << endl;
    }
    cout << "Gamma-M1 battery=" << m1->get_battery();
    if (m1->is_critical()) {
        cout << " CRITICAL";
    } else {
        cout << " ok";
    }
    cout << endl;

    delete alpha;
    delete beta;
    delete gamma;
    delete temp;
    delete m1;

    return 0;
}
