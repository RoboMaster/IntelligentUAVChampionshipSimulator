#include "ros/ros.h"
#include "airsim_ros_wrapper.h"
#include <ros/spinner.h>
#include <signal.h>


int main(int argc, char** argv)
{
    ros::init(argc, argv, "airsim_node");
    ros::NodeHandle nh;
    ros::NodeHandle nh_private("~");

    std::string host_ip = "localhost";
    nh_private.getParam("host_ip", host_ip);
    AirsimROSWrapper airsim_ros_wrapper(nh, nh_private, host_ip);
    airsim_ros_wrapper.drone_state_async_spinner_.start();
    airsim_ros_wrapper.update_commands_async_spinner_.start();
    airsim_ros_wrapper.command_listener_async_spinner_.start();
    if (airsim_ros_wrapper.is_used_img_timer_cb_queue_) {
        if(airsim_ros_wrapper.is_RGBD_)
        {
            // ROS_ERROR("rgbd spinner");
            airsim_ros_wrapper.img_RGBD_async_spinner_.start();
        }
        if(airsim_ros_wrapper.is_stereo_)
        {
            // ROS_ERROR("stereo spinner");
            airsim_ros_wrapper.img_stereo_async_spinner_.start();
        }
        airsim_ros_wrapper.img_bottom_async_spinner_.start();
    }

    if (airsim_ros_wrapper.is_used_lidar_timer_cb_queue_) {
        airsim_ros_wrapper.lidar_async_spinner_.start();
    }

    ros::waitForShutdown();
    return 0;
}