#!/bin/bash
echo "launch roswrapper."
gnome-terminal --tab --title="Roswrapper_Core"  -- /bin/bash -c "sleep 5;export ROS_IP=172.17.0.1;export ROS_MASTER_URI=http://localhost:11311;source /opt/ros/noetic/setup.sh;source $PWD/ros/devel/setup.bash;roslaunch airsim_ros_pkgs airsim_node.launch host_ip:=$1; exec bash"