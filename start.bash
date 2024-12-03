#!/bin/bash
source /opt/ros/noetic/setup.bash
roscore &
/usr/local/LinuxNoEditor/RMUA.sh -RenderOffscreen seed $Seed