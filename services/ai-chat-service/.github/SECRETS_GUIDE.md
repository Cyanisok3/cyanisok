# GitHub Actions Secrets 配置指南

## 需要的 Secrets

在 GitHub 仓库的 `Settings > Secrets and variables > Actions` 中添加以下 secrets：

### Docker Hub (必需)

| Secret 名称 | 说明 | 如何获取 |
|-------------|------|----------|
| `DOCKERHUB_USERNAME` | Docker Hub 用户名 | Docker Hub 账号 |
| `DOCKERHUB_TOKEN` | Docker Hub 访问令牌 | Docker Hub > Account Settings > Security > New Access Token |

### 部署服务器 (可选 - 如需自动部署)

| Secret 名称 | 说明 | 如何获取 |
|-------------|------|----------|
| `SERVER_HOST` | 服务器 IP 或域名 | 你的云服务器地址 |
| `SERVER_USER` | SSH 用户名 | 通常是 `root` 或其他用户 |
| `SERVER_SSH_KEY` | SSH 私钥 | `cat ~/.ssh/id_rsa` |

### 运行时环境变量 (可选)

| Secret 名称 | 说明 |
|-------------|------|
| `DASHSCOPE_API_KEY` | 阿里云 DashScope API Key |
| `RABBITMQ_HOST` | RabbitMQ 主机地址 |
| `MYSQL_HOST` | MySQL 主机地址 |

## 配置步骤

### 1. 添加 Docker Hub Secrets

1. 进入 GitHub 仓库 `Settings`
2. 左侧菜单选择 `Secrets and variables > Actions`
3. 点击 `New repository secret`
4. 添加 `DOCKERHUB_USERNAME` 和 `DOCKERHUB_TOKEN`

### 2. 添加服务器部署 Secrets (可选)

如果需要自动部署到服务器，添加：

- `SERVER_HOST`: 你的服务器 IP
- `SERVER_USER`: SSH 用户名
- `SERVER_SSH_KEY`: SSH 私钥内容

### 3. 启用 Actions

推送代码后，GitHub Actions 会自动：
1. 运行 CI 构建测试
2. 构建 Docker 镜像并推送到 Docker Hub

### 触发条件

| 分支 | 行为 |
|------|------|
| `main` | CI 运行，CD 部署 |
| `develop` | 仅 CI 运行 |
| `PR` | 仅 CI 运行 |

## 工作流说明

```
┌─────────────────────────────────────────────────────────────┐
│                        push / PR                            │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   CI - Build & Test                         │
│  - 安装依赖 (boost, openssl, mysql, rabbitmq, etc.)          │
│  - 编译 muduo 库                                            │
│  - 编译 SimpleAmqpClient 库                                 │
│  - 编译项目                                                 │
│  - 上传构建产物                                             │
└─────────────────────────┬───────────────────────────────────┘
                          │ success
                          ▼
┌─────────────────────────────────────────────────────────────┐
│           CD - Docker Build & Push                          │
│  (仅 main 分支)                                             │
│  - 构建 Docker 镜像 (多架构: amd64, arm64)                   │
│  - 推送到 Docker Hub                                         │
│  - 可选: 部署到服务器                                        │
└─────────────────────────────────────────────────────────────┘
```
