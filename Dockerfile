# Building the image:
# sudo docker build -t ftxcc - < ./Dockerfile
#
# Running:
# sudo docker run -it -v /path/to/src/:/ftxcc -w /ftxcc/bin ftxcc /bin/bash

FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get install -y \
    libssl-dev \
    gcc \
    g++ \
    make \
    pkg-config \
    cmake \
    curl \
    tar \
    less \
    gzip \
    ssh \
    ca-certificates \
    libwebsocketpp-dev \
    libboost-dev \
    build-essential software-properties-common # required for add-apt-repository
