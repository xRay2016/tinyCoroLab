FROM gcc:13.3.0

LABEL maintainer="xray20161@gmail.com"
LABEL version="1.0"
LABEL description="C++ toolchain"

RUN apt-get update && \
    apt-get install -y \
    cmake \
    git \
    gdb

WORKDIR /project
# 拷贝third_party中的liburing
COPY third_party/liburing /project/liburing
RUN cd /project/liburing && \
    ./configure --cc=gcc --cxx=g++ && \
    make -j4 && \
    make liburing.pc && \
    make install
