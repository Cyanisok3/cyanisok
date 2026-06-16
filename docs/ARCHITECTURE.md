# Cyanisok Architecture

## Overview

`cyanisok` is an integrated personal website and AI chat system.

It combines:

- A Next.js portfolio and blog frontend.
- A `/chat` frontend integrated into the portfolio.
- A C++ muduo-based AI Chat Service.
- A Next.js same-origin API proxy.
- MySQL as the source of truth for users and chat messages.
- Docker Compose for the integrated application stack.
- Host Nginx, HTTPS, ICP footer, and GitHub Actions self-hosted deployment.

The production site is intended to be reached through:

```text
https://cyanisok.cn
```

## Repository Layout

```text
.
├── AGENTS.md
├── docker-compose.yml
├── deploy/
│   └── nginx/
│       └── cyanisok.host.conf
├── docs/
│   └── ARCHITECTURE.md
├── apps/
│   └── portfolio/
│       ├── src/app/
│       ├── src/features/ai-chat/
│       ├── src/components/
│       ├── content/
│       ├── public/
│       ├── Dockerfile
│       └── package.json
└── services/
    └── ai-chat-service/
        ├── AIApps/ChatServer/
        ├── HttpServer/
        ├── Dockerfile
        ├── init.sh
        └── .env.example
```

## Runtime Architecture

```text
Browser
  |
  | HTTPS
  v
Host Nginx :80/:443
  |
  | proxy_pass http://127.0.0.1:3000
  v
portfolio-web container
  |
  | Next.js route /api/ai-chat/*
  v
chat-service container :80
  |
  +--> DashScope-compatible AI API
  +--> MySQL
  +--> ONNX Runtime image recognizer
```

The browser should only talk to the Next.js app. It should not call the C++ backend directly.

## Portfolio App

Path:

```text
apps/portfolio
```

Main technology:

- Next.js 16 App Router.
- React 19.
- Tailwind CSS 4.
- shadcn-style UI primitives.
- Magic UI components.
- `content-collections` for MDX blog content.

Important routes:

```text
/                  portfolio home
/blog              blog index
/blog/[slug]       MDX article page
/chat              AI chat UI
/api/ai-chat/*     same-origin proxy to the C++ backend
```

The `/chat` frontend lives under:

```text
apps/portfolio/src/features/ai-chat
```

The app-level route is:

```text
apps/portfolio/src/app/chat/page.tsx
```

## Blog Content System

Blog posts are collected from:

```text
apps/portfolio/content/*.mdx
```

The content configuration is:

```text
apps/portfolio/content-collections.ts
```

Current collection behavior:

- Only first-level `*.mdx` files inside `apps/portfolio/content` are collected.
- Nested folders are not included by the current collection configuration.
- Frontmatter is validated with `zod`.
- MDX is compiled with `remark-gfm` and a custom code metadata plugin.

This document does not define article-writing style. It only records the content system as part of the application architecture.

## Next API Proxy

Proxy path:

```text
apps/portfolio/src/app/api/ai-chat/[...path]/route.ts
```

Environment variable:

```text
AI_CHAT_SERVICE_URL=http://chat-service:80
```

Production Docker sets this value for `portfolio-web`.

The proxy allowlist is intentionally narrow:

```text
POST /api/ai-chat/login        -> POST /login
POST /api/ai-chat/register     -> POST /register
POST /api/ai-chat/logout       -> POST /user/logout
POST /api/ai-chat/chat/send    -> POST /chat/send
POST /api/ai-chat/chat/history -> POST /chat/history
POST /api/ai-chat/upload/send  -> POST /upload/send
GET  /api/ai-chat/health       -> GET  /health
```

Proxy responsibilities:

- Keep browser requests same-origin.
- Forward cookies to the C++ backend for session auth.
- Forward response cookies back to the browser.
- Enforce an endpoint allowlist.
- Apply request body size limits.
- Apply a basic in-memory per-IP rate limit.
- Preserve `text/event-stream` responses for streaming chat.
- Avoid exposing backend URLs or API keys to the browser.

Current size limits:

```text
JSON requests: 32 KiB
Upload requests: 8 MiB
```

The chat UI limits selected image files to 5 MiB. Before OpenCV decodes an
uploaded image, the backend additionally enforces:

```text
Compressed image data: 6 MiB
Maximum dimension: 8192 px
Maximum pixel count: 16 megapixels
Supported formats: PNG, JPEG, GIF, WebP
```

## C++ AI Chat Service

Path:

```text
services/ai-chat-service
```

The backend contains:

- `HttpServer`: a lightweight HTTP framework built on muduo.
- `AIApps/ChatServer`: the actual AI chat application.
- MySQL integration.
- Direct MySQL persistence through domain repositories.
- ONNX Runtime image recognition.
- DashScope-compatible chat completion calls.

The C++ service is API-only. Old standalone HTML pages and page handlers were removed from the supported application surface.

Supported API routes:

```text
POST /login
POST /register
POST /user/logout
POST /chat/send
POST /chat/history
POST /upload/send
GET  /health
GET  /ready
```

`POST /chat/send` uses Server-Sent Events for streaming responses.

## Authentication and Sessions

Authentication is session-cookie based.

The C++ backend owns session state through an in-memory session storage:

```text
services/ai-chat-service/HttpServer/src/session/
```

Cookie behavior:

```text
sessionId=<id>; Path=/; HttpOnly; SameSite=Lax
```

If `SESSION_COOKIE_SECURE=true`, the cookie also gets:

```text
Secure
```

The Next.js proxy forwards the browser `Cookie` header to the backend and forwards backend `Set-Cookie` headers back to the browser.

Operational implication:

- Session state is currently memory-backed inside the C++ process.
- Restarting `chat-service` can invalidate active sessions.

## Database Schema

Schema initialization:

```text
services/ai-chat-service/init.sh
```

Current tables:

```text
users
chat_message
```

`users`:

- `id`
- `username`
- `password_hash`
- `created_at`

`chat_message`:

- `message_id`
- `user_id`
- `username`
- `is_user`
- `content`
- `ts`
- `created_at`

Messages are associated with `user_id`, not only `username`.

## Docker Stack

Root integrated stack:

```text
docker-compose.yml
```

Services:

```text
portfolio-web
chat-service
mysql
```

Production exposure:

- `portfolio-web` binds `127.0.0.1:3000` to container `3000`.
- `chat-service` is exposed only inside the Docker network.
- MySQL is internal by default.
- Public HTTP/HTTPS is handled by host Nginx, not by a Docker Nginx container.

Important service names:

```text
portfolio-web -> cyanisok-portfolio
chat-service  -> cyanisok-ai-chat-service
mysql         -> cyanisok-ai-chat-mysql
```

## Environment Variables

Root `.env` is used by Docker Compose and must not be committed.

Important variables:

```text
DASHSCOPE_API_KEY
AI_MODEL
AI_API_URL
AI_REQUEST_TIMEOUT_SECONDS
AI_CONNECT_TIMEOUT_SECONDS
AI_STREAM_IDLE_TIMEOUT_SECONDS
SESSION_COOKIE_SECURE
MYSQL_USER
MYSQL_PASSWORD
MYSQL_ROOT_PASSWORD
MYSQL_DATABASE
AI_WORKER_COUNT
AI_QUEUE_CAPACITY
CHAT_CONTEXT_MESSAGES
CHAT_HISTORY_MESSAGES
CHAT_RETENTION_DAYS
IMAGE_RECOGNITION_ENABLED
```

For the integrated stack, `portfolio-web` receives:

```text
AI_CHAT_SERVICE_URL=http://chat-service:80
```

Do not expose `DASHSCOPE_API_KEY` or any backend credential to client code.

## Nginx, HTTPS, and ICP

Tracked Nginx reference config:

```text
deploy/nginx/cyanisok.host.conf
```

Production host Nginx config location:

```text
/www/server/panel/vhost/nginx/cyanisok.cn.conf
```

Production TLS certificate files live on the server:

```text
/home/ubuntu/cyanisok/cyanisok.cn_nginx/cyanisok.cn_bundle.crt
/home/ubuntu/cyanisok/cyanisok.cn_nginx/cyanisok.cn.key
```

Local TLS material is ignored by Git:

```text
cyanisok.cn_nginx/
```

Nginx responsibilities:

- Redirect HTTP to HTTPS.
- Terminate TLS.
- Proxy all site traffic to `http://127.0.0.1:3000`.
- Keep long read/send timeouts for SSE.
- Keep `proxy_buffering off` for streaming.

ICP footer component:

```text
apps/portfolio/src/components/site-footer.tsx
```

ICP number:

```text
浙ICP备2026031525号-1
```

## Deployment Workflow

GitHub Actions workflow:

```text
.github/workflows/deploy.yml
```

Deployment model:

```text
git push origin dev
  -> GitHub Actions
  -> self-hosted runner on the production server
  -> local deployment script
```

Runner target:

```text
runs-on: [self-hosted, linux, x64, cyanisok]
```

Server project path:

```text
/home/ubuntu/cyanisok
```

Current workflow behavior:

```text
git fetch origin dev
compare origin/dev with refs/cyanisok/deployed/dev
git checkout dev
git reset --hard origin/dev
docker compose config --quiet
build only the services affected by changed paths
wait for service health checks
validate and reload Nginx when deploy/nginx changes
update refs/cyanisok/deployed/dev only after a successful deployment
docker compose ps
```

Failed or cancelled deployments do not advance the deployment baseline. The next
run therefore includes every change since the last healthy release.

Before building, the workflow requires a server-local file:

```text
/home/ubuntu/cyanisok/.env
```

At minimum it must contain non-placeholder values for:

```text
DASHSCOPE_API_KEY
MYSQL_PASSWORD
MYSQL_ROOT_PASSWORD
```

This file is ignored by Git and is not created from `.env.example`
automatically.

Backend CI is defined at:

```text
.github/workflows/backend-ci.yml
```

It runs for backend changes on `main` and `dev`, builds the C++ service, and
executes CTest.

## Local Development

Portfolio:

```text
cd apps/portfolio
pnpm dev
pnpm lint
pnpm build
```

Integrated Docker stack:

```text
docker compose up --build
```

For the integrated stack, the public entry is the Next.js app on port `3000`.

When an existing `mysql_data` volume is retained, changing MySQL credentials in
`.env` does not rotate the users already stored inside MySQL. Update the
database users first, or deliberately recreate the volume only when its data is
confirmed disposable.

## Production Checks

Common server checks:

```text
cd /home/ubuntu/cyanisok
git rev-parse --short HEAD
docker compose ps
```

Nginx checks:

```text
sudo nginx -t
sudo systemctl reload nginx || sudo nginx -s reload
```

Local-on-server HTTPS check:

```text
curl -k -I https://127.0.0.1 -H "Host: cyanisok.cn"
```

Expected:

```text
HTTP/2 200
x-powered-by: Next.js
```

## Current Constraints

- The C++ backend is API-only and should not regain standalone HTML frontend pages.
- The browser should only call `/api/ai-chat/*`.
- The C++ service is stateful because sessions are memory-backed.
- `SESSION_COOKIE_SECURE` should be `true` in real HTTPS production.
- TLS private keys and `.env` files must not be committed.
- Root Docker Compose is the source of truth for integrated deployment.
- Host Nginx owns ports `80` and `443`; do not add a Docker Nginx service that binds those ports unless the deployment model changes.

## Future Work: Personal RAG Chat Service

The main planned future work is a personal, style-aware blog AI Chat Service powered by RAG.

Initial scope:

- Index `apps/portfolio/content/*.mdx`.
- Clean frontmatter and MDX component markup before embedding.
- Chunk by article, heading, and paragraph boundaries.
- Preserve metadata:
  - `title`
  - `slug`
  - `heading`
  - `publishedAt`
  - source path
- Retrieve top-k chunks for a chat query.
- Inject retrieved context into the existing streaming chat flow.
- Return answer text with source article references.

Recommended MVP approach:

- Start with a small local or generated JSON index.
- Keep retrieval logic simple until the article corpus is larger.
- Avoid sending private `.env`, TLS certificates, or deployment-only files into the knowledge base.

Possible later upgrades:

- Qdrant as a standalone vector database for production-grade search.
- Better chunk ranking with hybrid lexical and vector retrieval.
- User-selectable modes such as "Ask my blog", "Project explainer", and "General chat".
- Personal writing-style conditioning based on selected public posts.

Do not over-engineer the RAG layer before the blog corpus becomes large enough to evaluate retrieval quality.
