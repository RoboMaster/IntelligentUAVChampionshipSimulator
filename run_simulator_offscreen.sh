#!/bin/bash
source /opt/ros/noetic/setup.bash
roscore &
./Build/LinuxNoEditor/RMUA.sh -seed=123 -RenderOffscreen