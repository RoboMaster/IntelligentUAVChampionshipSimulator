docker network create --driver bridge --subnet 192.168.52.0/24 --gateway 192.168.52.254 inet
xhost +local:docker
docker run -it --cpuset-cpus="0-9" -e RenderOffScreen=$1  -e Seed=$2 -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:rw --rm --name sim --net inet --ip 192.168.52.2  -e ROS_MASTER_URI=http://192.168.52.2:11311 -e ROS_IP=192.168.52.2  --gpus 'device=0' simulator
