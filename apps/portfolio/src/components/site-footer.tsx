const ICP_NUMBER = "浙ICP备2026031525号-1";

export function SiteFooter() {
  return (
    <footer className="mt-16 pb-10 text-center text-xs text-muted-foreground">
      <a
        href="https://beian.miit.gov.cn/"
        target="_blank"
        rel="noopener noreferrer"
        className="transition-colors hover:text-foreground"
      >
        {ICP_NUMBER}
      </a>
    </footer>
  );
}
