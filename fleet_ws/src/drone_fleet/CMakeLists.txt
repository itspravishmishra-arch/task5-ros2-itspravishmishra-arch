cmake_minimum_required(VERSION 3.16)
project(drone_fleet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-sign-compare -Wno-catch-value -Wno-unused-parameter)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(include)

add_executable(drone_node src/drone_node.cpp)
ament_target_dependencies(drone_node rclcpp std_msgs)

add_executable(fleet_manager src/fleet_manager.cpp)
ament_target_dependencies(fleet_manager rclcpp std_msgs std_srvs)

add_executable(health_monitor src/health_monitor.cpp)
ament_target_dependencies(health_monitor rclcpp std_msgs)

install(TARGETS
  drone_node
  fleet_manager
  health_monitor
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

ament_package()
