# Cyanisok

Personal portfolio and integrated AI chat service.

## Structure

```txt
cyanisok/
├── apps/
│   └── portfolio/              # Next.js portfolio and /chat frontend
├── services/
│   └── ai-chat-service/        # C++ muduo API service for chat and image recognition
├── docker-compose.yml          # Integrated local/host deployment
├── .env.example                # Integrated environment template
└── README.md
```

## Run Integrated Stack

Create the root environment file:

```bash
cp .env.example .env
```

Set `DEEPSEEK_API_KEY`, `MYSQL_PASSWORD`, and `MYSQL_ROOT_PASSWORD`, then
start the full stack from the repository root:

```bash
docker compose up --build
```

The portfolio is exposed at `http://127.0.0.1:3000`. In production, the
server's host Nginx or Baota panel should reverse proxy `cyanisok.cn` to that
address. The C++ chat service and MySQL stay on the internal Docker network.

For production HTTPS, place the domain certificate files in the repository root on the server and configure the host Nginx server block to use them:

```txt
cyanisok.cn_nginx/
├── cyanisok.cn.key
└── cyanisok.cn_bundle.crt
```

This directory is intentionally ignored by Git because it contains private key material.

## Development

Portfolio-only development:

```bash
cd apps/portfolio
pnpm install
pnpm dev
```

Chat service-only development:

```bash
cd services/ai-chat-service
docker compose up --build
```

In the integrated stack, the browser talks only to the Next.js same-origin API proxy under `/api/ai-chat/*`; Next forwards those requests to the C++ service.
