import { describe, expect, it } from "vitest";
import { extractTableOfContents } from "./toc";

describe("extractTableOfContents", () => {
  it("extracts h2 and h3 while ignoring fenced code", () => {
    const content = [
      "## Section",
      "### Detail",
      "```md",
      "## Not a heading",
      "```",
    ].join("\n");

    expect(extractTableOfContents(content)).toEqual([
      { id: "section", text: "Section", depth: 2 },
      { id: "detail", text: "Detail", depth: 3 },
    ]);
  });
});
