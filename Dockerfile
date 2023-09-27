FROM adamrehn/ue4-runtime:20.04-vulkan-noaudio

USER root
RUN echo 'Etc/UTC' > /etc/timezone && \
    ln -s /usr/share/zoneinfo/Etc/UTC /etc/localtime && \
    apt-get update && \
    apt-get install -q -y --no-install-recommends tzdata && \
    rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install -q -y --no-install-recommends \
    dirmngr \
    gnupg2 \
    && rm -rf /var/lib/apt/lists/*
RUN echo "deb http://packages.ros.org/ros/ubuntu focal main" > /etc/apt/sources.list.d/ros1-latest.list
RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8
ENV ROS_DISTRO noetic
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-noetic-ros-core=1.5.0-1* \
    unzip \
    ffmpeg
RUN apt-get install -y --no-install-recommends iproute2 iputils-ping\
    && rm -rf /var/lib/apt/lists/* 

ADD start.bash /usr/local/
RUN chmod +x /usr/local/start.bash
# ADD Build /usr/local/
ADD https://stg-robomasters-hz-q0o2.oss-cn-hangzhou.aliyuncs.com/uasim_2301_student_230926_r1_shipping.zip /usr/local/
RUN cd /usr/local/ && unzip -o uasim_2301_student_230926_r1_shipping.zip && mv Build/LinuxNoEditor /usr/local/
ADD settings.json /usr/local/LinuxNoEditor/RMUA/Binaries/Linux/

RUN chmod +x /usr/local/LinuxNoEditor/RMUA/Binaries/Linux/RMUA-Linux-Shipping
USER ue4
ENTRYPOINT [ "/usr/local/start.bash" ]
