"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { Dock, DockIcon } from "@/components/magicui/dock";
import { ModeToggle } from "@/components/mode-toggle";
import { Separator } from "@/components/ui/separator";
import {
  Tooltip,
  TooltipArrow,
  TooltipContent,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { DATA, type DataContact } from "@/data/resume";
import { cn } from "@/lib/utils";

const navIconClassName =
  "rounded-3xl cursor-pointer size-full bg-background p-0 text-muted-foreground hover:text-foreground hover:bg-muted backdrop-blur-3xl border border-border transition-colors";

export default function Navbar() {
  const pathname = usePathname();
  const socialLinks = Object.entries(DATA.contact?.social ?? {}) as Array<
    [string, DataContact["social"][string]]
  >;

  const isRouteActive = (href: string) => {
    if (href.startsWith("http")) return false;
    if (href === "/") return pathname === "/";
    return pathname === href || pathname.startsWith(`${href}/`);
  };

  return (
    <div className="pointer-events-none fixed inset-x-0 bottom-4 z-30">
      <Dock className="z-50 pointer-events-auto relative h-14 p-2 w-fit mx-auto flex gap-2 border bg-card/90 backdrop-blur-3xl shadow-[0_0_10px_3px] shadow-primary/5">
        {DATA.navbar.map((item) => {
          const isExternal = item.href.startsWith("http");
          const isActive = isRouteActive(item.href);
          const itemClassName = cn(
            navIconClassName,
            isActive &&
              "bg-primary text-primary-foreground border-primary shadow-[0_8px_24px_-12px_rgba(0,0,0,0.55)] hover:bg-primary hover:text-primary-foreground"
          );

          return (
            <Tooltip key={item.href}>
              <TooltipTrigger asChild>
                {isExternal ? (
                  <a
                    href={item.href}
                    target="_blank"
                    rel="noopener noreferrer"
                  >
                    <DockIcon className={itemClassName}>
                      <item.icon className="size-full rounded-sm overflow-hidden object-contain" />
                    </DockIcon>
                  </a>
                ) : (
                  <Link
                    href={item.href}
                    aria-current={isActive ? "page" : undefined}
                  >
                    <DockIcon className={itemClassName} isActive={isActive}>
                      <item.icon className="size-full rounded-sm overflow-hidden object-contain" />
                    </DockIcon>
                  </Link>
                )}
              </TooltipTrigger>
              <TooltipContent
                side="top"
                sideOffset={8}
                className="rounded-xl bg-primary text-primary-foreground px-4 py-2 text-sm shadow-[0_10px_40px_-10px_rgba(0,0,0,0.3)] dark:shadow-[0_10px_40px_-10px_rgba(0,0,0,0.5)]"
              >
                <p>{item.label}</p>
                <TooltipArrow className="fill-primary" />
              </TooltipContent>
            </Tooltip>
          );
        })}

        {socialLinks.length > 0 && (
          <>
            <Separator
              orientation="vertical"
              className="h-2/3 m-auto w-px bg-border"
            />
            {socialLinks
              .filter(([_, social]) => social.navbar)
              .map(([name, social], index) => {
                const isExternal = social.url.startsWith("http");
                const IconComponent = social.icon;

                return (
                  <Tooltip key={`social-${name}-${index}`}>
                    <TooltipTrigger asChild>
                      <a
                        href={social.url}
                        target={isExternal ? "_blank" : undefined}
                        rel={isExternal ? "noopener noreferrer" : undefined}
                      >
                        <DockIcon className={navIconClassName}>
                          <IconComponent className="size-full rounded-sm overflow-hidden object-contain" />
                        </DockIcon>
                      </a>
                    </TooltipTrigger>
                    <TooltipContent
                      side="top"
                      sideOffset={8}
                      className="rounded-xl bg-primary text-primary-foreground px-4 py-2 text-sm shadow-[0_10px_40px_-10px_rgba(0,0,0,0.3)] dark:shadow-[0_10px_40px_-10px_rgba(0,0,0,0.5)]"
                    >
                      <p>{name}</p>
                      <TooltipArrow className="fill-primary" />
                    </TooltipContent>
                  </Tooltip>
                );
              })}
          </>
        )}

        <Tooltip>
          <TooltipTrigger asChild>
            <DockIcon className={navIconClassName}>
              <ModeToggle className="absolute inset-0 z-10 size-full cursor-pointer [&>svg]:h-1/2 [&>svg]:w-1/2" />
            </DockIcon>
          </TooltipTrigger>
          <TooltipContent
            side="top"
            sideOffset={8}
            className="rounded-xl bg-primary text-primary-foreground px-4 py-2 text-sm shadow-[0_10px_40px_-10px_rgba(0,0,0,0.3)] dark:shadow-[0_10px_40px_-10px_rgba(0,0,0,0.5)]"
          >
            <p>Theme</p>
            <TooltipArrow className="fill-primary" />
          </TooltipContent>
        </Tooltip>
      </Dock>
    </div>
  );
}
