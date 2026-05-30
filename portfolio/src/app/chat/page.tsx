"use client";

import {
  AlertCircle,
  Bot,
  History,
  ImageIcon,
  Loader2,
  LogOut,
  RefreshCcw,
  Send,
  ShieldCheck,
  Sparkles,
  UploadCloud,
  UserRound,
} from "lucide-react";
import { FormEvent, useCallback, useEffect, useState } from "react";
import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";

import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Separator } from "@/components/ui/separator";
import { cn } from "@/lib/utils";

type ApiError = Error & {
  status?: number;
};

type AuthMode = "login" | "register";
type ActiveView = "chat" | "image";

type ChatMessage = {
  id: string;
  role: "user" | "assistant";
  content: string;
};

type HistoryResponse = {
  success?: boolean;
  history?: Array<{
    is_user?: boolean;
    content?: string;
  }>;
};

type LoginResponse = {
  success?: boolean;
  userId?: number;
  username?: string;
};

type ChatResponse = {
  success?: boolean;
  information?: string;
  Information?: string;
  message?: string;
};

type UploadResponse = {
  success?: string;
  filename?: string;
  class_name?: string;
  confidence?: number;
  message?: string;
};

const API_ROOT = "/api/ai-chat";

function createId() {
  if (typeof crypto !== "undefined" && "randomUUID" in crypto) {
    return crypto.randomUUID();
  }

  return `${Date.now()}-${Math.random().toString(36).slice(2)}`;
}

async function readJson<T>(response: Response): Promise<T & { message?: string }> {
  const text = await response.text();
  if (!text) {
    return {} as T & { message?: string };
  }

  try {
    return JSON.parse(text) as T & { message?: string };
  } catch {
    return { message: text } as T & { message?: string };
  }
}

async function apiRequest<T>(
  path: string,
  init: RequestInit = {}
): Promise<T & { message?: string }> {
  const headers = new Headers(init.headers);
  if (init.body && !headers.has("content-type")) {
    headers.set("content-type", "application/json");
  }

  const response = await fetch(`${API_ROOT}/${path}`, {
    ...init,
    headers,
    credentials: "same-origin",
    cache: "no-store",
  });
  const data = await readJson<T>(response);

  if (!response.ok) {
    const error = new Error(
      data.message || `Request failed with status ${response.status}`
    ) as ApiError;
    error.status = response.status;
    throw error;
  }

  return data;
}

function isUnauthorized(error: unknown) {
  return Boolean(error && typeof error === "object" && "status" in error && error.status === 401);
}

function getErrorMessage(error: unknown) {
  if (error instanceof Error) {
    return error.message;
  }

  return "Something went wrong";
}

function readFileAsDataUrl(file: File) {
  return new Promise<string>((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(String(reader.result));
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}

export default function ChatPage() {
  const [authMode, setAuthMode] = useState<AuthMode>("login");
  const [activeView, setActiveView] = useState<ActiveView>("chat");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [isAuthed, setIsAuthed] = useState(false);
  const [authLoading, setAuthLoading] = useState(false);
  const [historyLoading, setHistoryLoading] = useState(false);
  const [sending, setSending] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [draft, setDraft] = useState("");
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [imagePreview, setImagePreview] = useState<string | null>(null);
  const [imageResult, setImageResult] = useState<string | null>(null);
  const [notice, setNotice] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const handleApiFailure = useCallback((caught: unknown, quiet = false) => {
    if (isUnauthorized(caught)) {
      setIsAuthed(false);
      setMessages([]);
      if (!quiet) {
        setError("Session expired. Please sign in again.");
      }
      return;
    }

    if (!quiet) {
      setError(getErrorMessage(caught));
    }
  }, []);

  const syncHistory = useCallback(
    async (quiet = false) => {
      if (!quiet) {
        setHistoryLoading(true);
        setError(null);
        setNotice(null);
      }

      try {
        const data = await apiRequest<HistoryResponse>("chat/history", {
          method: "POST",
          body: JSON.stringify({}),
        });

        const restoredMessages =
          data.history?.map((item) => ({
            id: createId(),
            role: item.is_user ? ("user" as const) : ("assistant" as const),
            content: item.content || "",
          })) ?? [];

        setMessages(restoredMessages);
        setIsAuthed(true);
        if (!quiet) {
          setNotice("History synced");
        }
      } catch (caught) {
        handleApiFailure(caught, quiet);
      } finally {
        if (!quiet) {
          setHistoryLoading(false);
        }
      }
    },
    [handleApiFailure]
  );

  useEffect(() => {
    void syncHistory(true);
  }, [syncHistory]);

  useEffect(() => {
    return () => {
      if (imagePreview) {
        URL.revokeObjectURL(imagePreview);
      }
    };
  }, [imagePreview]);

  async function handleAuthSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setAuthLoading(true);
    setError(null);
    setNotice(null);

    try {
      if (authMode === "register") {
        await apiRequest("register", {
          method: "POST",
          body: JSON.stringify({ username, password }),
        });
      }

      const login = await apiRequest<LoginResponse>("login", {
        method: "POST",
        body: JSON.stringify({ username, password }),
      });

      if (!login.userId && login.success === false) {
        throw new Error(login.message || "Unable to sign in");
      }

      setIsAuthed(true);
      setPassword("");
      setNotice(authMode === "register" ? "Account ready" : "Signed in");
      await syncHistory(true);
    } catch (caught) {
      handleApiFailure(caught);
    } finally {
      setAuthLoading(false);
    }
  }

  async function handleLogout() {
    setError(null);
    setNotice(null);

    try {
      await apiRequest("logout", {
        method: "POST",
        body: JSON.stringify({}),
      });
    } catch {
      // The local session still gets cleared so the page does not trap the user.
    } finally {
      setIsAuthed(false);
      setMessages([]);
      setImageResult(null);
      setSelectedFile(null);
      setNotice("Signed out");
    }
  }

  async function handleSend(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    const question = draft.trim();
    if (!question || sending) return;

    setSending(true);
    setError(null);
    setNotice(null);
    setDraft("");

    const userMessage: ChatMessage = {
      id: createId(),
      role: "user",
      content: question,
    };
    setMessages((current) => [...current, userMessage]);

    try {
      const data = await apiRequest<ChatResponse>("chat/send", {
        method: "POST",
        body: JSON.stringify({ question }),
      });
      const reply = data.information || data.Information || data.message;

      setMessages((current) => [
        ...current,
        {
          id: createId(),
          role: "assistant",
          content: reply || "The service returned an empty response.",
        },
      ]);
    } catch (caught) {
      handleApiFailure(caught);
      setMessages((current) => [
        ...current,
        {
          id: createId(),
          role: "assistant",
          content: `[Error] ${getErrorMessage(caught)}`,
        },
      ]);
    } finally {
      setSending(false);
    }
  }

  function handleFileChange(file: File | null) {
    setSelectedFile(file);
    setImageResult(null);

    if (imagePreview) {
      URL.revokeObjectURL(imagePreview);
      setImagePreview(null);
    }

    if (file) {
      setImagePreview(URL.createObjectURL(file));
    }
  }

  async function handleUpload(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    if (!selectedFile || uploading) return;

    if (selectedFile.size > 5 * 1024 * 1024) {
      setError("Image must be 5MB or smaller.");
      return;
    }

    setUploading(true);
    setError(null);
    setNotice(null);

    try {
      const dataUrl = await readFileAsDataUrl(selectedFile);
      const image = dataUrl.split(",")[1];
      const data = await apiRequest<UploadResponse>("upload/send", {
        method: "POST",
        body: JSON.stringify({
          filename: selectedFile.name,
          image,
        }),
      });

      if (!data.class_name) {
        throw new Error(data.message || "No recognition result returned");
      }

      const confidence =
        typeof data.confidence === "number"
          ? ` (${Math.round(data.confidence * 100)}%)`
          : "";
      setImageResult(`${data.class_name}${confidence}`);
    } catch (caught) {
      handleApiFailure(caught);
    } finally {
      setUploading(false);
    }
  }

  return (
    <section className="min-h-[calc(100vh-12rem)] flex flex-col gap-6">
      <div className="flex flex-col gap-4">
        <Badge variant="outline" className="w-fit gap-2 bg-background/70">
          <Sparkles className="size-3.5" />
          AI Chat Service
        </Badge>
        <div className="flex items-start justify-between gap-4">
          <div className="space-y-2">
            <h1 className="text-3xl font-semibold">Chat</h1>
            <p className="text-sm text-muted-foreground">
              C++ muduo backend, Next.js interface.
            </p>
          </div>
          {isAuthed && (
            <Button variant="outline" size="sm" className="gap-2" onClick={handleLogout}>
              <LogOut className="size-4" />
              Logout
            </Button>
          )}
        </div>
      </div>

      {(error || notice) && (
        <div
          className={cn(
            "flex items-start gap-3 rounded-lg border bg-background/80 px-4 py-3 text-sm shadow-sm",
            error ? "border-destructive/40 text-destructive" : "border-border text-muted-foreground"
          )}
        >
          {error ? (
            <AlertCircle className="mt-0.5 size-4 flex-none" />
          ) : (
            <ShieldCheck className="mt-0.5 size-4 flex-none" />
          )}
          <p>{error || notice}</p>
        </div>
      )}

      {!isAuthed ? (
        <div className="rounded-lg border bg-card/80 p-5 shadow-sm backdrop-blur">
          <div className="mb-5 flex rounded-md border bg-background p-1">
            {(["login", "register"] as const).map((mode) => (
              <button
                key={mode}
                type="button"
                className={cn(
                  "h-9 flex-1 rounded-sm px-3 text-sm font-medium transition-colors",
                  authMode === mode
                    ? "bg-primary text-primary-foreground"
                    : "text-muted-foreground hover:bg-muted hover:text-foreground"
                )}
                onClick={() => {
                  setAuthMode(mode);
                  setError(null);
                  setNotice(null);
                }}
              >
                {mode === "login" ? "Sign in" : "Create account"}
              </button>
            ))}
          </div>

          <form className="flex flex-col gap-4" onSubmit={handleAuthSubmit}>
            <label className="flex flex-col gap-2 text-sm font-medium">
              Username
              <input
                value={username}
                onChange={(event) => setUsername(event.target.value)}
                className="h-10 rounded-md border bg-background px-3 text-sm outline-none transition-colors focus:border-ring"
                minLength={3}
                maxLength={32}
                autoComplete="username"
                required
              />
            </label>
            <label className="flex flex-col gap-2 text-sm font-medium">
              Password
              <input
                value={password}
                onChange={(event) => setPassword(event.target.value)}
                className="h-10 rounded-md border bg-background px-3 text-sm outline-none transition-colors focus:border-ring"
                minLength={8}
                maxLength={128}
                type="password"
                autoComplete={authMode === "login" ? "current-password" : "new-password"}
                required
              />
            </label>
            <Button type="submit" className="gap-2" disabled={authLoading}>
              {authLoading ? (
                <Loader2 className="size-4 animate-spin" />
              ) : (
                <ShieldCheck className="size-4" />
              )}
              {authMode === "login" ? "Sign in" : "Create account"}
            </Button>
          </form>
        </div>
      ) : (
        <div className="rounded-lg border bg-card/80 shadow-sm backdrop-blur">
          <div className="flex flex-wrap items-center justify-between gap-3 p-4">
            <div className="flex rounded-md border bg-background p-1">
              <button
                type="button"
                className={cn(
                  "inline-flex h-9 items-center gap-2 rounded-sm px-3 text-sm font-medium transition-colors",
                  activeView === "chat"
                    ? "bg-primary text-primary-foreground"
                    : "text-muted-foreground hover:bg-muted hover:text-foreground"
                )}
                onClick={() => setActiveView("chat")}
              >
                <Bot className="size-4" />
                Chat
              </button>
              <button
                type="button"
                className={cn(
                  "inline-flex h-9 items-center gap-2 rounded-sm px-3 text-sm font-medium transition-colors",
                  activeView === "image"
                    ? "bg-primary text-primary-foreground"
                    : "text-muted-foreground hover:bg-muted hover:text-foreground"
                )}
                onClick={() => setActiveView("image")}
              >
                <ImageIcon className="size-4" />
                Image
              </button>
            </div>

            {activeView === "chat" && (
              <Button
                type="button"
                variant="outline"
                size="sm"
                className="gap-2"
                disabled={historyLoading}
                onClick={() => void syncHistory(false)}
              >
                {historyLoading ? (
                  <Loader2 className="size-4 animate-spin" />
                ) : (
                  <RefreshCcw className="size-4" />
                )}
                Sync
              </Button>
            )}
          </div>

          <Separator />

          {activeView === "chat" ? (
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

              <form className="flex gap-3 p-4" onSubmit={handleSend}>
                <textarea
                  value={draft}
                  onChange={(event) => setDraft(event.target.value)}
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
          ) : (
            <form className="flex min-h-[34rem] flex-col gap-5 p-4" onSubmit={handleUpload}>
              <label className="flex min-h-48 cursor-pointer flex-col items-center justify-center gap-3 rounded-lg border border-dashed bg-background/80 p-6 text-center transition-colors hover:bg-muted/50">
                <UploadCloud className="size-8 text-muted-foreground" />
                <span className="text-sm font-medium">
                  {selectedFile ? selectedFile.name : "Choose image"}
                </span>
                <input
                  className="sr-only"
                  type="file"
                  accept="image/*"
                  onChange={(event) => handleFileChange(event.target.files?.[0] ?? null)}
                />
              </label>

              {imagePreview && (
                <div className="overflow-hidden rounded-lg border bg-background">
                  {/* eslint-disable-next-line @next/next/no-img-element */}
                  <img
                    src={imagePreview}
                    alt={selectedFile?.name || "Selected image"}
                    className="max-h-80 w-full object-contain"
                  />
                </div>
              )}

              {imageResult && (
                <div className="rounded-lg border bg-background px-4 py-3">
                  <p className="text-xs uppercase text-muted-foreground">Recognition</p>
                  <p className="mt-1 text-lg font-semibold">{imageResult}</p>
                </div>
              )}

              <Button type="submit" className="gap-2" disabled={!selectedFile || uploading}>
                {uploading ? (
                  <Loader2 className="size-4 animate-spin" />
                ) : (
                  <ImageIcon className="size-4" />
                )}
                Recognize
              </Button>
            </form>
          )}
        </div>
      )}
    </section>
  );
}
