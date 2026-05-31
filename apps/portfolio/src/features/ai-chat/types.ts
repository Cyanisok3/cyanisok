export type ApiError = Error & {
  status?: number;
};

export type AuthMode = "login" | "register";

export type ActiveView = "chat" | "image";

export type ChatMessage = {
  id: string;
  role: "user" | "assistant";
  content: string;
};

export type HistoryResponse = {
  success?: boolean;
  history?: Array<{
    is_user?: boolean;
    content?: string;
  }>;
};

export type LoginResponse = {
  success?: boolean;
  userId?: number;
  username?: string;
  message?: string;
};

export type ChatResponse = {
  success?: boolean;
  information?: string;
  Information?: string;
  message?: string;
};

export type UploadResponse = {
  success?: string;
  filename?: string;
  class_name?: string;
  confidence?: number;
  message?: string;
};
