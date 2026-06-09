export type TocItem = {
  id: string;
  text: string;
  depth: 2 | 3;
};

type Slugger = (text: string) => string;

function toPlainText(value: string) {
  return value
    .replace(/<[^>]+>/g, "")
    .replace(/[`*_~[\]()]/g, "")
    .replace(/\s+/g, " ")
    .trim();
}

function createSlugger(): Slugger {
  const seen = new Map<string, number>();

  return (text: string) => {
    const base =
      text
        .toLowerCase()
        .trim()
        .replace(/[^\p{Letter}\p{Number}\s-]/gu, "")
        .replace(/\s+/g, "-")
        .replace(/-+/g, "-")
        .replace(/^-|-$/g, "") || "section";
    const count = seen.get(base) ?? 0;
    seen.set(base, count + 1);

    return count === 0 ? base : `${base}-${count}`;
  };
}

export function extractTableOfContents(content: string): TocItem[] {
  const slug = createSlugger();
  const items: TocItem[] = [];
  let inCodeFence = false;

  for (const line of content.split(/\r?\n/)) {
    if (/^\s*```/.test(line)) {
      inCodeFence = !inCodeFence;
      continue;
    }

    if (inCodeFence) continue;

    const match = /^(##|###)\s+(.+?)\s*#*$/.exec(line);
    if (!match) continue;

    const text = toPlainText(match[2]);
    if (!text) continue;

    items.push({
      id: slug(text),
      text,
      depth: match[1].length as 2 | 3,
    });
  }

  return items;
}

function getNodeText(node: any): string {
  if (!node) return "";
  if (node.type === "text" && typeof node.value === "string") {
    return node.value;
  }
  if (Array.isArray(node.children)) {
    return node.children.map(getNodeText).join("");
  }
  return "";
}

function visit(node: any, visitor: (node: any) => void) {
  visitor(node);
  if (!Array.isArray(node?.children)) return;
  for (const child of node.children) {
    visit(child, visitor);
  }
}

export function rehypeHeadingIds() {
  return (tree: any) => {
    const slug = createSlugger();

    visit(tree, (node) => {
      if (
        node?.type !== "element" ||
        (node.tagName !== "h2" && node.tagName !== "h3")
      ) {
        return;
      }

      const text = toPlainText(getNodeText(node));
      if (!text) return;

      node.properties = {
        ...node.properties,
        id: slug(text),
      };
    });
  };
}
