"use client";

import {
  Children,
  isValidElement,
  useEffect,
  useMemo,
  useState,
  type ComponentProps,
  type ReactNode,
} from "react";
import { Copy, Check } from "lucide-react";
import { Button } from "../ui/button";
import { codeToHtml } from "shiki/bundle/web";
import { cn } from "@/lib/utils";

type CodeBlockProps = ComponentProps<"pre">;

function extractLanguage(className?: string): string {
  if (!className) return "plaintext";
  const match = className.match(/language-([a-z0-9-]+)/i);
  return match ? match[1] : "plaintext";
}

function getTextContent(node: ReactNode): string {
  if (typeof node === "string" || typeof node === "number") {
    return String(node);
  }

  if (Array.isArray(node)) {
    return node.map(getTextContent).join("");
  }

  if (isValidElement(node)) {
    return getTextContent((node.props as { children?: ReactNode }).children);
  }

  return "";
}

function getCodeMetadata(children: ReactNode) {
  const codeElement = Children.toArray(children).find(
    (child) => isValidElement(child) && child.type === "code"
  );

  if (!isValidElement(codeElement)) {
    return {
      codeText: getTextContent(children),
      className: "",
      title: null,
    };
  }

  const props = codeElement.props as {
    children?: ReactNode;
    className?: string;
    "data-title"?: string;
  };

  return {
    codeText: getTextContent(props.children),
    className: props.className || "",
    title: props["data-title"] || null,
  };
}

export function CodeBlock({ children, ...props }: CodeBlockProps) {
  const [copied, setCopied] = useState(false);
  const { codeText, className, title } = useMemo(
    () => getCodeMetadata(children),
    [children]
  );
  const highlightSignature = `${className}\u0000${codeText}`;
  const [{ html, signature }, setHighlight] = useState<{
    html: string;
    signature: string;
  }>({ html: "", signature: "" });
  const isCurrentHighlight = signature === highlightSignature;
  const isLoading = !isCurrentHighlight;

  useEffect(() => {
    let cancelled = false;
    const lang = extractLanguage(className);

    void codeToHtml(codeText, {
      lang: lang as any,
      themes: {
        light: "github-light",
        dark: "github-dark",
      },
      defaultColor: false,
    })
      .then((html) => {
        if (cancelled) return;

        const parser = new DOMParser();
        const doc = parser.parseFromString(html, "text/html");
        setHighlight({
          html: doc.querySelector("code")?.innerHTML ?? "",
          signature: highlightSignature,
        });
      })
      .catch((error) => {
        if (cancelled) return;

        console.error("Failed to highlight code:", error);
        setHighlight({
          html: "",
          signature: highlightSignature,
        });
      });

    return () => {
      cancelled = true;
    };
  }, [className, codeText, highlightSignature]);

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(codeText);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch (error) {
      console.error("Failed to copy code:", error);
    }
  };

  return (
    <div className="group relative rounded-xl overflow-hidden border border-border">
      <pre
        {...props}
        className={cn("p-0! m-0! overflow-x-auto", props.className)}
      >
        {title && (
          <div className="p-3 text-xs font-medium border-b border-border rounded-t-xl bg-muted/50 text-foreground">
            {title}
          </div>
        )}

        <Button
          onClick={handleCopy}
          variant="outline"
          size="icon"
          className={cn("absolute size-8 text-primary cursor-pointer right-3 opacity-100 lg:opacity-0 lg:group-hover:opacity-100 transition-opacity rounded-md border border-border shadow-none", title ? "top-13" : "top-3", props.className)}
          aria-label="Copy code"
        >
          {copied ? <Check className="size-4" /> : <Copy className="size-4" />}
        </Button>
        {isCurrentHighlight && html ? (
          <div className="p-3">
            <code
              className={`shiki ${className}`}
              dangerouslySetInnerHTML={{ __html: html }}
            />
          </div>
        ) : isLoading ? (
          <div className="space-y-2 p-4" aria-hidden="true">
            <div className="h-3 w-11/12 animate-pulse rounded-full bg-muted" />
            <div className="h-3 w-9/12 animate-pulse rounded-full bg-muted" />
            <div className="h-3 w-10/12 animate-pulse rounded-full bg-muted" />
            <div className="h-3 w-7/12 animate-pulse rounded-full bg-muted" />
          </div>
        ) : (
          <div className="p-4">
            <code className={cn("block whitespace-pre text-foreground/90", className)}>
              {codeText}
            </code>
          </div>
        )}
      </pre >
    </div >
  );
}
