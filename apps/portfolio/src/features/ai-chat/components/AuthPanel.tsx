import type { FormEvent } from "react";
import { Loader2, ShieldCheck } from "lucide-react";

import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";

import type { AuthMode } from "../types";

type AuthPanelProps = {
  authMode: AuthMode;
  username: string;
  password: string;
  authLoading: boolean;
  onAuthModeChange: (mode: AuthMode) => void;
  onUsernameChange: (value: string) => void;
  onPasswordChange: (value: string) => void;
  onSubmit: (event: FormEvent<HTMLFormElement>) => void;
};

export function AuthPanel({
  authMode,
  username,
  password,
  authLoading,
  onAuthModeChange,
  onUsernameChange,
  onPasswordChange,
  onSubmit,
}: AuthPanelProps) {
  return (
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
            onClick={() => onAuthModeChange(mode)}
          >
            {mode === "login" ? "Sign in" : "Create account"}
          </button>
        ))}
      </div>

      <form className="flex flex-col gap-4" onSubmit={onSubmit}>
        <label className="flex flex-col gap-2 text-sm font-medium">
          Username
          <input
            value={username}
            onChange={(event) => onUsernameChange(event.target.value)}
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
            onChange={(event) => onPasswordChange(event.target.value)}
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
  );
}
