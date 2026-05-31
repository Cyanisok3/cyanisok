import type { ApiError } from "./types";

export const API_ROOT = "/api/ai-chat";

export async function readJson<T>(
  response: Response
): Promise<T & { message?: string }> {
  const text = await response.text();
  if (!text) {
    return {} as T & { message?: string };
  }

  try {
    return JSON.parse(text) as T & { message?: string };
  } catch {
    return { message: text } as T & { message?: string };
  }
}

export async function apiRequest<T>(
  path: string,
  init: RequestInit = {}
): Promise<T & { message?: string }> {
  const headers = new Headers(init.headers);
  if (init.body && !headers.has("content-type")) {
    headers.set("content-type", "application/json");
  }

  const response = await fetch(`${API_ROOT}/${path}`, {
    ...init,
    headers,
    credentials: "same-origin",
    cache: "no-store",
  });
  const data = await readJson<T>(response);

  if (!response.ok) {
    const error = new Error(
      data.message || `Request failed with status ${response.status}`
    ) as ApiError;
    error.status = response.status;
    throw error;
  }

  return data;
}
