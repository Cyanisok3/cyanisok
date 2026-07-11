import { describe, expect, it } from "vitest";

import {
  getUtf8ByteLength,
  isAbortError,
  isSupportedImageFile,
} from "./utils";

describe("AI chat request utilities", () => {
  it("recognizes abort errors", () => {
    expect(isAbortError(new DOMException("Aborted", "AbortError"))).toBe(true);
    expect(isAbortError(new Error("network failed"))).toBe(false);
  });

  it.each([
    "image/png",
    "image/jpeg",
    "image/webp",
  ])("accepts supported image type %s", (type) => {
    expect(isSupportedImageFile({ type } as File)).toBe(true);
  });

  it("rejects unsupported image types", () => {
    expect(isSupportedImageFile({ type: "image/gif" } as File)).toBe(false);
    expect(isSupportedImageFile({ type: "image/svg+xml" } as File)).toBe(false);
  });

  it("counts UTF-8 bytes rather than JavaScript code units", () => {
    expect(getUtf8ByteLength("hello")).toBe(5);
    expect(getUtf8ByteLength("你好")).toBe(6);
    expect(getUtf8ByteLength("🙂")).toBe(4);
    expect(getUtf8ByteLength("你".repeat(2666))).toBe(7998);
    expect(getUtf8ByteLength("你".repeat(2667))).toBe(8001);
  });
});
