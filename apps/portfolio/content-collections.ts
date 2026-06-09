import { defineCollection, defineConfig } from "@content-collections/core";
import { compileMDX } from "@content-collections/mdx";
import remarkGfm from "remark-gfm";
import { z } from "zod";
import { remarkCodeMeta } from "./src/lib/remark-code-meta";
import { extractTableOfContents, rehypeHeadingIds } from "./src/lib/toc";

const posts = defineCollection({
  name: "posts",
  directory: "content",
  include: "*.mdx",
  schema: z.object({
    title: z.string(),
    publishedAt: z.string(),
    updatedAt: z.string().optional(),
    author: z.string().optional(),
    summary: z.string(),
    tags: z.array(z.string()).optional(),
    image: z.string().optional(),
    content: z.string(),
  }),
  transform: async (document, context) => {
    const mdx = await compileMDX(context, document, {
      remarkPlugins: [remarkGfm, remarkCodeMeta],
      rehypePlugins: [rehypeHeadingIds],
    });
    return {
      ...document,
      toc: extractTableOfContents(document.content),
      mdx,
    };
  },
});

export default defineConfig({
  collections: [posts],
});
