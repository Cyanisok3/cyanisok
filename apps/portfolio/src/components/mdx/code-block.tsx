import { Children, isValidElement, type ComponentProps, type ReactNode } from "react";
import { bundledLanguages, codeToHtml } from "shiki/bundle/full";

import { cn } from "@/lib/utils";
import { resolveCodeLanguage } from "@/lib/code-language";
import { CodeCopyButton } from "./code-copy-button";

type CodeBlockProps = ComponentProps<"pre">;

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

function normalizeCodeText(text: string) {
  return text.replace(/(?:\r?\n)+$/, "");
}

function getCodeMetadata(children: ReactNode) {
  const codeElement = Children.toArray(children).find(
    (child) => isValidElement(child)
  );

  if (!isValidElement(codeElement)) {
    return {
      codeText: normalizeCodeText(getTextContent(children)),
      className: "",
      title: null,
    };
  }

  const props = codeElement.props as {
    children?: ReactNode;
    className?: string;
    "data-language"?: string;
    "data-title"?: string;
  };
  const languageClassName = props["data-language"]
    ? `language-${props["data-language"]}`
    : "";

  return {
    codeText: normalizeCodeText(getTextContent(props.children)),
    className: props.className || languageClassName,
    title: props["data-title"] || null,
  };
}

async function highlightCode(codeText: string, className: string) {
  if (!codeText) return "";

  try {
    const lang = resolveCodeLanguage(className, bundledLanguages);
    return await codeToHtml(codeText, {
      lang: lang as any,
      themes: {
        light: "github-light",
        dark: "github-dark",
      },
      structure: "inline",
    });
  } catch {
    return "";
  }
}

export async function CodeBlock({ children, ...props }: CodeBlockProps) {
  const { codeText, className, title } = getCodeMetadata(children);
  const html = await highlightCode(codeText, className);

  return (
    <div className="group relative overflow-hidden rounded-xl border border-border backdrop-blur-xs shadow-sm">
      <pre
        {...props}
        className={cn("p-0! m-0! overflow-x-auto", props.className)}
      >
        {title && (
          <div className="p-3 text-xs font-medium border-b border-border rounded-t-xl bg-muted/50 text-foreground">
            {title}
          </div>
        )}

        <CodeCopyButton codeText={codeText} hasTitle={Boolean(title)} />
        {html ? (
          <div className="p-3">
            <code
              className={`shiki ${className}`}
              dangerouslySetInnerHTML={{ __html: html }}
            />
          </div>
        ) : (
          <div className="p-4">
            <code className={cn("block whitespace-pre text-foreground/90", className)}>
              {codeText}
            </code>
          </div>
        )}
      </pre>
    </div>
  );
}
