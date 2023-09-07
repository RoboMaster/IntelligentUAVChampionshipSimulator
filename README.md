# __自主无人机竞速模拟器使用说明__  
## 简介
    RMUA2023赛季综合赛模拟器
## 使用说明
1. ## 安装Nvidia-Docker  
>确保已安装 Nvidia 驱动  
----
>安装docker
>+ `sudo apt-get install ca-certificates gnupg lsb-release`
>+ `sudo mkdir -p /etc/apt/keyrings`
>+ `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
>+ `echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
>+ `sudo apt-get update`
>+ `sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin`
----
>安装nvidia-container-toolkit
>+ `distribution=$(. /etc/os-release;echo $ID$VERSION_ID)`
>+ `curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -`
>+ `curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list`
>+ `sudo apt-get update && sudo apt-get install -y nvidia-container-toolkit`
>+ `sudo systemctl restart docker`
---
>设置用户组，消除 *sudo* 限制  
>+ `sudo groupadd docker`  
>+ `sudo gpasswd -a $USER docker`  
>+ 注销账户并重新登录使新的用户组生效
>+ sudo service docker restart
2. ## 安装ROS-Noetic 
>+ `sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'`   
>+ `sudo apt install curl `  
>+ `curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -`   
>+ `sudo apt update`
>+ `sudo apt install ros-noetic-desktop-full`
>+ `sudo apt install python3-catkin-tools`

3. ## 使用模拟器
### 本机启动
>+ `cd /path/to/IntelligentUAVChampionshipSimulator`  
>+ `wget https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/uasim_student_230905_r3_shipping.zip`  
>+ `unzip uasim_student_230905_r3_shipping.zip`  
>+ `mkdir ~/Documents/AirSim`  
>+ `cp settings.json ~/Documents/AirSim`   
>+ 渲染模式  `./run_simulator.sh`  
>+ 后台模式  `./run_simulator_offscreen.sh`     
注意：脚本中的 _-seed_ 参数为模拟器的随机种子，可根据需要修改   
![pic](./docs/渲染模式.png) 
>+ 使用ros查看主题  
>+ `source /opt/ros/noetic/setup.bash`    
>+ `rostopic list`    
![pic](./docs/topic.png)   

### Docker启动
>+ `cd /path/to/IntelligentUAVChampionshipSimulator` 
>+ `docker build -t simulator .`  
>+ `./run_docker_simulator.sh 123`  
注意：Docker仅支持后台模式运行,启动脚本后第一个参数 _123_ 是模拟器的随机种子，可根据需要修改    
>+ 使用ros查看主题 `source /opt/ros/noetic/setup.bash`    
>+ `rostopic list`    
![pic](./docs/topic.png)  

## ros数据交互
![pic](./docs/5.png)   
>用于获取数据的可订阅的主题  
>+ 下视相机   
`/airsim_node/drone_1/bottom_center/Scene`  
>+ 双目左rgb图  
`/airsim_node/drone_1/front_left/Scene`
>+ 双目右rgb图    
`/airsim_node/drone_1/front_right/Scene`
>+ imu数据  
`/airsim_node/drone_1/imu/imu`
>+ 无人机状态真值  
`/airsim_node/drone_1/debug/pose_gt`
>+ gps数据  
`/airsim_node/drone_1/pose`
>+ 障碍圈位姿真值  
`/airsim_node/drone_1/debug/circle_poses_gt`  
>+ 障碍圈参考位姿    
`/airsim_node/drone_1/circle_poses`  
>+ 赛道中生成的树的真实位置  
`/airsim_node/drone_1/debug/tree_poses_gt`
>+ 电机输入PWM信号(0:右前, 1:左后, 2:左前, 3:右后)  
`/airsim_node/drone_1/rotor_pwm`  
----
>用于发送指令的主题
>+ 姿态控制  
`/airsim_node/drone_1/pose_cmd_body_frame` 
>+ 速度控制   
`/airsim_node/drone_1/vel_cmd_body_frame`
>+ 角速度推力控制  
`/airsim_node/drone_1/angle_rate_throttle_frame`
----
>可用服务   
>+ 起飞   
`/airsim_node/drone_1/takeoff`   
>+ 降落   
`/airsim_node/drone_1/land`   
>+ 重置   
`/airsim_node/reset` 
### 注意:   
服务器仅开放 _下视相机_, _双目左rgb图_, _双目右rgb图_, _gps数据_, _障碍圈参考位姿_, _imu数据_， 规则手册中未提及的话题(_无人机状态真值_, _障碍圈位姿真值_, _赛道中生成的树的真实位置_, _电机输入PWM信号_)仅供调试程序使用。



