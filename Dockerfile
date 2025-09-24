FROM alpine:3.22 AS requirement
ENV CPM_SOURCE_CACHE=/root/.cache/CPM
WORKDIR /root

RUN apk add --no-cache \
    git \
    g++ \
    cmake \
    samurai;

COPY cmake/ cmake/
COPY CMakeLists.txt .
RUN cmake -G Ninja -B build -DCPPXX_REQUIREMENTS=ON;


FROM gcc:14-bookworm AS test
ENV CPM_SOURCE_CACHE=/root/.cache/CPM
WORKDIR /root

RUN apt-get update && \
    apt-get install -y \
        git \
        cmake \
        ninja-build && \
    rm -rf /var/lib/apt/lists/*;


COPY cmake/ cmake/
COPY CMakeLists.txt .
COPY include/ include/
COPY tests/ tests/
COPY reflectcpp/ reflectcpp/
COPY --from=requirement /root/.cache/CPM /root/.cache/CPM
RUN cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DCPPXX_BUILD_TESTS=ON && \
    cmake --build build && \
    ./build/test_all;


FROM requirement AS build

COPY include/ include/
COPY cmd/ cmd/
COPY reflectcpp/ reflectcpp/
RUN cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCPPXX_REQUIREMENTS=OFF -DCPPXX_BUILD_CMD=ON && \
    cmake --build build;
