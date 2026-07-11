# Agent Context

This repository is a full-stack personal portfolio system, not only a static blog.

Before architecture, backend, deployment, Docker, Nginx, or AI chat changes, read:

- `docs/ARCHITECTURE.md`

Core project areas:

- `apps/portfolio`: Next.js portfolio, blog, and `/chat` frontend.
- `services/ai-chat-service`: C++ muduo-based API service for AI chat, auth, history, and image recognition.
- `docker-compose.yml`: integrated production stack for portfolio, chat service, and MySQL.
- `.github/workflows/deploy.yml`: self-hosted runner deployment on the production server.

Important constraints:

- The C++ service is API-only. Do not restore old standalone HTML pages.
- Browser code must call same-origin `/api/ai-chat/*`, not the C++ service directly.
- Do not expose API keys, TLS private keys, or `.env` values to the browser or Git.
- Keep public registration disabled in production. Do not add a browser registration
  route or set `REGISTRATION_ENABLED=true` without explicit user authorization.
- Production configuration starts from `.env.production.example`; keep `.env` mode
  `0600`, `SESSION_COOKIE_SECURE=true`, and `AI_MAX_TOKENS=4096`.
- Successful login must rotate the session ID. Keep blocking AI and image work
  off muduo I/O loops and preserve bounded executor backpressure.
- Keep `/chat` session probes, history syncs, streams, and uploads abortable and
  generation-guarded so stale responses cannot replace current UI state.
- Streaming authentication checks must use `SessionManager::findSession()` so
  unauthenticated requests do not allocate anonymous sessions.
- Project videos autoplay muted and preload metadata in ordinary mode. Under
  `prefers-reduced-motion`, keep autoplay off and expose native controls.
- In `HttpServer`, `EventLoop` must be declared before `TcpServer` so construction
  and destruction order remains valid.
- Do not rebuild `chat-service` on every frontend-only deploy unless backend files changed.
- Blog content editing is out of scope unless the user explicitly asks for content changes.

Blog and MDX rendering constraints:

- Blog posts are MDX/content-collections backed under `apps/portfolio/content`.
- Treat MDX display behavior as an application rendering contract, not as article content.
- For MDX rendering bugs, prefer fixes in `apps/portfolio/src/mdx-components.tsx`,
  `apps/portfolio/src/components/mdx/`, `apps/portfolio/src/app/globals.css`, or
  `apps/portfolio/src/lib/` tests before editing posts.
- Preserve the current prose contract: `CodeBlock` owns fenced-code block surfaces,
  Shiki owns token colors, inline code styling must stay scoped to non-`pre` code,
  plaintext fences must stay readable in both themes, and `not-prose` selectors
  must remain explicit.
- When changing blog MDX rendering, prose styles, code blocks, or TOC behavior,
  run `pnpm -C apps/portfolio test`, `pnpm -C apps/portfolio lint`, and
  `pnpm -C apps/portfolio build`.
