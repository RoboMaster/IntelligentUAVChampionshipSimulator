# __自主无人机竞速模拟器使用说明__  
## 简介
    RMUA2026赛季模拟器

## 官方测试环境
> ros-noetic  
> ubuntu20.04  
> NVIDIA RTX3090TI gpu   
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
>+ 模拟器：`wget https://sz-rm-rmua-dispatch-prod.oss-cn-shenzhen.aliyuncs.com/b39a0194e982f0d987153c6016feb325/simulator_12.0.0.5.zip`  
>+ `unzip simulator_12.0.0.5.zip`   
>+ 将此GitHub工程的所有文件下载到模拟器解压后的build文件夹同级目录中（注：不是放入build文件夹中） 
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
>+ 风速计  
`airsim_node/drone_1/debug/wind`
>+ 电机输入PWM信号(0:右前, 1:左后, 2:左前, 3:右后)  
`/airsim_node/drone_1/debug/rotor_pwm`
>+ 起始位姿  
`/airsim_node/initial_pose`  
>+ 终点位置  
`/airsim_node/end_goal`
---- 
>用于发送指令的主题
>+ 速度控制(0:x轴速度, 1:y轴速度, 2:z轴速度, 3:角速度，4:加速度(上限为8m/s2),5:是否急停(1表示急停))  
`/airsim_node/drone_1/vel_body_cmd`  
>+ PWM控制(0:右前, 1:左后, 2:左前, 3:右后)  
`/airsim_node/drone_1/rotor_pwm_cmd`
----
>可用服务
>+ 工厂巡检数据上报  
index:(0 每条路径的中央枢纽前工厂； 1 每条路径的中央枢纽后工厂)  value:(仪表数值)  
`/airsim_node/meter_report`  

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

### 速度控制描述
>1.速度控制仅提供基础飞控，保证静止无风时可悬停，但限制最高加速度，并且出现大幅速度变化时会出现姿态波动，如需要更稳定的飞控，建议使用pwm控制

>2.此处的加速度是标量，表示x轴与y轴速度变成预期速度（速度控制中输入的数值）的快慢，最高为8m/s2，超过8的数值按8进行处理，
示例：无人机初始速度为0，此时x轴速度输入为8，加速度输入为8，x轴速度会线性增加，1s后，x轴速度从0变为8m/s

>3.速度控制的第五个参数，表示是否需要急停，当输入为1时，会忽略所有输入，速度瞬间归零进入悬停状态，但飞机会有姿态变化，急停前速度越快，急停时姿态变化越大

### 工厂巡检数据上报
>工厂巡检数据上报服务输入参数index的0表示每条路径的中央枢纽前工厂，1表示每条路径的中央枢纽后工厂

>示例：8-12路径，在中央枢纽前进入工厂后，输入index为0进行上报，中央枢纽后无需进入工厂巡检上报；12-10路径，在中央枢纽前进入工厂后，输入index为0进行上报，在中央枢纽后进入工厂后，输入index为1进行上报；

### 调试配置表
>在模拟器路径下的 _/Build/LinuxNoEditor/RMUA/Content/Configs/GameConfig.json_ 的json文件中有两个字段IgnoreAllHitCollision，IgnoreOverTime，第一个字段设置为true后，撞击不会导致比赛结束，第二个字段设置为true后，超时不会导致比赛结束。可酌情使用，便于调试。

### 找不到数据类型
> 开发速度控制ros通信所需的msg文件，可查阅VelCmdmsg文件夹

> 使用rqt_topic时发现一些数据类型缺失，可参考source官方开发案例教程中basic_dev中的airsim_ros包。具体请参考: https://github.com/RoboMaster/IntelligentUAVChampionshipBase/tree/RMUA2026


### 帧率波动
> 当帧率波动严重时，可以更换更高性能的显卡。    
也可以关闭不需要的相机降低模拟器性能需求，提升帧率稳定性。
对于本机启动，仅需要把 _/path/to/IntelligentUAVChampionshipSimulator_ 中的 _settings.json_ 中相应相机配置删除即可关闭该相机。  
对于docker启动，需要把 _/path/to/IntelligentUAVChampionshipSimulator_ 中的 _settings.json_ 中相应相机配置删除后重新构建镜像即可。   
![pic](./docs/关闭相机.png)   

### 时钟同步
> 模拟器时钟与本地时钟存在一定差异，建议使用 IMU 主题传出的时间戳作为全局时钟进行程序设计。



