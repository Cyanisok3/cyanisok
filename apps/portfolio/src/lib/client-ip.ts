export function getTrustedClientIp(headers: Headers) {
  return headers.get("x-real-ip")?.trim() || "local";
}
