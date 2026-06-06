FROM osrf/ros:humble-desktop AS builder

RUN apt-get update && apt-get install -y \
    python3-colcon-common-extensions \
    python3-rosdep \
    ros-humble-std-srvs \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws

COPY src/ src/

RUN . /opt/ros/humble/setup.sh && \
    colcon build --packages-select drone_fleet --cmake-args -DCMAKE_BUILD_TYPE=Release

FROM osrf/ros:humble-ros-base AS runtime

RUN apt-get update && apt-get install -y \
    ros-humble-std-srvs \
    python3-launch-ros \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws

COPY --from=builder /ros2_ws/install/ install/

RUN echo '#!/bin/bash' > /entrypoint.sh && \
    echo 'source /opt/ros/humble/setup.bash' >> /entrypoint.sh && \
    echo 'source /ros2_ws/install/setup.bash' >> /entrypoint.sh && \
    echo 'exec "$@"' >> /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["ros2", "launch", "drone_fleet", "fleet.launch.py"]
