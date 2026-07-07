import { describe, expect, it } from "vitest";
import { resolveCodeLanguage } from "./code-language";

const supported = {
  bash: {},
  cpp: {},
  latex: {},
  plaintext: {},
  python: {},
};

describe("resolveCodeLanguage", () => {
  it("maps text fences to plaintext", () => {
    expect(resolveCodeLanguage("language-text", supported)).toBe("plaintext");
    expect(resolveCodeLanguage("language-txt", supported)).toBe("plaintext");
  });

  it("maps py fences to python", () => {
    expect(resolveCodeLanguage("language-py", supported)).toBe("python");
  });

  it("maps tex fences to latex", () => {
    expect(resolveCodeLanguage("language-tex", supported)).toBe("latex");
  });

  it("falls back for unknown languages", () => {
    expect(resolveCodeLanguage("language-not-real", supported)).toBe(
      "plaintext"
    );
  });
});
