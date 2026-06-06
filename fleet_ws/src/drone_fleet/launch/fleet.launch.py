from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    alpha = Node(
        package="drone_fleet",
        executable="drone_node",
        name="alpha_drone",
        parameters=[
            {"drone_name": "Alpha"},
            {"initial_battery": 100.0},
            {"mission_name": "AlphaMission"}
        ],
        output="screen"
    )

    beta = Node(
        package="drone_fleet",
        executable="drone_node",
        name="beta_drone",
        parameters=[
            {"drone_name": "Beta"},
            {"initial_battery": 60.0},
            {"mission_name": "BetaMission"}
        ],
        output="screen"
    )

    gamma = Node(
        package="drone_fleet",
        executable="drone_node",
        name="gamma_drone",
        parameters=[
            {"drone_name": "Gamma"},
            {"initial_battery": 35.0},
            {"mission_name": "GammaMission"}
        ],
        output="screen"
    )

    fleet_manager = Node(
        package="drone_fleet",
        executable="fleet_manager",
        name="fleet_manager",
        output="screen"
    )

    health_monitor = Node(
        package="drone_fleet",
        executable="health_monitor",
        name="health_monitor",
        output="screen"
    )

    return LaunchDescription([
        alpha,
        beta,
        gamma,
        fleet_manager,
        health_monitor
    ])
