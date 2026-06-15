export function resolveSiteUrl(value: string, siteUrl: string) {
  try {
    return new URL(value, siteUrl).toString();
  } catch {
    return value;
  }
}
