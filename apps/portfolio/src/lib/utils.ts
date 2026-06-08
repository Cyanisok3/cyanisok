import { type ClassValue, clsx } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function formatDate(date: string | Date) {
  // Use UTC to ensure consistent formatting between server and client
  const dateObj = typeof date === "string" ? new Date(date) : date;
  return dateObj.toLocaleDateString("en-US", {
    year: "numeric",
    month: "long",
    day: "numeric",
    timeZone: "UTC",
  });
}

export function getReadingTime(content: string) {
  const readableContent = content
    .replace(/```[\s\S]*?```/g, " ")
    .replace(/`[^`]*`/g, " ");
  const latinWords =
    readableContent.match(/[A-Za-z0-9]+(?:[-'][A-Za-z0-9]+)*/g)?.length ?? 0;
  const cjkChars = readableContent.match(/[\u3400-\u9fff]/g)?.length ?? 0;
  const estimatedWords = latinWords + cjkChars / 2;
  const minutes = Math.max(1, Math.ceil(estimatedWords / 220));

  return `${minutes} min read`;
}
