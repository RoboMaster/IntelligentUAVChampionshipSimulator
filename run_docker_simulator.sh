docker run -it --cpuset-cpus="0-9" -e Seed=$1 --rm --name sim01 --net host --gpus 'device=0' simulator01
