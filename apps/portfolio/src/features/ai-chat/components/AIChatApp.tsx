"use client";

import type { FormEvent, KeyboardEvent } from "react";
import { useCallback, useEffect, useRef, useState } from "react";
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
  ChatMessage,
  HistoryResponse,
  LoginResponse,
  UploadResponse,
} from "../types";
import {
  appendMessageContent,
  CHAT_MESSAGE_MAX_BYTES,
  createId,
  getErrorMessage,
  getUtf8ByteLength,
  isAbortError,
  isSupportedImageFile,
  isUnauthorized,
  readFileAsDataUrl,
  replaceMessageContent,
} from "../utils";
import { AuthPanel } from "./AuthPanel";
import { ChatPanel } from "./ChatPanel";
import { ImagePanel } from "./ImagePanel";
import { StatusBanner } from "./StatusBanner";

const TAB_IDS: Record<ActiveView, string> = {
  chat: "chat-tool-tab-chat",
  image: "chat-tool-tab-image",
};

const PANEL_IDS: Record<ActiveView, string> = {
  chat: "chat-tool-panel-chat",
  image: "chat-tool-panel-image",
};

export function AIChatApp() {
  const [activeView, setActiveView] = useState<ActiveView>("chat");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [isAuthed, setIsAuthed] = useState(false);
  const [authChecking, setAuthChecking] = useState(true);
  const [authLoading, setAuthLoading] = useState(false);
  const [historyLoading, setHistoryLoading] = useState(false);
  const [sending, setSending] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [draft, setDraft] = useState("");
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [imagePreview, setImagePreview] = useState<string | null>(null);
  const [imageResult, setImageResult] = useState<string | null>(null);
  const [imageConfidence, setImageConfidence] = useState<number | null>(null);
  const [imageError, setImageError] = useState<string | null>(null);
  const [notice, setNotice] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [chatAnnouncement, setChatAnnouncement] = useState("");
  const [scrollToLatestKey, setScrollToLatestKey] = useState(0);

  const sessionGenerationRef = useRef(0);
  const probeGenerationRef = useRef(0);
  const historyGenerationRef = useRef(0);
  const authGenerationRef = useRef(0);
  const streamGenerationRef = useRef(0);
  const uploadGenerationRef = useRef(0);

  const probeControllerRef = useRef<AbortController | null>(null);
  const historyControllerRef = useRef<AbortController | null>(null);
  const authControllerRef = useRef<AbortController | null>(null);
  const streamControllerRef = useRef<AbortController | null>(null);
  const uploadControllerRef = useRef<AbortController | null>(null);
  const deltaFrameRef = useRef<number | null>(null);
  const pendingDeltaRef = useRef("");

  const cancelAllRequests = useCallback(() => {
    sessionGenerationRef.current += 1;
    probeGenerationRef.current += 1;
    historyGenerationRef.current += 1;
    authGenerationRef.current += 1;
    streamGenerationRef.current += 1;
    uploadGenerationRef.current += 1;

    probeControllerRef.current?.abort();
    historyControllerRef.current?.abort();
    authControllerRef.current?.abort();
    streamControllerRef.current?.abort();
    uploadControllerRef.current?.abort();

    probeControllerRef.current = null;
    historyControllerRef.current = null;
    authControllerRef.current = null;
    streamControllerRef.current = null;
    uploadControllerRef.current = null;

    if (deltaFrameRef.current !== null) {
      window.cancelAnimationFrame(deltaFrameRef.current);
      deltaFrameRef.current = null;
    }
    pendingDeltaRef.current = "";
  }, []);

  const clearLocalSession = useCallback(() => {
    setIsAuthed(false);
    setAuthChecking(false);
    setAuthLoading(false);
    setHistoryLoading(false);
    setSending(false);
    setUploading(false);
    setMessages([]);
    setDraft("");
    setImageResult(null);
    setImageConfidence(null);
    setImageError(null);
    setImagePreview(null);
    setSelectedFile(null);
    setChatAnnouncement("");
  }, []);

  const handleApiFailure = useCallback(
    (caught: unknown, quiet = false) => {
      if (isAbortError(caught)) return;

      if (isUnauthorized(caught)) {
        cancelAllRequests();
        clearLocalSession();
        if (!quiet) {
          setError("Session expired. Please sign in again.");
        }
        return;
      }

      if (!quiet) {
        setError(getErrorMessage(caught));
      }
    },
    [cancelAllRequests, clearLocalSession]
  );

  const restoreHistory = useCallback((data: HistoryResponse) => {
    const restoredMessages =
      data.history?.map((item) => ({
        id: createId(),
        role: item.is_user ? ("user" as const) : ("assistant" as const),
        content: item.content || "",
      })) ?? [];

    setMessages(restoredMessages);
    setIsAuthed(true);
  }, []);

  const probeSession = useCallback(async () => {
    probeControllerRef.current?.abort();
    const controller = new AbortController();
    const generation = ++probeGenerationRef.current;
    const sessionGeneration = sessionGenerationRef.current;
    probeControllerRef.current = controller;
    setAuthChecking(true);

    const isCurrent = () =>
      !controller.signal.aborted &&
      generation === probeGenerationRef.current &&
      sessionGeneration === sessionGenerationRef.current;

    try {
      const data = await apiRequest<HistoryResponse>("chat/history", {
        method: "POST",
        body: JSON.stringify({}),
        signal: controller.signal,
      });

      if (!isCurrent()) return;
      restoreHistory(data);
    } catch (caught) {
      if (!isCurrent() || isAbortError(caught)) return;
      handleApiFailure(caught, true);
    } finally {
      if (isCurrent()) {
        setAuthChecking(false);
        probeControllerRef.current = null;
      }
    }
  }, [handleApiFailure, restoreHistory]);

  const syncHistory = useCallback(
    async (quiet = false) => {
      historyControllerRef.current?.abort();
      const controller = new AbortController();
      const generation = ++historyGenerationRef.current;
      const sessionGeneration = sessionGenerationRef.current;
      historyControllerRef.current = controller;

      if (!quiet) {
        setHistoryLoading(true);
        setError(null);
        setNotice(null);
      }

      const isCurrent = () =>
        !controller.signal.aborted &&
        generation === historyGenerationRef.current &&
        sessionGeneration === sessionGenerationRef.current;

      try {
        const data = await apiRequest<HistoryResponse>("chat/history", {
          method: "POST",
          body: JSON.stringify({}),
          signal: controller.signal,
        });

        if (!isCurrent()) return;
        restoreHistory(data);
        if (!quiet) {
          setNotice("History synced");
          setChatAnnouncement("Chat history synced");
        }
      } catch (caught) {
        if (!isCurrent() || isAbortError(caught)) return;
        handleApiFailure(caught, quiet);
      } finally {
        if (isCurrent()) {
          if (!quiet) setHistoryLoading(false);
          historyControllerRef.current = null;
        }
      }
    },
    [handleApiFailure, restoreHistory]
  );

  useEffect(() => {
    void probeSession();
    return cancelAllRequests;
  }, [cancelAllRequests, probeSession]);

  useEffect(() => {
    const view = new URLSearchParams(window.location.search).get("view");
    if (view === "chat" || view === "image") {
      setActiveView(view);
    }
  }, []);

  useEffect(() => {
    return () => {
      if (imagePreview) {
        URL.revokeObjectURL(imagePreview);
      }
    };
  }, [imagePreview]);

  function handleActiveViewChange(view: ActiveView) {
    setActiveView(view);

    const url = new URL(window.location.href);
    if (view === "chat") {
      url.searchParams.delete("view");
    } else {
      url.searchParams.set("view", view);
    }

    window.history.replaceState(null, "", `${url.pathname}${url.search}`);
  }

  function handleTabKeyDown(event: KeyboardEvent<HTMLButtonElement>) {
    if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) {
      return;
    }

    event.preventDefault();
    let nextView: ActiveView;
    if (event.key === "Home") {
      nextView = "chat";
    } else if (event.key === "End") {
      nextView = "image";
    } else if (event.key === "ArrowRight") {
      nextView = activeView === "chat" ? "image" : "chat";
    } else {
      nextView = activeView === "chat" ? "image" : "chat";
    }
    handleActiveViewChange(nextView);
    document.getElementById(TAB_IDS[nextView])?.focus();
  }

  async function handleAuthSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    cancelAllRequests();
    setAuthChecking(false);
    setHistoryLoading(false);
    setSending(false);
    setUploading(false);

    const controller = new AbortController();
    const generation = ++authGenerationRef.current;
    authControllerRef.current = controller;
    setAuthLoading(true);
    setError(null);
    setNotice(null);

    const isCurrent = () =>
      !controller.signal.aborted && generation === authGenerationRef.current;

    try {
      const login = await apiRequest<LoginResponse>("login", {
        method: "POST",
        body: JSON.stringify({ username, password }),
        signal: controller.signal,
      });

      if (!isCurrent()) return;
      if (!login.userId && login.success === false) {
        throw new Error(login.message || "Unable to sign in");
      }

      sessionGenerationRef.current += 1;
      setIsAuthed(true);
      setPassword("");
      setNotice("Signed in");
      await syncHistory(true);
    } catch (caught) {
      if (!isCurrent() || isAbortError(caught)) return;
      setError(getErrorMessage(caught));
    } finally {
      if (isCurrent()) {
        setAuthLoading(false);
        authControllerRef.current = null;
      }
    }
  }

  async function handleLogout() {
    cancelAllRequests();
    setAuthChecking(false);
    setHistoryLoading(false);
    setSending(false);
    setUploading(false);
    setError(null);
    setNotice(null);

    const controller = new AbortController();
    const generation = ++authGenerationRef.current;
    authControllerRef.current = controller;
    setAuthLoading(true);

    const isCurrent = () =>
      !controller.signal.aborted && generation === authGenerationRef.current;

    try {
      await apiRequest("logout", {
        method: "POST",
        body: JSON.stringify({}),
        signal: controller.signal,
      });

      if (!isCurrent()) return;
      clearLocalSession();
      setNotice("Signed out");
    } catch (caught) {
      if (!isCurrent() || isAbortError(caught)) return;

      if (isUnauthorized(caught)) {
        clearLocalSession();
        setNotice("Signed out");
      } else {
        setError(getErrorMessage(caught));
      }
    } finally {
      if (isCurrent()) {
        setAuthLoading(false);
        authControllerRef.current = null;
      }
    }
  }

  async function handleSend(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    const question = draft.trim();
    if (!question || sending) return;
    if (getUtf8ByteLength(question) > CHAT_MESSAGE_MAX_BYTES) {
      setError("Message must be 8000 UTF-8 bytes or fewer.");
      return;
    }

    streamControllerRef.current?.abort();
    const controller = new AbortController();
    const generation = ++streamGenerationRef.current;
    const sessionGeneration = sessionGenerationRef.current;
    streamControllerRef.current = controller;
    pendingDeltaRef.current = "";

    setSending(true);
    setError(null);
    setNotice(null);
    setDraft("");
    setChatAnnouncement("Assistant is responding");
    setScrollToLatestKey((current) => current + 1);

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

    const isCurrent = () =>
      !controller.signal.aborted &&
      generation === streamGenerationRef.current &&
      sessionGeneration === sessionGenerationRef.current;

    const flushPendingDelta = () => {
      deltaFrameRef.current = null;
      const content = pendingDeltaRef.current;
      pendingDeltaRef.current = "";
      if (content && isCurrent()) {
        appendMessageContent(setMessages, assistantMessageId, content);
      }
    };

    const flushNow = () => {
      if (deltaFrameRef.current !== null) {
        window.cancelAnimationFrame(deltaFrameRef.current);
      }
      flushPendingDelta();
    };

    try {
      const reply = await sendStreamingChat(
        question,
        (content) => {
          if (!content || !isCurrent()) return;
          pendingDeltaRef.current += content;
          if (deltaFrameRef.current === null) {
            deltaFrameRef.current = window.requestAnimationFrame(flushPendingDelta);
          }
        },
        controller.signal
      );

      flushNow();
      if (!isCurrent()) return;
      replaceMessageContent(
        setMessages,
        assistantMessageId,
        reply || "The service returned an empty response."
      );
      setChatAnnouncement("Assistant response complete");
    } catch (caught) {
      flushNow();
      if (!isCurrent() || isAbortError(caught)) return;

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
        setChatAnnouncement("Assistant response failed");
      }
    } finally {
      if (isCurrent()) {
        setSending(false);
        streamControllerRef.current = null;
      }
    }
  }

  function handleFileChange(file: File | null) {
    if (uploadControllerRef.current) {
      uploadControllerRef.current.abort();
      uploadControllerRef.current = null;
      uploadGenerationRef.current += 1;
      setUploading(false);
    }

    setError(null);
    setNotice(null);
    setImageResult(null);
    setImageConfidence(null);
    setImageError(null);

    if (file && !isSupportedImageFile(file)) {
      setSelectedFile(null);
      setImagePreview(null);
      setImageError("Choose a PNG, JPEG, or WebP image.");
      return;
    }

    setSelectedFile(file);
    setImagePreview(file ? URL.createObjectURL(file) : null);
  }

  async function handleUpload(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    if (!selectedFile || uploading) return;

    const file = selectedFile;
    if (!isSupportedImageFile(file)) {
      setImageError("Choose a PNG, JPEG, or WebP image.");
      return;
    }

    if (file.size > 5 * 1024 * 1024) {
      setError(null);
      setImageError("Image must be 5MB or smaller.");
      return;
    }

    uploadControllerRef.current?.abort();
    const controller = new AbortController();
    const generation = ++uploadGenerationRef.current;
    const sessionGeneration = sessionGenerationRef.current;
    uploadControllerRef.current = controller;

    setUploading(true);
    setError(null);
    setImageError(null);
    setNotice(null);

    const isCurrent = () =>
      !controller.signal.aborted &&
      generation === uploadGenerationRef.current &&
      sessionGeneration === sessionGenerationRef.current;

    try {
      const dataUrl = await readFileAsDataUrl(file, controller.signal);
      if (!isCurrent()) return;

      const image = dataUrl.split(",")[1];
      const data = await apiRequest<UploadResponse>("upload/send", {
        method: "POST",
        body: JSON.stringify({
          filename: file.name,
          image,
        }),
        signal: controller.signal,
      });

      if (!isCurrent()) return;
      if (!data.class_name) {
        throw new Error(data.message || "No recognition result returned");
      }

      setImageResult(data.class_name);
      setImageConfidence(
        typeof data.confidence === "number" ? data.confidence : null
      );
    } catch (caught) {
      if (!isCurrent() || isAbortError(caught)) return;

      if (isUnauthorized(caught)) {
        handleApiFailure(caught);
      } else {
        setImageError(getErrorMessage(caught));
      }
    } finally {
      if (isCurrent()) {
        setUploading(false);
        uploadControllerRef.current = null;
      }
    }
  }

  return (
    <section className="flex min-h-[calc(100vh-12rem)] flex-col gap-6">
      <div className="flex flex-col gap-4">
        <Badge variant="outline" className="w-fit gap-2 bg-background/70">
          <Sparkles className="size-3.5" aria-hidden />
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
              disabled={authLoading}
              onClick={handleLogout}
            >
              {authLoading ? (
                <Loader2 className="size-4 animate-spin" aria-hidden />
              ) : (
                <LogOut className="size-4" aria-hidden />
              )}
              Logout
            </Button>
          )}
        </div>
      </div>

      <StatusBanner error={error} notice={notice} />

      {authChecking ? (
        <div
          className="flex min-h-48 items-center justify-center gap-3 rounded-lg border bg-card/80 text-sm text-muted-foreground shadow-sm backdrop-blur"
          role="status"
          aria-live="polite"
        >
          <Loader2 className="size-4 animate-spin" aria-hidden />
          Checking your session…
        </div>
      ) : !isAuthed ? (
        <AuthPanel
          username={username}
          password={password}
          authLoading={authLoading}
          onUsernameChange={setUsername}
          onPasswordChange={setPassword}
          onSubmit={handleAuthSubmit}
        />
      ) : (
        <div className="min-h-0 rounded-lg border bg-card/80 shadow-sm backdrop-blur">
          <div className="flex flex-wrap items-center justify-between gap-3 p-4">
            <div
              className="flex rounded-md border bg-background p-1"
              role="tablist"
              aria-label="Chat tools"
              aria-orientation="horizontal"
            >
              <button
                id={TAB_IDS.chat}
                type="button"
                role="tab"
                aria-selected={activeView === "chat"}
                aria-controls={PANEL_IDS.chat}
                tabIndex={activeView === "chat" ? 0 : -1}
                className={cn(
                  "inline-flex h-9 items-center gap-2 rounded-sm px-3 text-sm font-medium transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
                  activeView === "chat"
                    ? "bg-primary text-primary-foreground"
                    : "text-muted-foreground hover:bg-muted hover:text-foreground"
                )}
                onClick={() => handleActiveViewChange("chat")}
                onKeyDown={handleTabKeyDown}
              >
                <Bot className="size-4" aria-hidden />
                Chat
              </button>
              <button
                id={TAB_IDS.image}
                type="button"
                role="tab"
                aria-selected={activeView === "image"}
                aria-controls={PANEL_IDS.image}
                tabIndex={activeView === "image" ? 0 : -1}
                className={cn(
                  "inline-flex h-9 items-center gap-2 rounded-sm px-3 text-sm font-medium transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
                  activeView === "image"
                    ? "bg-primary text-primary-foreground"
                    : "text-muted-foreground hover:bg-muted hover:text-foreground"
                )}
                onClick={() => handleActiveViewChange("image")}
                onKeyDown={handleTabKeyDown}
              >
                <ImageIcon className="size-4" aria-hidden />
                Image
              </button>
            </div>

            {activeView === "chat" && (
              <Button
                type="button"
                variant="outline"
                size="sm"
                className="gap-2"
                disabled={historyLoading || sending}
                onClick={() => void syncHistory(false)}
              >
                {historyLoading ? (
                  <Loader2 className="size-4 animate-spin" aria-hidden />
                ) : (
                  <RefreshCcw className="size-4" aria-hidden />
                )}
                Sync
              </Button>
            )}
          </div>

          <Separator />

          {activeView === "chat" ? (
            <div
              id={PANEL_IDS.chat}
              role="tabpanel"
              aria-labelledby={TAB_IDS.chat}
              className="min-h-0"
            >
              <ChatPanel
                messages={messages}
                draft={draft}
                sending={sending}
                announcement={chatAnnouncement}
                scrollToLatestKey={scrollToLatestKey}
                onDraftChange={setDraft}
                onSend={handleSend}
                onPromptSelect={setDraft}
              />
            </div>
          ) : (
            <div
              id={PANEL_IDS.image}
              role="tabpanel"
              aria-labelledby={TAB_IDS.image}
              className="min-h-0"
            >
              <ImagePanel
                selectedFile={selectedFile}
                imagePreview={imagePreview}
                imageResult={imageResult}
                imageConfidence={imageConfidence}
                imageError={imageError}
                uploading={uploading}
                onFileChange={handleFileChange}
                onUpload={handleUpload}
              />
            </div>
          )}
        </div>
      )}
    </section>
  );
}
