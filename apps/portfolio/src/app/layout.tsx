import Navbar from "@/components/navbar";
import { SiteFooter } from "@/components/site-footer";
import { ThemeProvider } from "@/components/theme-provider";
import { TooltipProvider } from "@/components/ui/tooltip";
import { DATA } from "@/data/resume";
import { cn } from "@/lib/utils";
import type { Metadata } from "next";
import localFont from "next/font/local";
import "./globals.css";

const sans = localFont({
  src: "../../public/fonts/CabinetGrotesk-Medium.ttf",
  variable: "--font-sans",
  display: "swap",
});

const code = localFont({
  src: [
    {
      path: "../../public/fonts/google-sans-code/GoogleSansCode[wght].ttf",
      style: "normal",
      weight: "300 800",
    },
    {
      path: "../../public/fonts/google-sans-code/GoogleSansCode-Italic[wght].ttf",
      style: "italic",
      weight: "300 800",
    },
  ],
  variable: "--font-google-sans-code",
  display: "swap",
});

export const metadata: Metadata = {
  metadataBase: new URL(DATA.url),
  title: {
    default: DATA.name,
    template: `%s | ${DATA.name}`,
  },
  description: DATA.description,
  openGraph: {
    title: `${DATA.name}`,
    description: DATA.description,
    url: DATA.url,
    siteName: `${DATA.name}`,
    locale: "en_US",
    type: "website",
  },
  robots: {
    index: true,
    follow: true,
    googleBot: {
      index: true,
      follow: true,
      "max-video-preview": -1,
      "max-image-preview": "large",
      "max-snippet": -1,
    },
  },
  twitter: {
    title: `${DATA.name}`,
    card: "summary_large_image",
  },
  verification: {
    google: "",
    yandex: "",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" suppressHydrationWarning className="bg-background">
      <body
        className={cn(
          "min-h-screen bg-background font-sans antialiased relative",
          sans.variable,
          code.variable
        )}
      >
        <ThemeProvider attribute="class" defaultTheme="light">
          <TooltipProvider delayDuration={0}>
            <div className="site-background-cover fixed inset-0 z-0 pointer-events-none" />
            <div className="site-background-artwork fixed inset-0 z-0 pointer-events-none" />
            <div className="site-background-overlay fixed inset-0 z-0 pointer-events-none" />
            <div className="relative z-10 max-w-2xl mx-auto py-12 pb-24 sm:py-24 px-6">
              <main id="main-content">{children}</main>
              <SiteFooter />
            </div>
            <Navbar />
          </TooltipProvider>
        </ThemeProvider>
      </body>
    </html>
  );
}
