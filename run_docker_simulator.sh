docker run -it --cpuset-cpus="0-9" -e RenderOffScreen=1  -e Seed=$1 --rm --name sim --net host --gpus 'device=0' simulator
