# Agent Context

This repository is a full-stack personal portfolio system, not only a static blog.

Before architecture, backend, deployment, Docker, Nginx, or AI chat changes, read:

- `docs/ARCHITECTURE.md`

Core project areas:

- `apps/portfolio`: Next.js portfolio, blog, and `/chat` frontend.
- `services/ai-chat-service`: C++ muduo-based API service for AI chat, auth, history, and image recognition.
- `docker-compose.yml`: integrated production stack for portfolio, chat service, MySQL, and RabbitMQ.
- `.github/workflows/deploy.yml`: self-hosted runner deployment on the production server.

Important constraints:

- The C++ service is API-only. Do not restore old standalone HTML pages.
- Browser code must call same-origin `/api/ai-chat/*`, not the C++ service directly.
- Do not expose API keys, TLS private keys, or `.env` values to the browser or Git.
- Do not rebuild `chat-service` on every frontend-only deploy unless backend files changed.
- Blog content editing is out of scope unless the user explicitly asks for content changes.

