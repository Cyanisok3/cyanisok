import {
  BlogIndex,
  getBlogTotalPages,
} from "@/features/blog/blog-index";
import { notFound } from "next/navigation";

export function generateStaticParams() {
  const totalPages = getBlogTotalPages();

  return Array.from({ length: Math.max(totalPages - 1, 0) }, (_, index) => ({
    page: String(index + 2),
  }));
}

export default async function BlogPageNumber({
  params,
}: {
  params: Promise<{
    page: string;
  }>;
}) {
  const { page: pageParam } = await params;
  const page = Number.parseInt(pageParam, 10);
  const totalPages = getBlogTotalPages();

  if (!Number.isInteger(page) || page < 2 || page > totalPages) {
    notFound();
  }

  return <BlogIndex currentPage={page} />;
}
