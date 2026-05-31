import { NextRequest, NextResponse } from "next/server";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

type AllowedRoute = {
  method: "GET" | "POST";
  target: string;
  maxBodyBytes: number;
  requestsPerMinute: number;
  streaming?: boolean;
};

const MAX_JSON_BYTES = 32 * 1024;
const MAX_UPLOAD_BYTES = 8 * 1024 * 1024;

const ALLOWED_ROUTES: Record<string, AllowedRoute> = {
  login: {
    method: "POST",
    target: "/login",
    maxBodyBytes: MAX_JSON_BYTES,
    requestsPerMinute: 20,
  },
  register: {
    method: "POST",
    target: "/register",
    maxBodyBytes: MAX_JSON_BYTES,
    requestsPerMinute: 10,
  },
  logout: {
    method: "POST",
    target: "/user/logout",
    maxBodyBytes: MAX_JSON_BYTES,
    requestsPerMinute: 30,
  },
  "chat/send": {
    method: "POST",
    target: "/chat/send",
    maxBodyBytes: MAX_JSON_BYTES,
    requestsPerMinute: 30,
    streaming: true,
  },
  "chat/history": {
    method: "POST",
    target: "/chat/history",
    maxBodyBytes: MAX_JSON_BYTES,
    requestsPerMinute: 30,
  },
  "upload/send": {
    method: "POST",
    target: "/upload/send",
    maxBodyBytes: MAX_UPLOAD_BYTES,
    requestsPerMinute: 8,
  },
  health: {
    method: "GET",
    target: "/health",
    maxBodyBytes: 0,
    requestsPerMinute: 120,
  },
};

const rateBuckets = new Map<string, number[]>();

type RouteContext = {
  params: Promise<{ path?: string[] }>;
};

function jsonError(message: string, status: number) {
  return NextResponse.json({ status: "error", message }, { status });
}

function clientIp(request: NextRequest) {
  const forwardedFor = request.headers.get("x-forwarded-for");
  if (forwardedFor) {
    return forwardedFor.split(",")[0]?.trim() || "unknown";
  }
  return request.headers.get("x-real-ip") || "unknown";
}

function checkRateLimit(request: NextRequest, routeKey: string, limit: number) {
  const now = Date.now();
  const windowMs = 60_000;
  const bucketKey = `${clientIp(request)}:${routeKey}`;
  const recent = (rateBuckets.get(bucketKey) ?? []).filter(
    (timestamp) => now - timestamp < windowMs
  );

  if (recent.length >= limit) {
    rateBuckets.set(bucketKey, recent);
    return false;
  }

  recent.push(now);
  rateBuckets.set(bucketKey, recent);

  if (rateBuckets.size > 1000) {
    for (const [key, timestamps] of rateBuckets.entries()) {
      const fresh = timestamps.filter((timestamp) => now - timestamp < windowMs);
      if (fresh.length === 0) {
        rateBuckets.delete(key);
      } else {
        rateBuckets.set(key, fresh);
      }
    }
  }

  return true;
}

function copyResponseHeaders(source: Response, destination: NextResponse) {
  const skippedHeaders = new Set([
    "access-control-allow-credentials",
    "access-control-allow-headers",
    "access-control-allow-methods",
    "access-control-allow-origin",
    "access-control-max-age",
    "connection",
    "content-encoding",
    "content-length",
    "keep-alive",
    "set-cookie",
    "transfer-encoding",
  ]);

  source.headers.forEach((value, key) => {
    if (!skippedHeaders.has(key.toLowerCase())) {
      destination.headers.set(key, value);
    }
  });

  const cookieHeaders =
    (
      source.headers as Headers & {
        getSetCookie?: () => string[];
      }
    ).getSetCookie?.() ?? [];

  if (cookieHeaders.length > 0) {
    cookieHeaders.forEach((cookie) => {
      destination.headers.append("set-cookie", cookie);
    });
  } else {
    const cookie = source.headers.get("set-cookie");
    if (cookie) {
      destination.headers.append("set-cookie", cookie);
    }
  }

  destination.headers.set("cache-control", "no-store");
}

async function proxy(request: NextRequest, context: RouteContext) {
  const { path = [] } = await context.params;
  const routeKey = path.join("/");
  const route = ALLOWED_ROUTES[routeKey];

  if (!route) {
    return jsonError("Unknown AI chat endpoint", 404);
  }

  if (request.method !== route.method) {
    return jsonError("Method not allowed", 405);
  }

  if (!checkRateLimit(request, routeKey, route.requestsPerMinute)) {
    return jsonError("Too many requests. Please wait a moment and try again.", 429);
  }

  const serviceUrl = process.env.AI_CHAT_SERVICE_URL;
  if (!serviceUrl) {
    return jsonError("AI chat service is not configured", 503);
  }

  const contentLength = Number(request.headers.get("content-length") ?? 0);
  if (contentLength > route.maxBodyBytes) {
    return jsonError("Request body is too large", 413);
  }

  let body: ArrayBuffer | undefined;
  if (route.method !== "GET") {
    const requestBody = await request.arrayBuffer();
    if (requestBody.byteLength > route.maxBodyBytes) {
      return jsonError("Request body is too large", 413);
    }
    body = requestBody;
  }

  const target = new URL(route.target, serviceUrl);
  target.search = request.nextUrl.search;

  const headers = new Headers();
  const contentType = request.headers.get("content-type");
  const cookie = request.headers.get("cookie");
  const accept = request.headers.get("accept");

  if (contentType) headers.set("content-type", contentType);
  if (cookie) headers.set("cookie", cookie);
  if (accept) headers.set("accept", accept);
  if (body) headers.set("content-length", String(body.byteLength));

  try {
    const upstream = await fetch(target, {
      method: route.method,
      headers,
      body,
      redirect: "manual",
      cache: "no-store",
    });

    if (route.streaming) {
      const response = new NextResponse(upstream.body, {
        status: upstream.status,
        statusText: upstream.statusText,
      });

      copyResponseHeaders(upstream, response);
      return response;
    }

    const responseBody = await upstream.arrayBuffer();
    const response = new NextResponse(responseBody, {
      status: upstream.status,
      statusText: upstream.statusText,
    });

    copyResponseHeaders(upstream, response);
    return response;
  } catch {
    return jsonError("Unable to reach AI chat service", 502);
  }
}

export async function GET(request: NextRequest, context: RouteContext) {
  return proxy(request, context);
}

export async function POST(request: NextRequest, context: RouteContext) {
  return proxy(request, context);
}
