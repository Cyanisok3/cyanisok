/* eslint-disable @next/next/no-img-element */
import BlurFade from "@/components/magicui/blur-fade";
import BlurFadeText from "@/components/magicui/blur-fade-text";
import { DATA, type DataContact } from "@/data/resume";
import Link from "next/link";
import Markdown from "react-markdown";
import ContactSection from "@/components/section/contact-section";
import ProjectsSection from "@/components/section/projects-section";
import { ArrowUpRight, MapPin } from "lucide-react";

const BLUR_FADE_DELAY = 0.04;

export default function Page() {
  const socialLinks = Object.entries(DATA.contact?.social ?? {}) as Array<
    [string, DataContact["social"][string]]
  >;

  return (
    <>
      {/* Fixed background image with overlay */}
      <div
        className="fixed inset-0 -z-10"
        style={{
          backgroundImage: "url('/background.png')",
          backgroundSize: "cover",
          backgroundPosition: "center",
          backgroundRepeat: "no-repeat",
        }}
      />
      <div className="fixed inset-0 -z-10 bg-background dark:bg-background/80" />

      <main className="min-h-dvh flex flex-col gap-14 relative">
      <section id="hero">
        <div className="mx-auto w-full max-w-2xl">
          <div className="flex flex-col md:flex-row items-start md:items-center gap-8">
            {/* Text Content - Left aligned */}
            <div className="flex flex-col gap-4 flex-1">
              {/* Name */}
              <BlurFadeText
                delay={BLUR_FADE_DELAY}
                className="text-4xl md:text-5xl lg:text-6xl font-bold tracking-tight font-afl"
                yOffset={8}
                text={DATA.name}
              />

              {/* Location */}
              <div className="flex items-center gap-2 text-muted-foreground">
                <MapPin className="size-4" />
                <span className="text-base md:text-lg">{DATA.location}</span>
              </div>

              {/* Description */}
              <BlurFadeText
                className="text-muted-foreground md:text-lg lg:text-xl font-afl"
                delay={BLUR_FADE_DELAY}
                text={DATA.description}
              />

              {/* Social Links */}
              {socialLinks.length > 0 && (
                <div className="flex items-center gap-3 pt-2">
                  {socialLinks.map(([name, social], index) => {
                    const Icon = social.icon;
                    const isExternal = social.url.startsWith("http");

                    return (
                      <BlurFade
                        delay={BLUR_FADE_DELAY * (2 + index * 0.5)}
                        key={name}
                      >
                        <Link
                          href={social.url}
                          target={isExternal ? "_blank" : undefined}
                          rel={isExternal ? "noopener noreferrer" : undefined}
                          className="size-10 flex items-center justify-center rounded-full border bg-background hover:bg-accent hover:text-accent-foreground transition-colors"
                        >
                          <Icon className="size-5" />
                          <span className="sr-only">{name}</span>
                        </Link>
                      </BlurFade>
                    );
                  })}
                </div>
              )}
            </div>

            {/* Image - Right side */}
            <div className="flex-shrink-0">
              <BlurFade delay={BLUR_FADE_DELAY}>
                <img
                  src="/Polaroid.png"
                  alt={DATA.name}
                  className="w-52 md:w-64 h-auto rounded-lg shadow-xl"
                />
              </BlurFade>
            </div>
          </div>
        </div>
      </section>
      <section id="about">
        <div className="flex min-h-0 flex-col gap-y-4">
          <BlurFade delay={BLUR_FADE_DELAY * 4}>
            <h2 className="text-xl font-bold">About</h2>
          </BlurFade>
          <BlurFade delay={BLUR_FADE_DELAY * 4.5}>
            <div className="prose max-w-full text-pretty font-sans leading-relaxed text-muted-foreground dark:prose-invert font-afl">
              <Markdown>
                {DATA.summary}
              </Markdown>
            </div>
          </BlurFade>
        </div>
      </section>
      <section id="education">
        <div className="flex min-h-0 flex-col gap-y-6">
          <BlurFade delay={BLUR_FADE_DELAY * 7}>
            <h2 className="text-xl font-bold">Education</h2>
          </BlurFade>
          <div className="flex flex-col gap-8">
            {DATA.education.map((education, index) => (
              <BlurFade
                key={education.school}
                delay={BLUR_FADE_DELAY * 8 + index * 0.05}
              >
                <Link
                  href={education.href}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="flex items-center gap-x-3 justify-between group"
                >
                  <div className="flex items-center gap-x-3 flex-1 min-w-0">
                    {education.logoUrl ? (
                      <img
                        src={education.logoUrl}
                        alt={education.school}
                        className="size-8 md:size-10 p-1 border rounded-full shadow ring-2 ring-border overflow-hidden object-contain flex-none"
                      />
                    ) : (
                      <div className="size-8 md:size-10 p-1 border rounded-full shadow ring-2 ring-border bg-muted flex-none" />
                    )}
                    <div className="flex-1 min-w-0 flex flex-col gap-0.5">
                      <div className="font-semibold leading-none flex items-center gap-2">
                        {education.school}
                        <ArrowUpRight className="h-3.5 w-3.5 text-muted-foreground opacity-0 -translate-x-2 group-hover:opacity-100 group-hover:translate-x-0 transition-all duration-200" aria-hidden />
                      </div>
                      <div className="font-sans text-sm text-muted-foreground">
                        {education.degree}
                      </div>
                    </div>
                  </div>
                  <div className="flex items-center gap-1 text-xs tabular-nums text-muted-foreground text-right flex-none">
                    <span>
                      {education.start} - {education.end}
                    </span>
                  </div>
                </Link>
              </BlurFade>
            ))}
          </div>
        </div>
      </section>
      <section id="skills">
        <div className="flex min-h-0 flex-col gap-y-4">
          <BlurFade delay={BLUR_FADE_DELAY * 9}>
            <h2 className="text-xl font-bold">Skills</h2>
          </BlurFade>
          <div className="flex flex-wrap gap-2">
            {DATA.skills.map((skill, id) => (
              <BlurFade key={skill.name} delay={BLUR_FADE_DELAY * 10 + id * 0.05}>
                <div className="border bg-background border-border ring-2 ring-border/20 rounded-xl h-8 w-fit px-4 flex items-center gap-2">
                  {skill.icon && <skill.icon className="size-4 rounded overflow-hidden object-contain" />}
                  <span className="text-foreground text-sm font-medium">{skill.name}</span>
                </div>
              </BlurFade>
            ))}
          </div>
        </div>
      </section>
      <section id="projects">
        <BlurFade delay={BLUR_FADE_DELAY * 11}>
          <ProjectsSection />
        </BlurFade>
      </section>
      <section id="contact">
        <BlurFade delay={BLUR_FADE_DELAY * 16}>
          <ContactSection />
        </BlurFade>
      </section>
    </main>
    </>
  );
}
