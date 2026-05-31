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

Set `DASHSCOPE_API_KEY`, then start the full stack from the repository root:

```bash
docker compose up --build
```

The portfolio is exposed at `http://localhost:3000`. The C++ chat service, MySQL, and RabbitMQ stay on the internal Docker network by default.

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
