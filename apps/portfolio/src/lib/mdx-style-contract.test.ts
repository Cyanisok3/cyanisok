import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const globalsCss = readFileSync(join(process.cwd(), "src/app/globals.css"), {
  encoding: "utf8",
});

describe("MDX prose style contract", () => {
  it("keeps dark utilities bound to the application theme class", () => {
    expect(globalsCss).toContain(
      "@custom-variant dark (&:where(.dark, .dark *));"
    );
  });

  it("keeps inline code styling separate from fenced code blocks", () => {
    expect(globalsCss).toContain(".prose :where(:not(pre) > code)");
    expect(globalsCss).toContain(".prose pre code");
    expect(globalsCss).toContain("text-foreground/90!");
  });

  it("preserves markdown emphasis semantics inside headings and prose", () => {
    expect(globalsCss).toContain(".prose strong");
    expect(globalsCss).toContain(".prose em");
    expect(globalsCss).toContain(
      ".prose :where(h1, h2, h3, h4, h5, h6) strong"
    );
    expect(globalsCss).toContain(
      ".prose :where(h1, h2, h3, h4, h5, h6) em"
    );
  });

  it("styles unordered and ordered list markers with valid selectors", () => {
    expect(globalsCss).toContain(
      '.prose :where(ul > li):not(:where([class~="not-prose"] *))::marker'
    );
    expect(globalsCss).toContain(
      '.prose :where(ol > li):not(:where([class~="not-prose"] *))::marker'
    );
    expect(globalsCss).not.toContain(".prose :where():not");
  });

  it("uses the same not-prose scope for table rules", () => {
    expect(globalsCss).toContain(
      '.prose :where(table):not(:where([class~="not-prose"] *))'
    );
    expect(globalsCss).not.toContain(
      ':not(:where([class~="not-prose"]) *)'
    );
  });

  it("keeps quote and table text readable in either theme", () => {
    expect(globalsCss).toContain("text-foreground/90! italic");
    expect(globalsCss).toContain("font-semibold text-foreground!");
    expect(globalsCss).toContain("text-foreground/80! border-b");
  });
});
