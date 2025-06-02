FROM gcc:14.3.0

LABEL maintainer="xray20161@gmail.com"
LABEL version="1.0"
LABEL description="C++ toolchain"

RUN apt-get update && \
    apt-get install -y \
    cmake \
    git \
    gdb

WORKDIR /project

RUN wget https://sourceware.org/pub/valgrind/valgrind-3.21.0.tar.bz2 && \
    tar -xjvf valgrind-3.21.0.tar.bz2 && \
    cd valgrind-3.21.0 && \
    ./configure && \
    make && \
    make install

# install rustup & cargo
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain stable --profile minimal
ENV PATH="/root/.cargo/bin:${PATH}"
RUN rustc --version && cargo --version && rustup --version
RUN rustup default stable

# install liburing 2.9
RUN git clone https://github.com/axboe/liburing.git && \
    cd /project/liburing && \
    git checkout liburing-2.9
RUN cd /project/liburing && \
    ./configure --cc=gcc --cxx=g++ && \
    make -j4 && \
    make liburing.pc && \
    make install
