import { afterEach, describe, expect, it, vi } from "vitest";
import { parseSseEvent, sendStreamingChat } from "./sse";

function streamResponse(body: string) {
  const encoder = new TextEncoder();
  return new Response(
    new ReadableStream({
      start(controller) {
        controller.enqueue(encoder.encode(body));
        controller.close();
      },
    }),
    {
      status: 200,
      headers: { "content-type": "text/event-stream" },
    }
  );
}

afterEach(() => {
  vi.unstubAllGlobals();
});

describe("parseSseEvent", () => {
  it("parses named events and multiline data", () => {
    expect(
      parseSseEvent(
        'event: delta\r\ndata: {"content":"hello"}\r\ndata: continued'
      )
    ).toEqual({
      event: "delta",
      data: '{"content":"hello"}\ncontinued',
    });
  });
});

describe("sendStreamingChat", () => {
  it("returns the completed response", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue(
        streamResponse(
          [
            'event: delta\ndata: {"content":"hello"}',
            'event: done\ndata: {"content":"hello"}',
            "",
          ].join("\n\n")
        )
      )
    );
    const onDelta = vi.fn();

    await expect(sendStreamingChat("question", onDelta)).resolves.toBe("hello");
    expect(onDelta).toHaveBeenCalledWith("hello");
  });

  it("rejects a stream that closes without a done event", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue(
        streamResponse('event: delta\ndata: {"content":"partial"}\n\n')
      )
    );
    const onDelta = vi.fn();

    await expect(sendStreamingChat("question", onDelta)).rejects.toThrow(
      "AI chat stream ended before completion"
    );
    expect(onDelta).toHaveBeenCalledWith("partial");
  });
});
