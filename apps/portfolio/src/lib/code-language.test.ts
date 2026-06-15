import { describe, expect, it } from "vitest";
import { resolveCodeLanguage } from "./code-language";

const supported = {
  bash: {},
  cpp: {},
  latex: {},
  plaintext: {},
};

describe("resolveCodeLanguage", () => {
  it("maps tex fences to latex", () => {
    expect(resolveCodeLanguage("language-tex", supported)).toBe("latex");
  });

  it("falls back for unknown languages", () => {
    expect(resolveCodeLanguage("language-not-real", supported)).toBe(
      "plaintext"
    );
  });
});
