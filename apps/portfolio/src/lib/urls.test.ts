import { describe, expect, it } from "vitest";
import { resolveSiteUrl } from "./urls";

describe("resolveSiteUrl", () => {
  it("keeps absolute media URLs intact", () => {
    expect(
      resolveSiteUrl(
        "https://cdn.example.com/image.png",
        "https://cyanisok.cn"
      )
    ).toBe("https://cdn.example.com/image.png");
  });

  it("resolves root-relative media URLs", () => {
    expect(resolveSiteUrl("/image.png", "https://cyanisok.cn")).toBe(
      "https://cyanisok.cn/image.png"
    );
  });
});
