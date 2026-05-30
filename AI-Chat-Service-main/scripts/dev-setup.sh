#!/bin/bash
# Development setup script
# 每次容器启动时运行

set -e

echo "=== Building Project ==="

# 设置 CMake 配置
mkdir -p build
cd build

# Configure and build
cmake .. -DBUILD_AI_APPS=ON -DBUILD_WEB_APPS=OFF

# 编译
make -j$(nproc)

echo "=== Build Complete ==="
