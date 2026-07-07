import { BlogIndex } from "@/features/blog/blog-index";
import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Blog",
  description: "Dive into the ocean of information.",
  openGraph: {
    title: "Blog",
    description: "Dive into the ocean of information.",
  },
  twitter: {
    card: "summary_large_image",
    title: "Blog",
    description: "Dive into the ocean of information.",
  },
};

export default function BlogPage() {
  return <BlogIndex />;
}
