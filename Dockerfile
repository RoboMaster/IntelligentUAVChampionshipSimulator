FROM ubuntu:20.04

USER root
ADD config /config
ADD Ground_true /Ground_true
ADD include /include
ADD source /source
ADD thirdParties /thirdParties
ADD CMakeLists.txt /

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt update && apt install -y cmake build-essential libboost-all-dev

RUN cd /thirdParties/glog && mkdir build && cd build && cmake .. && make && make install

RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make 

ENTRYPOINT [ "/build/Bin/Release/GNSSSIMULATOR" ]