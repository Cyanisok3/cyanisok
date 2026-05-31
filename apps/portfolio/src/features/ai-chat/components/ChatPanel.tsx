import type { FormEvent } from "react";
import { Bot, History, Loader2, Send, UserRound } from "lucide-react";
import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";

import { Button } from "@/components/ui/button";
import { Separator } from "@/components/ui/separator";
import { cn } from "@/lib/utils";

import type { ChatMessage } from "../types";

type ChatPanelProps = {
  messages: ChatMessage[];
  draft: string;
  sending: boolean;
  onDraftChange: (value: string) => void;
  onSend: (event: FormEvent<HTMLFormElement>) => void;
};

export function ChatPanel({
  messages,
  draft,
  sending,
  onDraftChange,
  onSend,
}: ChatPanelProps) {
  return (
    <div className="flex min-h-[34rem] flex-col">
      <div className="flex-1 space-y-4 overflow-y-auto p-4">
        {messages.length === 0 ? (
          <div className="flex h-64 flex-col items-center justify-center gap-3 text-center text-sm text-muted-foreground">
            <History className="size-8" />
            <p>No messages yet.</p>
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
                  <Bot className="size-4" />
                </div>
              )}
              <div
                className={cn(
                  "max-w-[82%] rounded-lg border px-4 py-3 text-sm leading-relaxed shadow-sm",
                  message.role === "user"
                    ? "bg-primary text-primary-foreground"
                    : "bg-background text-foreground"
                )}
              >
                {message.role === "assistant" ? (
                  <div className="prose prose-sm max-w-none dark:prose-invert">
                    <ReactMarkdown remarkPlugins={[remarkGfm]}>
                      {message.content}
                    </ReactMarkdown>
                  </div>
                ) : (
                  <p className="whitespace-pre-wrap">{message.content}</p>
                )}
              </div>
              {message.role === "user" && (
                <div className="mt-1 flex size-8 flex-none items-center justify-center rounded-full border bg-background">
                  <UserRound className="size-4" />
                </div>
              )}
            </div>
          ))
        )}
        {sending && (
          <div className="flex items-center gap-3 text-sm text-muted-foreground">
            <Loader2 className="size-4 animate-spin" />
            Thinking
          </div>
        )}
      </div>

      <Separator />

      <form className="flex gap-3 p-4" onSubmit={onSend}>
        <textarea
          value={draft}
          onChange={(event) => onDraftChange(event.target.value)}
          className="min-h-11 flex-1 resize-none rounded-md border bg-background px-3 py-2 text-sm outline-none transition-colors focus:border-ring"
          maxLength={8000}
          placeholder="Message"
          rows={1}
          required
        />
        <Button type="submit" size="icon" disabled={sending || !draft.trim()}>
          {sending ? (
            <Loader2 className="size-4 animate-spin" />
          ) : (
            <Send className="size-4" />
          )}
          <span className="sr-only">Send</span>
        </Button>
      </form>
    </div>
  );
}
