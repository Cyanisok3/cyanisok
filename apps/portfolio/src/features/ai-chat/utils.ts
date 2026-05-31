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

export function getErrorMessage(error: unknown) {
  if (error instanceof Error) {
    return error.message;
  }

  return "Something went wrong";
}

export function readFileAsDataUrl(file: File) {
  return new Promise<string>((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(String(reader.result));
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}
