"use client";

import { useState } from "react";
import { Check, Copy } from "lucide-react";

import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";

type CodeCopyButtonProps = {
  codeText: string;
  hasTitle: boolean;
};

export function CodeCopyButton({ codeText, hasTitle }: CodeCopyButtonProps) {
  const [copied, setCopied] = useState(false);

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
    <Button
      onClick={handleCopy}
      variant="outline"
      size="icon"
      className={cn(
        "absolute right-3 size-8 cursor-pointer rounded-md border border-border text-primary opacity-100 shadow-none transition-opacity lg:opacity-0 lg:group-hover:opacity-100",
        hasTitle ? "top-13" : "top-3"
      )}
      aria-label="Copy code"
    >
      {copied ? <Check className="size-4" /> : <Copy className="size-4" />}
    </Button>
  );
}
