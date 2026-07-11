"use client";

import { ListTree } from "lucide-react";
import { useEffect, useMemo, useRef, useState } from "react";

import { Badge } from "@/components/ui/badge";
import { Card } from "@/components/ui/card";
import { Separator } from "@/components/ui/separator";
import { cn } from "@/lib/utils";
import type { TocItem } from "@/lib/toc";

type TableOfContentsProps = {
  items: TocItem[];
  variant: "mobile" | "desktop";
  className?: string;
};

type TocSection = {
  heading: TocItem;
  children: TocItem[];
};

const STICKY_TOP = 16;
const ACTIVE_OFFSET = 112;

function groupTocSections(items: TocItem[]) {
  const sections: TocSection[] = [];

  for (const item of items) {
    if (item.depth === 2) {
      sections.push({ heading: item, children: [] });
      continue;
    }

    sections[sections.length - 1]?.children.push(item);
  }

  return sections;
}

function TocLink({
  item,
  number,
  isActive,
  isMuted,
  className,
}: {
  item: TocItem;
  number?: number;
  isActive?: boolean;
  isMuted?: boolean;
  className?: string;
}) {
  return (
    <a
      href={`#${item.id}`}
      title={item.text}
      aria-current={isActive ? "location" : undefined}
      className={cn(
        "group flex min-w-0 items-baseline gap-2 rounded-md px-2 py-1.5 text-sm leading-snug text-muted-foreground transition-colors hover:bg-muted/60 hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
        item.depth === 3 && "ml-8 text-xs",
        isMuted && "text-muted-foreground hover:text-foreground",
        isActive && "bg-muted/70 text-foreground shadow-sm",
        className
      )}
    >
      {number !== undefined && (
        <span
          className={cn(
            "font-mono text-[10px] tabular-nums text-muted-foreground group-hover:text-foreground",
            isActive && "text-foreground/70"
          )}
        >
          {String(number).padStart(2, "0")}
        </span>
      )}
      <span className="min-w-0 truncate">{item.text}</span>
    </a>
  );
}

function TocLinks({
  items,
  activeItemId,
}: {
  items: TocItem[];
  activeItemId?: string;
}) {
  let sectionNumber = 0;

  return (
    <ol className="space-y-1.5">
      {items.map((item) => {
        const number = item.depth === 2 ? ++sectionNumber : null;

        return (
          <li key={item.id}>
            <TocLink
              item={item}
              number={number ?? undefined}
              isActive={activeItemId === item.id}
            />
          </li>
        );
      })}
    </ol>
  );
}

function useTocState({
  items,
  sections,
  cardRef,
  enabled,
}: {
  items: TocItem[];
  sections: TocSection[];
  cardRef: React.RefObject<HTMLDivElement | null>;
  enabled: boolean;
}) {
  const itemToSectionId = useMemo(() => {
    const map = new Map<string, string>();

    for (const section of sections) {
      map.set(section.heading.id, section.heading.id);
      for (const child of section.children) {
        map.set(child.id, section.heading.id);
      }
    }

    return map;
  }, [sections]);

  const [activeItemId, setActiveItemId] = useState(items[0]?.id ?? "");
  const [activeSectionId, setActiveSectionId] = useState(
    sections[0]?.heading.id ?? ""
  );
  const activeSectionIndex = Math.max(
    0,
    sections.findIndex((section) => section.heading.id === activeSectionId)
  );
  const [isContextMode, setIsContextMode] = useState(false);
  const [readingProgress, setReadingProgress] = useState(0);

  useEffect(() => {
    if (!enabled) return;

    let frame = 0;

    const update = () => {
      frame = 0;

      const documentElement = document.documentElement;
      const scrollableHeight =
        documentElement.scrollHeight - window.innerHeight;
      const nextProgress =
        scrollableHeight > 0
          ? Math.min(100, Math.max(0, (window.scrollY / scrollableHeight) * 100))
          : 0;

      setReadingProgress(nextProgress);

      if (cardRef.current) {
        const top = cardRef.current.getBoundingClientRect().top;
        setIsContextMode(top <= STICKY_TOP + 1);
      }

      let nextActiveItemId = items[0]?.id ?? "";

      for (const item of items) {
        const element = document.getElementById(item.id);
        if (!element) continue;

        if (element.getBoundingClientRect().top <= ACTIVE_OFFSET) {
          nextActiveItemId = item.id;
        } else {
          break;
        }
      }

      const nextActiveSectionId =
        itemToSectionId.get(nextActiveItemId) ?? sections[0]?.heading.id ?? "";

      setActiveItemId((current) =>
        current === nextActiveItemId ? current : nextActiveItemId
      );
      setActiveSectionId((current) =>
        current === nextActiveSectionId ? current : nextActiveSectionId
      );
    };

    const requestUpdate = () => {
      if (frame) return;
      frame = window.requestAnimationFrame(update);
    };

    update();
    window.addEventListener("scroll", requestUpdate, { passive: true });
    window.addEventListener("resize", requestUpdate);

    return () => {
      if (frame) window.cancelAnimationFrame(frame);
      window.removeEventListener("scroll", requestUpdate);
      window.removeEventListener("resize", requestUpdate);
    };
  }, [cardRef, enabled, itemToSectionId, items, sections]);

  return {
    activeItemId,
    activeSectionIndex,
    isContextMode,
    readingProgress,
  };
}

function ContextSectionLink({
  label,
  section,
  number,
}: {
  label: string;
  section: TocSection;
  number: number;
}) {
  return (
    <div className="space-y-1">
      <div className="px-2 text-[10px] font-medium uppercase tracking-wide text-muted-foreground">
        {label}
      </div>
      <TocLink
        item={section.heading}
        number={number}
        isMuted
        className="py-1 text-xs"
      />
    </div>
  );
}

function ContextToc({
  sections,
  activeItemId,
  activeSectionIndex,
}: {
  sections: TocSection[];
  activeItemId: string;
  activeSectionIndex: number;
}) {
  const previousSection = sections[activeSectionIndex - 1];
  const currentSection = sections[activeSectionIndex] ?? sections[0];
  const nextSection = sections[activeSectionIndex + 1];

  if (!currentSection) return null;

  return (
    <div className="space-y-3">
      {previousSection && (
        <ContextSectionLink
          label="Previous"
          section={previousSection}
          number={activeSectionIndex}
        />
      )}

      <div className="rounded-md border-l-2 border-primary bg-muted/55 p-2">
        <div className="mb-1 text-[10px] font-medium uppercase tracking-wide text-muted-foreground">
          Current
        </div>
        <TocLink
          item={currentSection.heading}
          number={activeSectionIndex + 1}
          isActive={activeItemId === currentSection.heading.id}
          className="px-0 py-1 text-foreground hover:bg-transparent"
        />

        {currentSection.children.length > 0 && (
          <ol className="mt-2 max-h-44 space-y-1 overflow-y-auto pr-1">
            {currentSection.children.map((child) => (
              <li key={child.id}>
                <TocLink
                  item={child}
                  isActive={activeItemId === child.id}
                  className="ml-0 px-2 py-1"
                />
              </li>
            ))}
          </ol>
        )}
      </div>

      {nextSection && (
        <ContextSectionLink
          label="Next"
          section={nextSection}
          number={activeSectionIndex + 2}
        />
      )}
    </div>
  );
}

export function TableOfContents({
  items,
  variant,
  className,
}: TableOfContentsProps) {
  const cardRef = useRef<HTMLDivElement>(null);
  const sections = useMemo(() => groupTocSections(items), [items]);
  const {
    activeItemId,
    activeSectionIndex,
    isContextMode,
    readingProgress,
  } = useTocState({
    items,
    sections,
    cardRef,
    enabled: variant === "desktop" && sections.length > 0,
  });

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
          <span className="font-mono text-[10px] text-muted-foreground">
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
      <Card
        ref={cardRef}
        className="sticky top-4 overflow-hidden border bg-background/70 shadow-sm backdrop-blur"
      >
        <div className="h-0.5 bg-muted">
          <div
            className="h-full bg-primary transition-[width]"
            style={{ width: `${readingProgress}%` }}
          />
        </div>

        <div className="p-3">
          <div className="mb-3 flex items-center justify-between gap-2">
            <div className="flex min-w-0 items-center gap-2 text-xs font-medium uppercase tracking-wide text-muted-foreground">
              <ListTree className="size-3.5 shrink-0" />
              <span>Contents</span>
            </div>
            <Badge
              variant="outline"
              className="shrink-0 px-1.5 py-0 text-[10px] font-medium text-muted-foreground"
            >
              {isContextMode && sections.length > 0
                ? `${activeSectionIndex + 1}/${sections.length}`
                : `${items.length} sections`}
            </Badge>
          </div>

          <Separator className="mb-3" />

          <div className="max-h-[calc(100vh-7rem)] overflow-y-auto pr-1">
            {isContextMode && sections.length > 0 ? (
              <ContextToc
                sections={sections}
                activeItemId={activeItemId}
                activeSectionIndex={activeSectionIndex}
              />
            ) : (
              <TocLinks items={items} activeItemId={activeItemId} />
            )}
          </div>
        </div>
      </Card>
    </aside>
  );
}
