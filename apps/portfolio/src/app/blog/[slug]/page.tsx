import { allPosts } from "content-collections";
import { formatDate, getReadingTime } from "@/lib/utils";
import { DATA } from "@/data/resume";
import type { Metadata } from "next";
import { notFound } from "next/navigation";
import { MDXContent } from "@content-collections/mdx/react";
import { mdxComponents } from "@/mdx-components";
import { TableOfContents } from "@/components/blog/table-of-contents";
import Link from "next/link";
import { ChevronLeft, ChevronRight } from "lucide-react";
import { resolveSiteUrl } from "@/lib/urls";

function getSortedPosts() {
  return [...allPosts].sort((a, b) => {
    if (new Date(a.publishedAt) > new Date(b.publishedAt)) {
      return -1;
    }
    return 1;
  });
}

export async function generateStaticParams() {
  return allPosts.map((post) => ({
    slug: post._meta.path.replace(/\.mdx$/, ""),
  }));
}

export async function generateMetadata({
  params,
}: {
  params: Promise<{
    slug: string;
  }>;
}): Promise<Metadata | undefined> {
  const { slug } = await params;
  const post = allPosts.find((p) => p._meta.path.replace(/\.mdx$/, "") === slug);

  if (!post) {
    return undefined;
  }

  let {
    title,
    publishedAt: publishedTime,
    summary: description,
    image,
  } = post;

  return {
    title,
    description,
    openGraph: {
      title,
      description,
      type: "article",
      publishedTime,
      url: `${DATA.url}/blog/${slug}`,
      ...(image && {
        images: [
          {
            url: resolveSiteUrl(image, DATA.url),
          },
        ],
      }),
    },
    twitter: {
      card: "summary_large_image",
      title,
      description,
      ...(image && {
        images: [resolveSiteUrl(image, DATA.url)],
      }),
    },
  };
}

export default async function Blog({
  params,
}: {
  params: Promise<{
    slug: string;
  }>;
}) {
  const { slug } = await params;
  const sortedPosts = getSortedPosts();
  const currentIndex = sortedPosts.findIndex(
    (p) => p._meta.path.replace(/\.mdx$/, "") === slug
  );
  const post = sortedPosts[currentIndex];

  if (!post) {
    notFound();
  }

  const previousPost = currentIndex > 0 ? sortedPosts[currentIndex - 1] : null;
  const nextPost = currentIndex < sortedPosts.length - 1 ? sortedPosts[currentIndex + 1] : null;
  const tocItems = post.toc ?? [];
  const hasToc = tocItems.length >= 2;

  const getSlug = (post: (typeof sortedPosts)[0]) =>
    post._meta.path.replace(/\.mdx$/, "");

  const jsonLdContent = JSON.stringify({
    "@context": "https://schema.org",
    "@type": "BlogPosting",
    headline: post.title,
    datePublished: post.publishedAt,
    dateModified: post.publishedAt,
    description: post.summary,
    image: post.image
      ? resolveSiteUrl(post.image, DATA.url)
      : `${DATA.url}/blog/${slug}/opengraph-image`,
    url: `${DATA.url}/blog/${slug}`,
    author: {
      "@type": "Person",
      name: DATA.name,
    },
  }).replace(/</g, "\\u003c");

  return (
    <section id="blog">
      <script
        type="application/ld+json"
        suppressHydrationWarning
        dangerouslySetInnerHTML={{
          __html: jsonLdContent,
        }}
      />
      <div className="flex justify-start gap-4 items-center">
        <Link href="/blog" className="text-sm text-muted-foreground hover:text-foreground transition-colors border border-border rounded-lg px-2 py-1 inline-flex items-center gap-1 mb-6 group" aria-label="Back to Blog">
          <ChevronLeft className="size-3 group-hover:-translate-x-px transition-transform" />
          Back to Blog
        </Link>
      </div>
      <div className="flex flex-col gap-4">
        <h1 className="title font-semibold text-3xl md:text-4xl tracking-tighter leading-tight">
          {post.title}
        </h1>
        <p className="max-w-2xl text-base leading-relaxed text-foreground/75">
          {post.summary}
        </p>
        <div className="flex flex-wrap items-center gap-x-2 gap-y-2 text-sm text-muted-foreground">
          <time dateTime={post.publishedAt}>{formatDate(post.publishedAt)}</time>
          <span aria-hidden>·</span>
          <span>{getReadingTime(post.content)}</span>
          {post.tags?.map((tag) => (
            <span
              key={tag}
              className="rounded-md border border-border bg-card px-2 py-0.5 text-xs text-muted-foreground"
            >
              {tag}
            </span>
          ))}
        </div>
      </div>
      <div className="my-6 flex w-full items-center">
        <div
          className="flex-1 h-px bg-border"
          style={{
            maskImage:
              "linear-gradient(90deg, transparent, black 8%, black 92%, transparent)",
            WebkitMaskImage:
              "linear-gradient(90deg, transparent, black 8%, black 92%, transparent)",
          }}
        />
      </div>

      {hasToc && (
        <TableOfContents items={tocItems} variant="mobile" className="mb-8" />
      )}

      <div className="relative">
        <div className="min-w-0">
          <article className="prose max-w-full text-pretty font-sans leading-relaxed text-foreground/90 dark:prose-invert">
            <MDXContent code={post.mdx} components={mdxComponents} />
          </article>

          <nav className="mt-12 pt-8 max-w-2xl">
            <div className="flex flex-col sm:flex-row justify-between gap-4">
              {previousPost ? (
                <Link
                  href={`/blog/${getSlug(previousPost)}`}
                  className="group flex-1 flex flex-col gap-1 p-4 rounded-lg border border-border hover:bg-accent/50 transition-colors"
                >
                  <span className="flex items-center gap-1 text-xs text-muted-foreground">
                    <ChevronLeft className="size-3" />
                    Previous
                  </span>
                  <span className="text-sm font-medium group-hover:text-foreground transition-colors whitespace-normal wrap-break-word">
                    {previousPost.title}
                  </span>
                </Link>
              ) : (
                <div className="hidden sm:block flex-1" />
              )}

              {nextPost ? (
                <Link
                  href={`/blog/${getSlug(nextPost)}`}
                  className="group flex-1 flex flex-col gap-1 p-4 rounded-lg border border-border hover:bg-accent/50 transition-colors text-right"
                >
                  <span className="flex items-center justify-end gap-1 text-xs text-muted-foreground">
                    Next
                    <ChevronRight className="size-3" />
                  </span>
                  <span className="text-sm font-medium group-hover:text-foreground transition-colors whitespace-normal wrap-break-word">
                    {nextPost.title}
                  </span>
                </Link>
              ) : (
                <div className="hidden sm:block flex-1" />
              )}
            </div>
          </nav>
        </div>

        {hasToc && (
          <TableOfContents
            items={tocItems}
            variant="desktop"
            className="absolute bottom-0 left-[calc(100%+2.5rem)] top-0 w-56"
          />
        )}
      </div>
    </section>
  );
}
