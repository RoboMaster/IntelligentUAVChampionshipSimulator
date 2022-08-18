# __自主无人机竞速模拟器使用说明__  
1. ## 更新说明
   1. RGBD以及双目的同步问题已解决；
   2. 帧率优化；
   3. 提供了多机分布方案；
   4. 模拟器集合了三个赛项，不需要再分别下载，单个赛项的分支将不再维护；
      1. LINUX + WIN 双机开发模式，模拟器运行于WIN,ros运行于Linux(推荐采用此模式)
      2. LINUX + LINUX 双机开发模式， 模拟器运行于A Linux,ros运行于B Linux
      3. LINUX 单机开发模式(不推荐)  

2. ## LINUX + WIN 双机开发模式
    ### 简介
     WIN 端运行模拟器能提供最佳运行性能，双击模式分离后免除了ros 端程序对模拟器的影响，使得开发更便利。使用 Linux 端 clone此仓库。
    ### 使用说明
   1. ## LINUX 端安装 ROS noetic
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

   2. ## 安装 catkin 管理工具
    >`sudo apt install python3-catkin-tools`
       
   3. ## 进入roswrapper资源目录
    >`cd /path/to/IntelligentUAVChampionshipSimulator/roswrapper/ros` 
   
   4. ## 构建roswrapper
    >`sudo cp -rf /usr/include/eigen3/Eigen /usr/include/Eigen -R`
    ----
    >`sudo apt install ros-noetic-mavros ros-noetic-mavros-extras`
    ----
    >`sudo apt install ros-noetic-tf2-sensor-msgs`
    ----
    >`source /opt/ros/noetic/setup.bash `
    ----
    >`catkin build`

   5. ## WIN 端下载模拟器
    >在浏览器中输入此网址即可开始下载:    
    `https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/simulator/simulator_WIN.zip`
    ---
    >解压后进入文件夹中，参考simulator_WIN.md 启动模拟器  
    ![pic](doc/2022-08-18%2015-26-03%20的屏幕截图.png)

   6. ## LINUX 端启动 roswrapper
    >`cd /path/to/IntelligentUAVChampionshipSimulator/roswrapper` 
    ---
    使用脚本启动模拟器，参数为你的模拟器ip地址
    >`./simulator.sh 127.0.0.1`
    ---

3. ## LINUX 单机开发模式
    ### 简介
    仅使用一台LINUX电脑做开发，模拟器帧率会受到较大影响。
    ### 使用说明
   1. ## ROS noetic
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

   2. ## 安装 catkin 管理工具
    >`sudo apt install python3-catkin-tools`
       
   3. ## 进入roswrapper资源目录
    >`cd /path/to/IntelligentUAVChampionshipSimulator/roswrapper/ros` 
   
   4. ## 构建roswrapper
    >`sudo cp -rf /usr/include/eigen3/Eigen /usr/include/Eigen -R`
    ----
    >`sudo apt install ros-noetic-mavros ros-noetic-mavros-extras`
    ----
    >`sudo apt install ros-noetic-tf2-sensor-msgs`
    ----
    >`source /opt/ros/noetic/setup.bash `
    ----
    >`catkin build`

   5. ## 下载模拟器
    >在浏览器中输入此网址即可开始下载:    
    `https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/simulator/simulator_LINUX.zip`
    ---
    >解压后进入文件夹中，参考simulator_LINUX.md 启动模拟器  
    ![pic](doc/2022-08-18%2015-35-08%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

   6. ## LINUX 端启动 roswrapper
    >`cd /path/to/IntelligentUAVChampionshipSimulator/roswrapper` 
    ---
    使用脚本启动模拟器，参数为你的模拟器ip地址
    >`./simulator.sh 127.0.0.1`
    ---

4. ## LINUX + LINUX 双机开发模式
    ### 简介
    双击模式分离后免除了ros 端程序对模拟器的影响，使得开发更便利。但是 Linux端运行模拟器对帧率有较大的影响。使用 Linux 端 clone此仓库。
    ### 使用说明
   1. ## LINUX 端安装 ROS noetic
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

   2. ## 安装 catkin 管理工具
    >`sudo apt install python3-catkin-tools`
       
   3. ## 进入roswrapper资源目录
    >`cd /path/to/IntelligentUAVChampionshipSimulator/roswrapper/ros` 
   
   4. ## 构建roswrapper
    >`sudo cp -rf /usr/include/eigen3/Eigen /usr/include/Eigen -R`
    ----
    >`sudo apt install ros-noetic-mavros ros-noetic-mavros-extras`
    ----
    >`sudo apt install ros-noetic-tf2-sensor-msgs`
    ----
    >`source /opt/ros/noetic/setup.bash `
    ----
    >`catkin build`

   5. ## LINUX 端下载模拟器
    >在浏览器中输入此网址即可开始下载:    
    `https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/simulator/simulator_LINUX.zip`
    ---
    >解压后进入文件夹中，参考simulator_LINUX.md 启动模拟器  
    ![pic](doc/2022-08-18%2015-35-08%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

   6. ## LINUX 端启动 roswrapper
    >`cd /path/to/IntelligentUAVChampionshipSimulator/roswrapper` 
    ---
    使用脚本启动模拟器，参数为你的模拟器ip地址
    >`./simulator.sh 127.0.0.1`
    ---

## Q&A
> 启动模拟器和roswrapper后，roswrapper显示无法连接自己的服务器,需要安装docker服务来新建相关子网。
![图片](doc/2022-08-05%2015-13-10%20的屏幕截图.png)
安装docker:
>+ `sudo apt-get install ca-certificates gnupg lsb-release`
>+ `sudo mkdir -p /etc/apt/keyrings`
>+ `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
>+ `echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
>+ `sudo apt-get update`
>+ `sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin`

