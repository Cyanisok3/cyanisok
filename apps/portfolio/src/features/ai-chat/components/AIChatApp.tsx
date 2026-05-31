"use client";

import type { FormEvent } from "react";
import { useCallback, useEffect, useState } from "react";
import {
  Bot,
  ImageIcon,
  Loader2,
  LogOut,
  RefreshCcw,
  Sparkles,
} from "lucide-react";

import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Separator } from "@/components/ui/separator";
import { cn } from "@/lib/utils";

import { apiRequest } from "../api";
import { sendStreamingChat } from "../sse";
import type {
  ActiveView,
  AuthMode,
  ChatMessage,
  HistoryResponse,
  LoginResponse,
  UploadResponse,
} from "../types";
import {
  appendMessageContent,
  createId,
  getErrorMessage,
  isUnauthorized,
  readFileAsDataUrl,
  replaceMessageContent,
} from "../utils";
import { AuthPanel } from "./AuthPanel";
import { ChatPanel } from "./ChatPanel";
import { ImagePanel } from "./ImagePanel";
import { StatusBanner } from "./StatusBanner";

export function AIChatApp() {
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

  function handleAuthModeChange(mode: AuthMode) {
    setAuthMode(mode);
    setError(null);
    setNotice(null);
  }

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
      if (imagePreview) {
        URL.revokeObjectURL(imagePreview);
      }
      setIsAuthed(false);
      setMessages([]);
      setImageResult(null);
      setImagePreview(null);
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
    const assistantMessageId = createId();
    const assistantMessage: ChatMessage = {
      id: assistantMessageId,
      role: "assistant",
      content: "",
    };
    setMessages((current) => [...current, userMessage, assistantMessage]);

    try {
      const reply = await sendStreamingChat(question, (content) => {
        appendMessageContent(setMessages, assistantMessageId, content);
      });

      if (reply) {
        replaceMessageContent(setMessages, assistantMessageId, reply);
      } else {
        replaceMessageContent(
          setMessages,
          assistantMessageId,
          "The service returned an empty response."
        );
      }
    } catch (caught) {
      handleApiFailure(caught);
      if (!isUnauthorized(caught)) {
        setMessages((current) =>
          current.map((message) =>
            message.id === assistantMessageId
              ? {
                  ...message,
                  content: `${message.content}${message.content ? "\n\n" : ""}[Error] ${getErrorMessage(caught)}`,
                }
              : message
          )
        );
      }
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
            <Button
              variant="outline"
              size="sm"
              className="gap-2"
              onClick={handleLogout}
            >
              <LogOut className="size-4" />
              Logout
            </Button>
          )}
        </div>
      </div>

      <StatusBanner error={error} notice={notice} />

      {!isAuthed ? (
        <AuthPanel
          authMode={authMode}
          username={username}
          password={password}
          authLoading={authLoading}
          onAuthModeChange={handleAuthModeChange}
          onUsernameChange={setUsername}
          onPasswordChange={setPassword}
          onSubmit={handleAuthSubmit}
        />
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
            <ChatPanel
              messages={messages}
              draft={draft}
              sending={sending}
              onDraftChange={setDraft}
              onSend={handleSend}
            />
          ) : (
            <ImagePanel
              selectedFile={selectedFile}
              imagePreview={imagePreview}
              imageResult={imageResult}
              uploading={uploading}
              onFileChange={handleFileChange}
              onUpload={handleUpload}
            />
          )}
        </div>
      )}
    </section>
  );
}
