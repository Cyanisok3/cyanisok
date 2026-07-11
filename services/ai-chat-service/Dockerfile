#===============================================================================
# C++ AI Chat Service - Unified Dockerfile
# Build:   docker build -t cpp-ai-service .
# Dev:     docker run --rm -it -v $(pwd):/workspace cpp-ai-service bash
# Prod:    docker run -d -p 8080:80 --name cpp-ai-service cpp-ai-service
#===============================================================================

ARG BUILD_TYPE=release
FROM ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV MYSQL_HOST=mysql
ENV HTTP_PORT=80

# Install dependencies (using Tsinghua mirror for speed)
RUN sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    make \
    git \
    wget \
    curl \
    libboost-dev \
    libssl-dev \
    nlohmann-json3-dev \
    libmysqlcppconn-dev \
    libmysqlclient-dev \
    libopencv-core-dev \
    libopencv-imgproc-dev \
    libopencv-imgcodecs-dev \
    libopencv-dnn-dev \
    libcurl4-openssl-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Build muduo
WORKDIR /tmp
RUN git clone --depth 1 https://gh-proxy.com/github.com/chenshuo/muduo.git && \
    cd muduo && \
    sed -i 's/-Werror//g' CMakeLists.txt && \
    mkdir build && cd build && \
    cmake .. && make -j$(nproc) && make install && \
    ldconfig && \
    cd /tmp && rm -rf muduo

# Install ONNX Runtime
WORKDIR /tmp
RUN if [ "$(dpkg --print-architecture)" = "arm64" ]; then \
        ONNX_PKG="onnxruntime-linux-aarch64-1.16.3.tgz"; \
    else \
        ONNX_PKG="onnxruntime-linux-x64-1.16.3.tgz"; \
    fi && \
    wget -q "https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/${ONNX_PKG}" && \
    mkdir -p onnxruntime && tar -xzf "${ONNX_PKG}" -C onnxruntime --strip-components=1 && \
    cp -r onnxruntime/include/* /usr/local/include/ && \
    cp onnxruntime/lib/* /usr/local/lib/ && \
    ldconfig && \
    rm -rf onnxruntime "${ONNX_PKG}"

# Download MobileNetV2 ONNX model and ImageNet labels
WORKDIR /root
RUN mkdir -p models/mobilenetv2 && \
    wget -q https://github.com/onnx/models/raw/main/validated/vision/classification/mobilenet/model/mobilenetv2-7.onnx -O models/mobilenetv2/mobilenetv2-7.onnx && \
    wget -q https://raw.githubusercontent.com/pytorch/hub/master/imagenet_classes.txt -O /root/imagenet_classes.txt

#===============================================================================
# Development image
#===============================================================================
FROM base AS dev

WORKDIR /workspace
CMD ["/bin/bash"]

#===============================================================================
# Build and test the production binary
#===============================================================================
FROM base AS builder

WORKDIR /project
COPY . .

RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. -DBUILD_AI_APPS=ON && \
    make -j$(nproc) && \
    ctest --output-on-failure

#===============================================================================
# Production image - runtime dependencies only
#===============================================================================
FROM ubuntu:22.04 AS production

ENV DEBIAN_FRONTEND=noninteractive
ENV MYSQL_HOST=mysql
ENV HTTP_PORT=80

RUN sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    libcurl4 \
    libmysqlclient21 \
    libmysqlcppconn7v5 \
    libopencv-core4.5d \
    libopencv-dnn4.5d \
    libopencv-imgcodecs4.5d \
    libopencv-imgproc4.5d \
    libssl3 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project

COPY --from=builder /project/build/http_server ./http_server
COPY --from=base /usr/local/lib/libonnxruntime.so* /usr/local/lib/
COPY --from=base /root/models /root/models
COPY --from=base /root/imagenet_classes.txt /root/imagenet_classes.txt

RUN ldconfig

EXPOSE ${HTTP_PORT}

CMD ["./http_server"]

#===============================================================================
# Default target: production
#===============================================================================
FROM production AS default
