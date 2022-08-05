#!/bin/bash
echo "launch simulator."
gnome-terminal --tab --title="Simulator" -- /bin/bash -c "/bin/bash ./FPV1.sh -WINDOWED -ResX=640 -ResY=480" \
echo "launch roswrapper."
gnome-terminal --tab --title="Roswrapper_Core"  -- /bin/bash -c "sleep 5;export ROS_IP=172.17.0.1;export ROS_MASTER_URI=http://localhost:11311;source /opt/ros/noetic/setup.sh;source $PWD/roswrapper/ros/devel/setup.bash;roslaunch airsim_ros_pkgs airsim_node.launch;exec bash"