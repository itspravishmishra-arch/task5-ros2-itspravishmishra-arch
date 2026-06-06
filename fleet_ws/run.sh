#!/bin/bash

docker run -it --rm \
    --net=host \
    -e ROS_DOMAIN_ID=42 \
    -v "$(pwd)/src:/ros2_ws/src" \
    drone_fleet:latest
