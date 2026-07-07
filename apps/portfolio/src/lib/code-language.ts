const languageAliases: Record<string, string> = {
  cxx: "cpp",
  py: "python",
  sh: "bash",
  shell: "bash",
  tex: "latex",
  text: "plaintext",
  txt: "plaintext",
};

export function extractCodeLanguage(className?: string) {
  if (!className) return "plaintext";
  const match = className.match(/language-([a-z0-9-]+)/i);
  return match ? match[1].toLowerCase() : "plaintext";
}

export function resolveCodeLanguage(
  className: string | undefined,
  supportedLanguages: Record<string, unknown>
) {
  const extracted = extractCodeLanguage(className);
  const aliased = languageAliases[extracted] ?? extracted;
  return aliased in supportedLanguages ? aliased : "plaintext";
}
