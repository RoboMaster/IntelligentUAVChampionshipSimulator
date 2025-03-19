# release note
## 20250319
>+ 修复IMU时间戳错误
## 20250227
>+ 增大工厂内仪表盘大小
>+ 修复工厂出入口动力丢失问题
## 20250224
>+ 修复工厂内gps不失效问题
## 20250219
>+ 增加复赛模拟器，包含工厂巡检任务、侧风以及风速计
## 20241209
>+ 修复中央枢纽空气墙问题  
>+ 修复雷达时间戳不稳定的问题  
## 20241211
>+ 修复中央枢纽出入口动力丢失问题  
>+ 性能优化，改善引擎卡顿导致的imu时钟错乱问题； 

# __自主无人机竞速模拟器使用说明__  
## 简介
    RMUA2025赛季模拟器

## 官方测试环境
> ros-noetic  
> ubuntu20.04  
> NVIDIA RTX3060TI gpu   
> INTEL I7 12th cpu  
### 注意：若使用神经网络，建议使用双显卡以保证模拟器性能

## 使用说明
## 1. 安装Nvidia-Docker  
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
## 2. 安装ROS-Noetic 
>+ `sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'`   
>+ `sudo apt install curl `  
>+ `curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -`   
>+ `sudo apt update`
>+ `sudo apt install ros-noetic-desktop-full`
>+ `sudo apt install python3-catkin-tools`

## 3. 使用模拟器
### 本机启动
>+ `cd /path/to/IntelligentUAVChampionshipSimulator`  
>+ 初赛模拟器：`wget https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/RMUA2024121102.zip`  
>+ 复赛模拟器：`wget https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/RMUA2024_11.0.1.7.zip`  
>+ `unzip RMUA2024121102.zip`  
>+ `mkdir ~/Documents/AirSim`  
>+ `cp settings.json ~/Documents/AirSim`   
>+ 渲染模式  `./run_simulator.sh 123`  
>+ 后台模式  `./run_simulator_offscreen.sh 123`     
注意：123 为随机种子参数，不同的种子对应不同的配置   
![pic](./docs/渲染模式.png) 
>+ 使用ros查看主题  
>+ `source /opt/ros/noetic/setup.bash`    
>+ `rostopic list`    

### Docker启动
>+ `cd /path/to/IntelligentUAVChampionshipSimulator` 
>+ `docker build -t simulator01 .`  
>+ `./run_docker_simulator.sh 123`  
注意：Docker仅支持后台模式运行,启动脚本后第一个参数 _123_ 是模拟器的随机种子，可根据需要修改     
>+ 使用ros查看主题    
>+ `source /opt/ros/noetic/setup.bash`     
>+ `rostopic list`    

## ros数据交互  
>用于获取数据的可订阅的主题  
>+ 前视相机   
`/airsim_node/drone_1/front_left/Scene`  
`/airsim_node/drone_1/front_right/Scene`
>+ 后视相机  
`/airsim_node/drone_1/back_left/Scene`  
`/airsim_node/drone_1/back_right/Scene`  
>+ imu数据  
`/airsim_node/drone_1/imu/imu`
>+ 雷达数据  
`/airsim_node/drone_1/lidar`
>+ 无人机状态真值  
`/airsim_node/drone_1/debug/pose_gt`  
>+ gps数据(含带误差姿态)  
`/airsim_node/drone_1/gps`
>+ 风速计(复赛内容)  
`airsim_node/drone_1/debug/wind`
>+ 电机输入PWM信号(0:右前, 1:左后, 2:左前, 3:右后)  
`/airsim_node/drone_1/debug/rotor_pwm`
>+ 起始位姿  
`/airsim_node/initial_pose`  
>+ 终点位置  
`/airsim_node/end_goal`
---- 
>用于发送指令的主题
>+ 速度控制  
`/airsim_node/drone_1/vel_cmd_body_frame`  
>+ PWM控制(0:右前, 1:左后, 2:左前, 3:右后)  
`/airsim_node/drone_1/rotor_pwm_cmd`
----
>可用服务
>+ 工厂巡检数据上报(复赛内容)  
index:(0 第一个工厂； 1 第二个工厂)  value:(仪表数值)  
`/airsim_node/meter_report`  
>+ 起飞   
`/airsim_node/drone_1/takeoff`   
>+ 降落   
`/airsim_node/drone_1/land`   
>+ 重置   
`/airsim_node/reset` 

## 系统相关参数
> 无人机系统参数  
>+ 质量 0.9kg    
>+ 轴距（电机至机体中心）0.18米  
>+ 转动惯量 Ixx 0.0046890742, Iyy 0.0069312, Izz 0.010421166  
>+ 电机升力系数 0.000367717  
>+ 电机反扭力系数 4.888486266072161e-06  
>+ 最大转速 11079.03 转每分钟
----
> 标定板参数
>+ 行数（内点）8  
>+ 列数（内点）11  
>+ 方块边长 0.06 米  


## Q&A

### 找不到数据类型
> 使用rqt_topic时发现一些数据类型缺失，需要source官方开发案例教程中basic_dev中的airsim_ros包。具体请参考: https://github.com/RoboMaster/IntelligentUAVChampionshipBase


### 帧率波动
> 当帧率波动严重时，可以更换更高性能的显卡。使用3090显卡进行测试，连续运行1个小时，模拟器帧率波动维持在0.5%以下。    
也可以关闭不需要的相机降低模拟器性能需求，提升帧率稳定性。
对于本机启动，仅需要把 _~/Documents/AirSim/settings.json_ 中相应相机配置删除即可关闭该相机。  
对于docker启动，需要把 _/path/to/IntelligentUAVChampionshipSimulator_ 中的 _settings.json_ 中相应相机配置删除后重新构建镜像即可。   
![pic](./docs/关闭相机.png)   

### 时钟同步
> 模拟器时钟与本地时钟存在一定差异，建议使用 IMU 主题传出的时间戳作为全局时钟进行程序设计。



