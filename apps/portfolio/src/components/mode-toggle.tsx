"use client";

import { Button } from "@/components/ui/button";
import { MoonIcon, SunIcon } from "@radix-ui/react-icons";
import { useTheme } from "next-themes";
import { cn } from "@/lib/utils";
import { useSyncExternalStore } from "react";

const emptySubscribe = () => () => {};

export function ModeToggle({ className }: { className?: string }) {
  const { resolvedTheme, setTheme } = useTheme();
  const mounted = useSyncExternalStore(
    emptySubscribe,
    () => true,
    () => false
  );

  const isDark = mounted && resolvedTheme === "dark";

  return (
    <Button
      type="button"
      variant="link"
      size="icon"
      className={cn(!mounted && "opacity-0", className)}
      onClick={() => setTheme(isDark ? "light" : "dark")}
      aria-label={isDark ? "Use light theme" : "Use dark theme"}
    >
      {isDark ? (
        <SunIcon className="h-full w-full" aria-hidden />
      ) : (
        <MoonIcon className="h-full w-full" aria-hidden />
      )}
    </Button>
  );
}
