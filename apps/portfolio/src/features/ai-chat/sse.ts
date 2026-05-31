import { API_ROOT, readJson } from "./api";
import type { ApiError, ChatResponse } from "./types";

function parseSseEvent(block: string) {
  let event = "message";
  const dataLines: string[] = [];

  for (const line of block.split(/\r?\n/)) {
    if (line.startsWith("event:")) {
      event = line.slice(6).trim();
    } else if (line.startsWith("data:")) {
      dataLines.push(line.slice(5).trimStart());
    }
  }

  return {
    event,
    data: dataLines.join("\n"),
  };
}

export async function sendStreamingChat(
  question: string,
  onDelta: (content: string) => void
) {
  const response = await fetch(`${API_ROOT}/chat/send`, {
    method: "POST",
    headers: {
      "content-type": "application/json",
      accept: "text/event-stream",
    },
    credentials: "same-origin",
    cache: "no-store",
    body: JSON.stringify({ question }),
  });

  if (!response.ok) {
    const data = await readJson<ChatResponse>(response);
    const error = new Error(
      data.message || `Request failed with status ${response.status}`
    ) as ApiError;
    error.status = response.status;
    throw error;
  }

  if (!response.body) {
    throw new Error("AI chat service returned an empty stream");
  }

  const reader = response.body.getReader();
  const decoder = new TextDecoder();
  let buffer = "";
  let finalContent = "";

  function processBuffer(flush = false) {
    const normalized = buffer.replace(/\r\n/g, "\n");
    const blocks = normalized.split("\n\n");
    buffer = flush ? "" : blocks.pop() ?? "";

    const completeBlocks = flush ? blocks.filter(Boolean) : blocks;
    for (const block of completeBlocks) {
      const { event, data } = parseSseEvent(block);
      if (!data) continue;

      let payload: { content?: string; message?: string };
      try {
        payload = JSON.parse(data) as { content?: string; message?: string };
      } catch {
        payload = { content: data };
      }

      if (event === "delta") {
        const content = payload.content ?? "";
        finalContent += content;
        onDelta(content);
      } else if (event === "done") {
        finalContent = payload.content ?? finalContent;
      } else if (event === "error") {
        throw new Error(payload.message || "AI stream failed");
      }
    }
  }

  while (true) {
    const { done, value } = await reader.read();
    if (value) {
      buffer += decoder.decode(value, { stream: !done });
      processBuffer(false);
    }

    if (done) {
      buffer += decoder.decode();
      processBuffer(true);
      break;
    }
  }

  return finalContent;
}
