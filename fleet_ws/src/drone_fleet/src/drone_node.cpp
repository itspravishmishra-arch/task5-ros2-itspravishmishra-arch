#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <string>
#include <vector>
#include <tuple>

#include "drone_fleet/mission_drone.hpp"

using namespace std;

class DroneNode : public rclcpp::Node {
public:
    DroneNode() : rclcpp::Node("drone_node") {
        this->declare_parameter("drone_name", string("Alpha"));
        this->declare_parameter("initial_battery", 100.0);
        this->declare_parameter("mission_name", string("DefaultMission"));

        drone_name = this->get_parameter("drone_name").as_string();
        double init_battery = this->get_parameter("initial_battery").as_double();
        string mission = this->get_parameter("mission_name").as_string();

        waypoints.push_back(make_tuple(0.0f, 0.0f, 10.0f));
        waypoints.push_back(make_tuple(10.0f, 0.0f, 15.0f));
        waypoints.push_back(make_tuple(10.0f, 10.0f, 20.0f));
        waypoints.push_back(make_tuple(0.0f, 10.0f, 15.0f));
        waypoints.push_back(make_tuple(0.0f, 0.0f, 10.0f));

        drone = new MissionDrone(drone_name, mission, waypoints, 120, 15, (float)init_battery);
        drone->take_off(10);

        string status_topic = "/drone/" + drone_name + "/status";
        string alert_topic = "/drone/" + drone_name + "/alert";
        string complete_topic = "/drone/" + drone_name + "/mission_complete";
        string telemetry_topic = "/drone/" + drone_name + "/telemetry";

        status_pub = this->create_publisher<std_msgs::msg::String>(status_topic, 10);
        alert_pub = this->create_publisher<std_msgs::msg::String>(alert_topic, 10);
        complete_pub = this->create_publisher<std_msgs::msg::String>(complete_topic, 10);
        telemetry_pub = this->create_publisher<std_msgs::msg::String>(telemetry_topic, 10);

        status_timer = this->create_wall_timer(
            chrono::milliseconds(1000),
            bind(&DroneNode::publish_status, this)
        );
        telemetry_timer = this->create_wall_timer(
            chrono::milliseconds(2000),
            bind(&DroneNode::publish_telemetry, this)
        );

        publish_count = 0;

        RCLCPP_INFO(this->get_logger(), "Drone %s online battery=%.1f", drone_name.c_str(), (float)init_battery);
    }

    ~DroneNode() {
        delete drone;
    }

private:
    string drone_name;
    MissionDrone* drone;
    vector<tuple<float, float, float>> waypoints;
    int publish_count;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr complete_pub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub;
    rclcpp::TimerBase::SharedPtr status_timer;
    rclcpp::TimerBase::SharedPtr telemetry_timer;

    void publish_status() {
        if (drone->get_battery() <= 0) {
            return;
        }

        try {
            drone->drain_battery(0.5);
        } catch (BatteryDepletedError e) {
            auto alert_msg = std_msgs::msg::String();
            alert_msg.data = "BATTERY_DEPLETED|drone:" + drone_name;
            alert_pub->publish(alert_msg);
            return;
        }

        publish_count = publish_count + 1;

        if (publish_count % 3 == 0) {
            if (!drone->mission_complete()) {
                try {
                    drone->next_waypoint();
                } catch (exception e) {
                    RCLCPP_WARN(this->get_logger(), "Waypoint advance error: %s", e.what());
                }
            }
        }

        if (drone->mission_complete()) {
            auto complete_msg = std_msgs::msg::String();
            complete_msg.data = "MISSION_COMPLETE|drone:" + drone_name + "|mission:" + drone->mission_name;
            complete_pub->publish(complete_msg);
            RCLCPP_INFO(this->get_logger(), "%s mission complete restarting", drone_name.c_str());
            drone->current_waypoint_index = 0;
        }

        if (drone->is_critical()) {
            auto alert_msg = std_msgs::msg::String();
            alert_msg.data = "BATTERY_CRITICAL|drone:" + drone_name + "|battery:" + to_string(drone->get_battery());
            alert_pub->publish(alert_msg);
            if (drone->get_status() == "flying") {
                drone->land();
                RCLCPP_WARN(this->get_logger(), "%s battery critical landing", drone_name.c_str());
            }
        }

        int total = (int)drone->waypoints.size();
        int current = drone->current_waypoint_index;
        if (current > total) {
            current = total;
        }

        string msg_data = "name:" + drone_name
            + "|battery:" + to_string(drone->get_battery())
            + "|altitude:" + to_string(drone->get_altitude())
            + "|status:" + drone->get_status()
            + "|waypoint:" + to_string(current) + "/" + to_string(total)
            + "|speed:" + to_string(drone->get_speed());

        auto msg = std_msgs::msg::String();
        msg.data = msg_data;
        status_pub->publish(msg);
    }

    void publish_telemetry() {
        int total = (int)drone->waypoints.size();
        int current = drone->current_waypoint_index;
        if (current > total) {
            current = total;
        }

        string json = "{";
        json = json + "\"name\":\"" + drone_name + "\",";
        json = json + "\"battery\":" + to_string(drone->get_battery()) + ",";
        json = json + "\"altitude\":" + to_string(drone->get_altitude()) + ",";
        json = json + "\"status\":\"" + drone->get_status() + "\",";
        json = json + "\"waypoint_current\":" + to_string(current) + ",";
        json = json + "\"waypoint_total\":" + to_string(total) + ",";
        json = json + "\"speed\":" + to_string(drone->get_speed()) + ",";
        json = json + "\"mission\":\"" + drone->mission_name + "\",";
        json = json + "\"is_critical\":" + (drone->is_critical() ? "true" : "false");
        json = json + "}";

        auto msg = std_msgs::msg::String();
        msg.data = json;
        telemetry_pub->publish(msg);
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = make_shared<DroneNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
