import { AlertCircle, ShieldCheck } from "lucide-react";

import { cn } from "@/lib/utils";

type StatusBannerProps = {
  error: string | null;
  notice: string | null;
};

export function StatusBanner({ error, notice }: StatusBannerProps) {
  if (!error && !notice) {
    return null;
  }

  return (
    <div
      className={cn(
        "flex items-start gap-3 rounded-lg border bg-background/80 px-4 py-3 text-sm shadow-sm",
        error
          ? "border-destructive/40 text-destructive"
          : "border-border text-muted-foreground"
      )}
    >
      {error ? (
        <AlertCircle className="mt-0.5 size-4 flex-none" />
      ) : (
        <ShieldCheck className="mt-0.5 size-4 flex-none" />
      )}
      <p>{error || notice}</p>
    </div>
  );
}
