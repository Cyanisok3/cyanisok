"use client";

import type { FormEvent, KeyboardEvent } from "react";
import { useEffect, useRef } from "react";
import { Bot, History, Send, UserRound } from "lucide-react";
import { useReducedMotion } from "motion/react";
import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";

import { Button } from "@/components/ui/button";
import { Separator } from "@/components/ui/separator";
import { cn } from "@/lib/utils";

import type { ChatMessage } from "../types";
import { CHAT_MESSAGE_MAX_BYTES, getUtf8ByteLength } from "../utils";

type ChatPanelProps = {
  messages: ChatMessage[];
  draft: string;
  sending: boolean;
  announcement: string;
  scrollToLatestKey: number;
  onDraftChange: (value: string) => void;
  onSend: (event: FormEvent<HTMLFormElement>) => void;
  onPromptSelect: (value: string) => void;
};

const promptSuggestions = [
  "Help me improve this portfolio page.",
  "Explain a C++ concept with a small example.",
  "Give me a focused study plan for algorithms.",
  "Review this idea and list the tradeoffs.",
];

export function ChatPanel({
  messages,
  draft,
  sending,
  announcement,
  scrollToLatestKey,
  onDraftChange,
  onSend,
  onPromptSelect,
}: ChatPanelProps) {
  const scrollAreaRef = useRef<HTMLDivElement>(null);
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const isNearBottomRef = useRef(true);
  const shouldReduceMotion = useReducedMotion();
  const shouldReduceMotionRef = useRef(shouldReduceMotion);
  const trimmedDraft = draft.trim();
  const messageBytes = getUtf8ByteLength(trimmedDraft);
  const isOverLimit = messageBytes > CHAT_MESSAGE_MAX_BYTES;

  useEffect(() => {
    shouldReduceMotionRef.current = shouldReduceMotion;
  }, [shouldReduceMotion]);

  useEffect(() => {
    const textarea = textareaRef.current;
    if (!textarea) return;

    textarea.style.height = "auto";
    textarea.style.height = `${Math.min(textarea.scrollHeight, 160)}px`;
  }, [draft]);

  useEffect(() => {
    const scrollArea = scrollAreaRef.current;
    if (!scrollArea || !isNearBottomRef.current) return;

    scrollArea.scrollTo({
      top: scrollArea.scrollHeight,
      behavior: "auto",
    });
  }, [messages, sending]);

  useEffect(() => {
    const scrollArea = scrollAreaRef.current;
    if (!scrollArea) return;

    isNearBottomRef.current = true;
    scrollArea.scrollTo({
      top: scrollArea.scrollHeight,
      behavior: shouldReduceMotionRef.current ? "auto" : "smooth",
    });
  }, [scrollToLatestKey]);

  function handleComposerKeyDown(event: KeyboardEvent<HTMLTextAreaElement>) {
    if (event.key !== "Enter" || event.shiftKey || event.nativeEvent.isComposing) {
      return;
    }

    event.preventDefault();
    event.currentTarget.form?.requestSubmit();
  }

  return (
    <div className="flex h-[clamp(30rem,68dvh,42rem)] min-h-0 flex-col">
      <p className="sr-only" role="status" aria-live="polite">
        {announcement}
      </p>
      <div
        ref={scrollAreaRef}
        className="min-h-0 flex-1 space-y-5 overflow-y-auto p-4"
        onScroll={(event) => {
          const area = event.currentTarget;
          isNearBottomRef.current =
            area.scrollHeight - area.scrollTop - area.clientHeight <= 96;
        }}
      >
        {messages.length === 0 ? (
          <div className="flex min-h-72 flex-col items-center justify-center gap-5 text-center">
            <div className="flex size-11 items-center justify-center rounded-full border bg-background shadow-sm">
              <History className="size-5 text-muted-foreground" aria-hidden />
            </div>
            <div className="space-y-1">
              <p className="text-sm font-medium text-foreground">
                Start with a focused prompt.
              </p>
              <p className="text-sm text-muted-foreground">
                Choose one below or write your own message.
              </p>
            </div>
            <div className="grid w-full max-w-xl grid-cols-1 gap-2 sm:grid-cols-2">
              {promptSuggestions.map((prompt) => (
                <button
                  key={prompt}
                  type="button"
                  className="rounded-lg border bg-background/80 px-3 py-2 text-left text-sm text-foreground/85 transition-colors hover:bg-muted focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring"
                  onClick={() => onPromptSelect(prompt)}
                >
                  {prompt}
                </button>
              ))}
            </div>
          </div>
        ) : (
          messages.map((message) => (
            <div
              key={message.id}
              className={cn(
                "flex gap-3",
                message.role === "user" ? "justify-end" : "justify-start"
              )}
            >
              {message.role === "assistant" && (
                <div className="mt-1 flex size-8 flex-none items-center justify-center rounded-full border bg-background">
                  <Bot className="size-4" aria-hidden />
                </div>
              )}
              <div
                className={cn(
                  "rounded-lg border px-4 py-3 text-sm leading-relaxed",
                  message.role === "user"
                    ? "max-w-[80%] bg-primary text-primary-foreground shadow-sm"
                    : "max-w-[92%] bg-background/85 text-foreground shadow-none"
                )}
              >
                {message.role === "assistant" ? (
                  message.content ? (
                    <div className="prose prose-sm max-w-none dark:prose-invert prose-p:my-2 prose-pre:my-3">
                      <ReactMarkdown remarkPlugins={[remarkGfm]}>
                        {message.content}
                      </ReactMarkdown>
                    </div>
                  ) : (
                    <div className="flex h-5 items-center gap-1" aria-hidden>
                      <span className="size-1.5 animate-pulse rounded-full bg-muted-foreground" />
                      <span className="size-1.5 animate-pulse rounded-full bg-muted-foreground delay-150" />
                      <span className="size-1.5 animate-pulse rounded-full bg-muted-foreground delay-300" />
                    </div>
                  )
                ) : (
                  <p className="whitespace-pre-wrap">{message.content}</p>
                )}
              </div>
              {message.role === "user" && (
                <div className="mt-1 flex size-8 flex-none items-center justify-center rounded-full border bg-background">
                  <UserRound className="size-4" aria-hidden />
                </div>
              )}
            </div>
          ))
        )}
      </div>

      <Separator />

      <form className="p-4" onSubmit={onSend}>
        <label
          htmlFor="chat-message-composer"
          className="mb-2 block text-xs font-medium text-muted-foreground"
        >
          Message
        </label>
        <div className="flex items-end gap-3">
          <textarea
            id="chat-message-composer"
            ref={textareaRef}
            value={draft}
            onChange={(event) => onDraftChange(event.target.value)}
            onKeyDown={handleComposerKeyDown}
            className="max-h-40 min-h-11 flex-1 resize-none rounded-md border bg-background px-3 py-2 text-sm leading-relaxed outline-none transition-colors focus:border-ring focus-visible:ring-2 focus-visible:ring-ring"
            maxLength={CHAT_MESSAGE_MAX_BYTES}
            placeholder="Ask anything about code, study plans, or this site."
            rows={1}
            aria-describedby="chat-message-limit"
            aria-invalid={isOverLimit}
            required
          />
          <Button
            type="submit"
            size="icon"
            disabled={sending || !trimmedDraft || isOverLimit}
            aria-label="Send message"
          >
            <Send className="size-4" aria-hidden />
          </Button>
        </div>
        <div className="mt-2 flex flex-wrap items-center justify-between gap-2 text-xs text-muted-foreground">
          <span>Enter to send · Shift+Enter for a new line</span>
          <span
            id="chat-message-limit"
            className={cn(isOverLimit && "font-medium text-destructive")}
          >
            {isOverLimit
              ? `${messageBytes - CHAT_MESSAGE_MAX_BYTES} bytes over limit`
              : `${messageBytes}/${CHAT_MESSAGE_MAX_BYTES} UTF-8 bytes`}
          </span>
        </div>
      </form>
    </div>
  );
}
