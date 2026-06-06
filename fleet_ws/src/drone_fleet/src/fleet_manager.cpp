#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

using namespace std;

struct DroneState {
    string name;
    float battery;
    float altitude;
    string status;
    int waypoint_current;
    int waypoint_total;
    float speed;
    string mission;
    bool is_critical;

    DroneState() {
        name = "unknown";
        battery = 0;
        altitude = 0;
        status = "unknown";
        waypoint_current = 0;
        waypoint_total = 0;
        speed = 0;
        mission = "unknown";
        is_critical = false;
    }
};

string get_json_value(string json, string key) {
    string search_key = "\"" + key + "\":";
    int pos = json.find(search_key);
    if (pos == (int)string::npos) {
        return "";
    }
    pos = pos + search_key.size();
    if (json[pos] == '"') {
        pos = pos + 1;
        int end = json.find('"', pos);
        return json.substr(pos, end - pos);
    } else {
        int end = pos;
        while (end < (int)json.size() && json[end] != ',' && json[end] != '}') {
            end = end + 1;
        }
        return json.substr(pos, end - pos);
    }
}

class FleetManager : public rclcpp::Node {
public:
    FleetManager() : rclcpp::Node("fleet_manager") {
        vector<string> drone_names;
        drone_names.push_back("Alpha");
        drone_names.push_back("Beta");
        drone_names.push_back("Gamma");

        for (int i = 0; i < (int)drone_names.size(); i++) {
            string n = drone_names[i];
            fleet_state[n] = DroneState();
            fleet_state[n].name = n;

            auto status_sub = this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/status", 10,
                [this, n](std_msgs::msg::String::SharedPtr msg) {
                    handle_status(n, msg->data);
                }
            );
            status_subs.push_back(status_sub);

            auto alert_sub = this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/alert", 10,
                [this, n](std_msgs::msg::String::SharedPtr msg) {
                    handle_alert(n, msg->data);
                }
            );
            alert_subs.push_back(alert_sub);

            auto complete_sub = this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/mission_complete", 10,
                [this, n](std_msgs::msg::String::SharedPtr msg) {
                    handle_mission_complete(n, msg->data);
                }
            );
            complete_subs.push_back(complete_sub);

            auto telemetry_sub = this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/telemetry", 10,
                [this, n](std_msgs::msg::String::SharedPtr msg) {
                    handle_telemetry(n, msg->data);
                }
            );
            telemetry_subs.push_back(telemetry_sub);
        }

        report_timer = this->create_wall_timer(
            chrono::milliseconds(5000),
            bind(&FleetManager::print_fleet_report, this)
        );

        status_service = this->create_service<std_srvs::srv::Trigger>(
            "/fleet/status_report",
            bind(&FleetManager::handle_status_service, this, placeholders::_1, placeholders::_2)
        );

        RCLCPP_INFO(this->get_logger(), "Fleet manager online monitoring Alpha Beta Gamma");
    }

private:
    map<string, DroneState> fleet_state;
    vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> status_subs;
    vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> alert_subs;
    vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> complete_subs;
    vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs;
    rclcpp::TimerBase::SharedPtr report_timer;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr status_service;

    void handle_status(string drone_name, string data) {
        (void)drone_name;
        (void)data;
    }

    void handle_alert(string drone_name, string data) {
        auto now = this->now();
        RCLCPP_WARN(this->get_logger(), "[%.3f] ALERT from %s: %s", now.seconds(), drone_name.c_str(), data.c_str());
    }

    void handle_mission_complete(string drone_name, string data) {
        RCLCPP_INFO(this->get_logger(), "Mission complete from %s: %s", drone_name.c_str(), data.c_str());
    }

    void handle_telemetry(string drone_name, string json) {
        DroneState state;
        state.name = drone_name;

        string battery_str = get_json_value(json, "battery");
        string altitude_str = get_json_value(json, "altitude");
        string status_str = get_json_value(json, "status");
        string wp_current_str = get_json_value(json, "waypoint_current");
        string wp_total_str = get_json_value(json, "waypoint_total");
        string speed_str = get_json_value(json, "speed");
        string mission_str = get_json_value(json, "mission");
        string critical_str = get_json_value(json, "is_critical");

        if (battery_str != "") state.battery = stof(battery_str);
        if (altitude_str != "") state.altitude = stof(altitude_str);
        if (status_str != "") state.status = status_str;
        if (wp_current_str != "") state.waypoint_current = stoi(wp_current_str);
        if (wp_total_str != "") state.waypoint_total = stoi(wp_total_str);
        if (speed_str != "") state.speed = stof(speed_str);
        if (mission_str != "") state.mission = mission_str;
        if (critical_str == "true") state.is_critical = true;

        fleet_state[drone_name] = state;
    }

    void print_fleet_report() {
        cout << "\n+----------+---------+----------+----------+--------+" << endl;
        cout << "| Drone    | Battery | Altitude | Waypoint | Status |" << endl;
        cout << "+----------+---------+----------+----------+--------+" << endl;

        map<string, DroneState>::iterator it;
        for (it = fleet_state.begin(); it != fleet_state.end(); it++) {
            DroneState s = it->second;
            string warn = s.is_critical ? " !!" : "   ";
            cout << "| " << left << setw(8) << s.name
                 << " | " << setw(7) << fixed << setprecision(1) << s.battery
                 << " | " << setw(8) << s.altitude
                 << " | " << setw(2) << s.waypoint_current << "/" << setw(5) << s.waypoint_total
                 << " | " << setw(6) << s.status << "|" << warn << endl;
        }
        cout << "+----------+---------+----------+----------+--------+" << endl;
    }

    void handle_status_service(
        std_srvs::srv::Trigger::Request::SharedPtr request,
        std_srvs::srv::Trigger::Response::SharedPtr response)
    {
        (void)request;
        print_fleet_report();
        response->success = true;
        response->message = "Fleet report printed to console";
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = make_shared<FleetManager>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
