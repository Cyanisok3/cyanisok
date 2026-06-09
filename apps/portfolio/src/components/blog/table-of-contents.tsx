import { ListTree } from "lucide-react";
import { cn } from "@/lib/utils";
import type { TocItem } from "@/lib/toc";

type TableOfContentsProps = {
  items: TocItem[];
  variant: "mobile" | "desktop";
  className?: string;
};

function TocLinks({ items }: { items: TocItem[] }) {
  return (
    <ol className="space-y-1.5">
      {items.map((item, index) => (
        <li key={item.id}>
          <a
            href={`#${item.id}`}
            className={cn(
              "group flex items-baseline gap-2 rounded-md px-2 py-1.5 text-sm leading-snug text-muted-foreground transition-colors hover:bg-muted/60 hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
              item.depth === 3 && "ml-4 text-xs"
            )}
          >
            <span className="font-mono text-[10px] tabular-nums text-muted-foreground/70 group-hover:text-foreground/70">
              {String(index + 1).padStart(2, "0")}
            </span>
            <span>{item.text}</span>
          </a>
        </li>
      ))}
    </ol>
  );
}

export function TableOfContents({
  items,
  variant,
  className,
}: TableOfContentsProps) {
  if (items.length < 2) {
    return null;
  }

  if (variant === "mobile") {
    return (
      <details
        className={cn(
          "group rounded-lg border bg-background/70 p-3 shadow-sm backdrop-blur xl:hidden",
          className
        )}
      >
        <summary className="flex cursor-pointer list-none items-center justify-between gap-3 rounded-md text-xs font-medium uppercase tracking-wide text-muted-foreground marker:hidden focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring">
          <span className="inline-flex items-center gap-2">
            <ListTree className="size-3.5" />
            Contents
          </span>
          <span className="font-mono text-[10px] text-muted-foreground/70">
            {items.length} sections
          </span>
        </summary>
        <div className="mt-3 border-t border-border pt-3">
          <TocLinks items={items} />
        </div>
      </details>
    );
  }

  return (
    <aside
      className={cn("hidden xl:block", className)}
      aria-label="Table of contents"
    >
      <div className="sticky top-24 rounded-lg border bg-background/65 p-3 shadow-sm backdrop-blur">
        <div className="mb-3 flex items-center gap-2 border-b border-border pb-3 text-xs font-medium uppercase tracking-wide text-muted-foreground">
          <ListTree className="size-3.5" />
          Contents
        </div>
        <TocLinks items={items} />
      </div>
    </aside>
  );
}
