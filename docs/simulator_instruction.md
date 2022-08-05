# __自主无人机竞速模拟器使用说明__  
1. ## 下载模拟器（按需下载） 
    >赛项一+RGBD:   
    `git clone https://github.com/RoboMaster/IntelligentUAVChampionshipSimulator.git -b stage1_RGBD`
    ----
    >赛项二+RGBD:   
    `git clone https://github.com/RoboMaster/IntelligentUAVChampionshipSimulator.git -b stage2_RGBD`
    ----
    >赛项一+双目:   
    `git clone https://github.com/RoboMaster/IntelligentUAVChampionshipSimulator.git -b stage1_stereo`
    ----
    >赛项二+双目:   
    `git clone https://github.com/RoboMaster/IntelligentUAVChampionshipSimulator.git -b stage2_stereo`
    ----
    >赛项三:   
    `git clone https://github.com/RoboMaster/IntelligentUAVChampionshipSimulator.git -b stage3`

3. ## 下载资源文件到特定位置
    >`cd IntelligentUAVChampionshipSimulator`  
    ---
    >`wget -P ./stage2_stereo/complex/Content/Paks/  https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/simulator/stage2/complex-LinuxNoEditor.pak`
    ---
    >`wget -P ./stage2_stereo/complex/Binaries/Linux/ https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/simulator/stage2/complex`

4. ## 安装 ROS noetic
    >设置软件库   
    `sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'`  
    ----    
    >安装 curl  
    `sudo apt install curl `  
    ----
    >设置KEY  
    `curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -`   
    ----
    >更新软件列表    
    `sudo apt update`
    ----
    >安装完整ROS包  
    `sudo apt install ros-noetic-desktop-full`

5. ## 安装 catkin 管理工具
    `sudo apt install python3-catkin-tools`
    
6. ## 进入模拟器文件*gameX_XXX*的根目录
    `cd /path/to/gameX_XXX`

7. ## 构建roswrapper
    >`sudo cp -rf /usr/include/eigen3/Eigen /usr/include/Eigen -R`
    ----
    >`sudo apt install ros-noetic-mavros ros-noetic-mavros-extras`
    ----
    >`sudo apt install ros-noetic-tf2-sensor-msgs`
    ----
    >`source /opt/ros/noetic/setup.bash `
    ----
    >`cd $PWD/roswrapper/ros`
    ----
    >`catkin build`

8. ## 使用脚本启动模拟器
    >`cd /path/to/stageX_XXX`  
    ----
    >`./simulator.sh`

    当看到如下画面时说明模拟器启动成功
    ![图片](./2022-07-30%2013-35-55%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)
    ![图片](./2022-07-30%2013-36-06%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

## Q&A
> 启动模拟器和roswrapper后，roswrapper显示无法连接自己的服务器,需要安装docker服务来新建相关子网。
![图片](./2022-08-05%2015-13-10%20的屏幕截图.png)
安装docker:
>+ `sudo apt-get install ca-certificates gnupg lsb-release`
>+ `sudo mkdir -p /etc/apt/keyrings`
>+ `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
>+ `echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
>+ `sudo apt-get update`
>+ `sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin`
