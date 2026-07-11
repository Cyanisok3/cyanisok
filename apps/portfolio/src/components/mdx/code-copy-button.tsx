"use client";

import { useEffect, useRef, useState } from "react";
import { Check, Copy } from "lucide-react";

import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";

type CodeCopyButtonProps = {
  codeText: string;
  hasTitle: boolean;
};

export function CodeCopyButton({ codeText, hasTitle }: CodeCopyButtonProps) {
  const [copied, setCopied] = useState(false);
  const resetTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    return () => {
      if (resetTimerRef.current) clearTimeout(resetTimerRef.current);
    };
  }, []);

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(codeText);
      setCopied(true);
      if (resetTimerRef.current) clearTimeout(resetTimerRef.current);
      resetTimerRef.current = setTimeout(() => setCopied(false), 2000);
    } catch (error) {
      console.error("Failed to copy code:", error);
    }
  };

  return (
    <>
      <Button
        type="button"
        onClick={handleCopy}
        variant="outline"
        size="icon"
        className={cn(
          "absolute right-3 size-8 cursor-pointer rounded-md border border-border text-primary opacity-100 shadow-none transition-opacity lg:opacity-0 lg:group-hover:opacity-100",
          hasTitle ? "top-13" : "top-3"
        )}
        aria-label={copied ? "Code copied" : "Copy code"}
      >
        {copied ? (
          <Check className="size-4" aria-hidden />
        ) : (
          <Copy className="size-4" aria-hidden />
        )}
      </Button>
      <span className="sr-only" role="status" aria-live="polite">
        {copied ? "Code copied to clipboard" : ""}
      </span>
    </>
  );
}
