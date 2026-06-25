# CppAIService - C++ HTTP Server Framework & AI Chat API Service

## Project Overview

This is a C++-based HTTP server framework with built-in AI capabilities. The project consists of two main components:

- **HttpServer**: A lightweight, high-performance HTTP server framework featuring routing, middleware, sessions, and SSL support
- **AIApps/ChatServer**: An API-only chat service built on top of HttpServer,
  integrating DeepSeek, optional ONNX image recognition, and direct MySQL
  persistence. The user-facing UI lives in the Next.js portfolio `/chat`
  route.

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
├── AIApps/ChatServer/            # Chat API Service
│   ├── include/
│   │   ├── handlers/             # HTTP request handlers
│   │   └── AIUtil/               # AI utilities (DeepSeek, Image Recognition)
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

### Environment Variables

Create a `.env` file based on `.env.example`:

```bash
cp .env.example .env
```

Required variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `DEEPSEEK_API_KEY` | DeepSeek API Key | Required |
| `MYSQL_PASSWORD` | Application database password | Required |
| `MYSQL_ROOT_PASSWORD` | MySQL administrative password | Required |

## Build & Run

All builds and runs are performed using **Docker Compose**.

### 1. Start Infrastructure Services

Start MySQL:

```bash
docker compose up -d mysql
```

### 2. Build & Run Application

Build and run the chat server:

```bash
docker-compose up --build -d app
```

The service exposes HTTP APIs at `http://localhost:8080`. In the integrated stack, the portfolio app is the user-facing frontend at `/chat`, and this C++ service is accessed through the Next.js `/api/ai-chat/*` proxy.

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

### 6. Run Tests

```bash
ctest --test-dir build --output-on-failure
./tests/smoke.sh
```

## Application Routes

| Method | Path | Description |
|--------|------|-------------|
| POST | `/login` | User login |
| POST | `/register` | User registration |
| POST | `/user/logout` | User logout |
| POST | `/chat/send` | Stream AI chat response via Server-Sent Events |
| POST | `/chat/history` | Get chat history |
| POST | `/upload/send` | Upload and recognize image |
| GET | `/health` | Process liveness |
| GET | `/ready` | Database and AI configuration readiness |

## Key Features

### Chat Functionality

- User registration and login with session-based authentication
- Streaming chat with DeepSeek AI
- Persistent chat history stored in MySQL
- Direct, ordered MySQL persistence with bounded AI execution
- Recent-message context loaded from MySQL for each request

### Image Recognition

- Upload images for classification using MobileNetV2 ONNX model
- ImageNet class labels

### Architecture Highlights

- **Reactor Pattern**: Built on muduo network library for high-performance I/O
- **Middleware Chain**: Extensible middleware for cross-cutting concerns
- **Connection Pooling**: MySQL connection pool for efficient database access
- **Bounded concurrency**: Fixed AI workers, bounded queue, and one active chat per user

## Third-Party Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| muduo | latest | C++ network library |
| Boost | system | Utility library |
| OpenSSL | system | SSL/TLS encryption |
| nlohmann/json | 3.x | JSON parsing |
| MySQL Connector/C++ | system | MySQL database driver |
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

### ChatServer API Service

```
AIApps/ChatServer/
├── include/handlers/       # Request handlers (Login, Chat, Upload, etc.)
├── include/AIUtil/         # AIHelper, ImageRecognizer, BoundedExecutor
└── src/                   # Implementation files
```

The legacy standalone HTML page handlers and templates have been removed. Keep new user-facing chat UI work in the portfolio app and keep this module focused on API handlers, service integrations, persistence, and streaming response transport.

## Troubleshooting

### Container won't start

Check if ports are already in use:

```bash
lsof -i :8080
lsof -i :3306
```

### Database connection issues

Ensure MySQL is healthy before starting the app:

```bash
docker-compose ps
docker-compose logs mysql
```

### API errors

Verify your `DEEPSEEK_API_KEY` is set correctly in `.env`.

## License

MIT License
