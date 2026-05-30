# CppAIService - C++ HTTP Server Framework & AI Chat Application

## Project Overview

This is a C++-based HTTP server framework with built-in AI capabilities. The project consists of two main components:

- **HttpServer**: A lightweight, high-performance HTTP server framework featuring routing, middleware, sessions, and SSL support
- **AIApps/ChatServer**: A chat application built on top of HttpServer, integrating with Alibaba Cloud's DashScope AI API and RabbitMQ message queue

## Architecture

```
CppAIService/
├── HttpServer/                    # HTTP Server Framework
│   ├── include/
│   │   ├── http/                 # Core HTTP handling (request/response parsing)
│   │   ├── router/               # Route matching (static & dynamic)
│   │   ├── middleware/            # Middleware chain (e.g., CORS)
│   │   ├── session/              # Session management
│   │   ├── ssl/                  # HTTPS/SSL support
│   │   └── utils/                # Utilities (MySQL, File, JSON)
│   └── src/
├── AIApps/ChatServer/            # Chat Application
│   ├── include/
│   │   ├── handlers/             # HTTP request handlers
│   │   └── AIUtil/               # AI utilities (DashScope, Image Recognition)
│   ├── resource/                 # HTML templates
│   └── src/
└── docker-compose.yml            # Container orchestration
```

### HttpServer Modules

| Module | Description |
|--------|-------------|
| **http** | Core HTTP parsing and handling (HttpServer, HttpRequest, HttpResponse, HttpContext) |
| **router** | Route matching with support for static paths and dynamic parameters |
| **middleware** | Middleware chain for pre/post request processing (e.g., CORS handling) |
| **session** | Session management with cookie-based user identification |
| **ssl** | HTTPS support via OpenSSL |
| **utils** | Database connection pool, file utilities, JSON parsing |

## Environment Dependencies

### Services (via Docker Compose)

| Service | Image | Purpose |
|---------|-------|---------|
| MySQL | mysql:8.0 | User data and chat message storage |
| RabbitMQ | rabbitmq:3-management | Async message queue for database operations |

### Environment Variables

Create a `.env` file based on `.env.example`:

```bash
cp .env.example .env
```

Required variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `DASHSCOPE_API_KEY` | Alibaba Cloud DashScope API Key | Required |
| `MYSQL_PASSWORD` | MySQL root password | 123456 |
| `RABBITMQ_USER` | RabbitMQ username | guest |
| `RABBITMQ_PASS` | RabbitMQ password | guest |

## Build & Run

All builds and runs are performed using **Docker Compose**.

### 1. Start Infrastructure Services

Start MySQL and RabbitMQ:

```bash
docker-compose up -d mysql rabbitmq
```

### 2. Build & Run Application

Build and run the chat server:

```bash
docker-compose up --build -d app
```

The application will be available at `http://localhost:8080`

### 3. Development Mode

For development with shell access:

```bash
docker-compose --profile dev up -d app_dev
docker-compose exec app_dev bash
```

Inside the dev container, you can manually build:

```bash
mkdir -p build && cd build
cmake .. -DBUILD_AI_APPS=ON
make -j$(nproc)
./http_server
```

### 4. View Logs

```bash
docker-compose logs -f app
```

### 5. Stop Services

```bash
docker-compose down
```

To also remove volumes (database data):

```bash
docker-compose down -v
```

## Application Routes

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Entry page |
| GET | `/entry` | Login/Register page |
| POST | `/login` | User login |
| POST | `/register` | User registration |
| POST | `/user/logout` | User logout |
| GET | `/chat` | Chat interface |
| POST | `/chat/send` | Send message to AI |
| POST | `/chat/history` | Get chat history |
| GET | `/menu` | AI features menu |
| GET | `/upload` | Image upload page |
| POST | `/upload/send` | Upload and recognize image |
| GET | `/health` | Health check |

## Key Features

### Chat Functionality

- User registration and login with session-based authentication
- Real-time chat with Alibaba Cloud DashScope AI (Qwen model)
- Persistent chat history stored in MySQL
- Async database writes via RabbitMQ for better performance

### Image Recognition

- Upload images for classification using MobileNetV2 ONNX model
- ImageNet class labels

### Architecture Highlights

- **Reactor Pattern**: Built on muduo network library for high-performance I/O
- **Middleware Chain**: Extensible middleware for cross-cutting concerns
- **Connection Pooling**: MySQL connection pool for efficient database access
- **Message Queue**: RabbitMQ for async operations and traffic smoothing

## Third-Party Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| muduo | latest | C++ network library |
| Boost | system | Utility library |
| OpenSSL | system | SSL/TLS encryption |
| nlohmann/json | 3.x | JSON parsing |
| MySQL Connector/C++ | system | MySQL database driver |
| SimpleAmqpClient | latest | RabbitMQ client |
| ONNX Runtime | 1.16.3 | Image classification |
| OpenCV | system | Image processing |
| libcurl | system | HTTP client for AI API calls |

## Project Structure Details

### HttpServer Framework

```
HttpServer/
├── include/http/           # HTTP protocol handling
├── include/router/         # Route registration and matching
├── include/middleware/      # Request/response interceptors
├── include/session/         # User session management
├── include/ssl/            # Secure connection handling
└── include/utils/          # Database, file, JSON utilities
```

### ChatServer Application

```
AIApps/ChatServer/
├── include/handlers/       # Request handlers (Login, Chat, Upload, etc.)
├── include/AIUtil/         # AIHelper, ImageRecognizer, MQManager
├── resource/               # HTML pages
└── src/                   # Implementation files
```

## Troubleshooting

### Container won't start

Check if ports are already in use:

```bash
lsof -i :8080
lsof -i :3306
lsof -i :5672
```

### Database connection issues

Ensure MySQL is healthy before starting the app:

```bash
docker-compose ps
docker-compose logs mysql
```

### API errors

Verify your `DASHSCOPE_API_KEY` is set correctly in `.env`.

## License

MIT License
