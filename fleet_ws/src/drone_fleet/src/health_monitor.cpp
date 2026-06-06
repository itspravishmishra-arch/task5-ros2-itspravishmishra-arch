#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;

struct BatterySample {
    float battery;
    double timestamp;
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

class HealthMonitor : public rclcpp::Node {
public:
    HealthMonitor() : rclcpp::Node("health_monitor") {
        vector<string> drone_names;
        drone_names.push_back("Alpha");
        drone_names.push_back("Beta");
        drone_names.push_back("Gamma");

        for (int i = 0; i < (int)drone_names.size(); i++) {
            string n = drone_names[i];
            battery_samples[n] = deque<BatterySample>();

            auto sub = this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/telemetry", 10,
                [this, n](std_msgs::msg::String::SharedPtr msg) {
                    handle_telemetry(n, msg->data);
                }
            );
            telemetry_subs.push_back(sub);
        }

        warning_pub = this->create_publisher<std_msgs::msg::String>("/fleet/health_warning", 10);
        summary_pub = this->create_publisher<std_msgs::msg::String>("/fleet/health_summary", 10);

        diagnostics_timer = this->create_wall_timer(
            chrono::milliseconds(10000),
            bind(&HealthMonitor::print_diagnostics, this)
        );

        RCLCPP_INFO(this->get_logger(), "Health monitor online");
    }

private:
    map<string, deque<BatterySample>> battery_samples;
    vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr warning_pub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub;
    rclcpp::TimerBase::SharedPtr diagnostics_timer;

    void handle_telemetry(string drone_name, string json) {
        string battery_str = get_json_value(json, "battery");
        if (battery_str == "") {
            return;
        }

        float battery = stof(battery_str);
        double now = this->now().seconds();

        BatterySample sample;
        sample.battery = battery;
        sample.timestamp = now;

        deque<BatterySample>& samples = battery_samples[drone_name];
        samples.push_back(sample);
        if ((int)samples.size() > 10) {
            samples.pop_front();
        }

        float drain_rate = compute_drain_rate(drone_name);
        if (drain_rate > 1.5) {
            auto msg = std_msgs::msg::String();
            msg.data = "HIGH_DRAIN|drone:" + drone_name + "|rate:" + to_string(drain_rate);
            warning_pub->publish(msg);
            RCLCPP_WARN(this->get_logger(), "High drain rate on %s: %.2f per second", drone_name.c_str(), drain_rate);
        }
    }

    float compute_drain_rate(string drone_name) {
        deque<BatterySample>& samples = battery_samples[drone_name];
        if ((int)samples.size() < 2) {
            return 0.0;
        }
        BatterySample oldest = samples.front();
        BatterySample newest = samples.back();
        double time_diff = newest.timestamp - oldest.timestamp;
        if (time_diff <= 0) {
            return 0.0;
        }
        float battery_diff = oldest.battery - newest.battery;
        return battery_diff / (float)time_diff;
    }

    void print_diagnostics() {
        cout << "\n=== HEALTH DIAGNOSTICS ===" << endl;
        cout << "+----------+------------+------------------+------------------+" << endl;
        cout << "| Drone    | Drain/sec  | Time to Critical | Time to Depleted |" << endl;
        cout << "+----------+------------+------------------+------------------+" << endl;

        string summary_json = "{\"drones\":[";
        bool first = true;

        map<string, deque<BatterySample>>::iterator it;
        for (it = battery_samples.begin(); it != battery_samples.end(); it++) {
            string drone_name = it->first;
            deque<BatterySample>& samples = it->second;

            float current_battery = 0;
            if ((int)samples.size() > 0) {
                current_battery = samples.back().battery;
            }

            float drain_rate = compute_drain_rate(drone_name);

            string time_to_critical = "N/A";
            string time_to_depleted = "N/A";

            if (drain_rate > 0) {
                float secs_to_critical = (current_battery - 20.0) / drain_rate;
                float secs_to_depleted = current_battery / drain_rate;
                if (secs_to_critical < 0) secs_to_critical = 0;
                if (secs_to_depleted < 0) secs_to_depleted = 0;
                time_to_critical = to_string((int)secs_to_critical) + "s";
                time_to_depleted = to_string((int)secs_to_depleted) + "s";
            }

            cout << "| " << left << setw(8) << drone_name
                 << " | " << setw(10) << fixed << setprecision(3) << drain_rate
                 << " | " << setw(16) << time_to_critical
                 << " | " << setw(16) << time_to_depleted << " |" << endl;

            if (!first) {
                summary_json = summary_json + ",";
            }
            first = false;
            summary_json = summary_json + "{";
            summary_json = summary_json + "\"name\":\"" + drone_name + "\",";
            summary_json = summary_json + "\"drain_rate\":" + to_string(drain_rate) + ",";
            summary_json = summary_json + "\"battery\":" + to_string(current_battery) + ",";
            summary_json = summary_json + "\"time_to_critical\":\"" + time_to_critical + "\",";
            summary_json = summary_json + "\"time_to_depleted\":\"" + time_to_depleted + "\"";
            summary_json = summary_json + "}";
        }

        cout << "+----------+------------+------------------+------------------+" << endl;

        summary_json = summary_json + "]}";
        auto msg = std_msgs::msg::String();
        msg.data = summary_json;
        summary_pub->publish(msg);
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = make_shared<HealthMonitor>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
