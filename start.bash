#!/bin/bash
source /opt/ros/noetic/setup.bash
roscore &
if [ "$RenderOffScreen" -eq "0" ]
then
    /usr/local/LinuxNoEditor/RMUA.sh -seed=$Seed
else 
    /usr/local/LinuxNoEditor/RMUA.sh -RenderOffscreen -seed=$Seed
fi