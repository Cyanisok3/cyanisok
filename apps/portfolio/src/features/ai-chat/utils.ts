import type { Dispatch, SetStateAction } from "react";

import type { ChatMessage } from "./types";

export function createId() {
  if (typeof crypto !== "undefined" && "randomUUID" in crypto) {
    return crypto.randomUUID();
  }

  return `${Date.now()}-${Math.random().toString(36).slice(2)}`;
}

export function appendMessageContent(
  setMessages: Dispatch<SetStateAction<ChatMessage[]>>,
  messageId: string,
  content: string
) {
  setMessages((current) =>
    current.map((message) =>
      message.id === messageId
        ? { ...message, content: `${message.content}${content}` }
        : message
    )
  );
}

export function replaceMessageContent(
  setMessages: Dispatch<SetStateAction<ChatMessage[]>>,
  messageId: string,
  content: string
) {
  setMessages((current) =>
    current.map((message) =>
      message.id === messageId ? { ...message, content } : message
    )
  );
}

export function isUnauthorized(error: unknown) {
  return Boolean(
    error &&
      typeof error === "object" &&
      "status" in error &&
      error.status === 401
  );
}

export function isAbortError(error: unknown) {
  return error instanceof Error && error.name === "AbortError";
}

export const SUPPORTED_IMAGE_TYPES = [
  "image/png",
  "image/jpeg",
  "image/webp",
] as const;

export const CHAT_MESSAGE_MAX_BYTES = 8000;

export function getUtf8ByteLength(value: string) {
  return new TextEncoder().encode(value).byteLength;
}

export function isSupportedImageFile(file: File) {
  return SUPPORTED_IMAGE_TYPES.some((type) => type === file.type);
}

export function getErrorMessage(error: unknown) {
  if (error instanceof Error) {
    return error.message;
  }

  return "Something went wrong";
}

export function readFileAsDataUrl(file: File, signal?: AbortSignal) {
  return new Promise<string>((resolve, reject) => {
    const reader = new FileReader();

    const cleanup = () => signal?.removeEventListener("abort", handleAbort);
    const handleAbort = () => {
      reader.abort();
      reject(new DOMException("The operation was aborted", "AbortError"));
    };

    if (signal?.aborted) {
      handleAbort();
      return;
    }

    reader.onload = () => {
      cleanup();
      resolve(String(reader.result));
    };
    reader.onerror = () => {
      cleanup();
      reject(reader.error ?? new Error("Unable to read the selected image"));
    };
    reader.onabort = cleanup;
    signal?.addEventListener("abort", handleAbort, { once: true });
    reader.readAsDataURL(file);
  });
}
