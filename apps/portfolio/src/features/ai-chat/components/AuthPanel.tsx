import type { FormEvent } from "react";
import { Loader2, ShieldCheck } from "lucide-react";

import { Button } from "@/components/ui/button";

type AuthPanelProps = {
  username: string;
  password: string;
  authLoading: boolean;
  onUsernameChange: (value: string) => void;
  onPasswordChange: (value: string) => void;
  onSubmit: (event: FormEvent<HTMLFormElement>) => void;
};

export function AuthPanel({
  username,
  password,
  authLoading,
  onUsernameChange,
  onPasswordChange,
  onSubmit,
}: AuthPanelProps) {
  return (
    <div className="rounded-lg border bg-card/80 p-5 shadow-sm backdrop-blur">
      <div className="mb-5 space-y-1">
        <h2 className="text-lg font-semibold">Sign in to continue</h2>
        <p className="text-sm leading-relaxed text-muted-foreground">
          Authentication keeps your chat history connected to your session and
          unlocks the image recognition tool.
        </p>
      </div>

      <form className="flex flex-col gap-4" onSubmit={onSubmit}>
        <label className="flex flex-col gap-2 text-sm font-medium">
          Username
          <input
            value={username}
            onChange={(event) => onUsernameChange(event.target.value)}
            className="h-10 rounded-md border bg-background px-3 text-sm outline-none transition-colors focus:border-ring focus-visible:ring-2 focus-visible:ring-ring"
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
            className="h-10 rounded-md border bg-background px-3 text-sm outline-none transition-colors focus:border-ring focus-visible:ring-2 focus-visible:ring-ring"
            minLength={8}
            maxLength={128}
            type="password"
            autoComplete="current-password"
            required
          />
        </label>
        <Button type="submit" className="gap-2" disabled={authLoading}>
          {authLoading ? (
            <Loader2 className="size-4 animate-spin" />
          ) : (
            <ShieldCheck className="size-4" />
          )}
          Sign in
        </Button>
      </form>
    </div>
  );
}
