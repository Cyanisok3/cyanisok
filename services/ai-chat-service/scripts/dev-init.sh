#!/bin/bash
# Development initialization script
# 运行一次，用于设置开发环境

set -e

echo "=== CppAIService Development Setup ==="

# 检查 .env 文件
if [ ! -f .env ]; then
    echo "Warning: .env file not found. Copying from .env.example"
    if [ -f .env.example ]; then
        cp .env.example .env
        echo "Please edit .env and add your DASHSCOPE_API_KEY"
    fi
fi

# 检查 DASHSCOPE_API_KEY
if grep -q "your_api_key_here" .env 2>/dev/null; then
    echo ""
    echo "=========================================="
    echo "IMPORTANT: Please set your DASHSCOPE_API_KEY in .env"
    echo "=========================================="
fi

echo ""
echo "=== Setup Complete ==="
echo "Now run: docker-compose up"
