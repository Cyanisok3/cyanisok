import Link from "next/link";
import { ArrowLeft } from "lucide-react";

import { Button } from "@/components/ui/button";

export default function ChatPage() {
  return (
    <section className="min-h-[calc(100vh-12rem)] flex flex-col justify-center gap-6">
      <div className="space-y-3">
        <p className="text-sm text-muted-foreground">Chat</p>
        <h1 className="text-3xl font-semibold tracking-tight">
          Chat is not open yet
        </h1>
        <p className="max-w-lg text-muted-foreground leading-relaxed">
          I&apos;m still shaping this page. For now, the blog and project notes
          are the best places to explore what I&apos;m working on.
        </p>
      </div>
      <div>
        <Button asChild variant="outline" className="gap-2">
          <Link href="/">
            <ArrowLeft className="size-4" />
            Back home
          </Link>
        </Button>
      </div>
    </section>
  );
}
