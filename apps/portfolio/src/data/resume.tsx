import { Icons } from "@/components/icons";
import { HomeIcon, NotebookIcon } from "lucide-react";
import { ReactLight } from "@/components/ui/svgs/reactLight";
import { NextjsIconDark } from "@/components/ui/svgs/nextjsIconDark";
import { Typescript } from "@/components/ui/svgs/typescript";
import { Python } from "@/components/ui/svgs/python";
import { Docker } from "@/components/ui/svgs/docker";
import { Java } from "@/components/ui/svgs/java";
import { Cplusplus } from "@/components/ui/svgs/cplusplus";
import { Sql } from "@/components/ui/svgs/sql";
import { Hugo } from "@/components/ui/svgs/hugo";
import { TailwindCss } from "@/components/ui/svgs/tailwindCss";
import { Html5 } from "@/components/ui/svgs/html5";
import { Css3 } from "@/components/ui/svgs/css3";
import { Git } from "@/components/ui/svgs/git";
import { FinalCutProX } from "@/components/ui/svgs/finalCutProX";
import { ChatBubbleIcon } from "@radix-ui/react-icons";

export type DataContact = {
  social: {
    [key: string]: {
      icon: React.ComponentType<{ className?: string }>;
      url: string;
      navbar: boolean;
    };
  };
};

export const DATA = {
  name: "@Cyanisok",
  initials: "Cyan",
  url: "https://cyanisok.cn",
  location: "Zhejiang, China",
  // locationLink: "https://www.google.com/maps/place/sanfrancisco",
  description:
    "A software developer with a passion for creating beautiful and functional web applications.",
  summary:
    "For me, programming is my profession, while photography, sketching, and short film editing are the passions that define my lifestyle. I enjoy freely switching between the rational world of technology and the emotional realm of artistic creation.",
  avatarUrl: "/my_bear_bright.png",
  skills: [
    { name: "C++", icon: Cplusplus },
    { name: "Java", icon: Java },
    { name: "Python", icon: Python },
    { name: "Typescript", icon: Typescript },
    { name: "SQL", icon: Sql },
    { name: "React", icon: ReactLight },
    { name: "Next.js", icon: NextjsIconDark },
    { name: "Hugo", icon: Hugo },
    { name: "Tailwind CSS", icon: TailwindCss },
    { name: "HTML5", icon: Html5 },
    { name: "CSS3", icon: Css3 },
    { name: "Git", icon: Git },
    { name: "Docker", icon: Docker },
    { name: "Final Cut Pro X", icon: FinalCutProX },
  ],
  navbar: [
    { href: "/", icon: HomeIcon, label: "Home" },
    { href: "/blog", icon: NotebookIcon, label: "Blog" },
    { href: "/chat", icon: ChatBubbleIcon, label: "Chat" },
  ],
  // contact: {
  //   email: "hello@example.com",
  //   tel: "+123456789",
  //   social: {
  //     GitHub: {
  //       name: "GitHub",
  //       url: "https://dub.sh/dillion-github",
  //       icon: Icons.github,
  //       navbar: true,
  //     },

  //     LinkedIn: {
  //       name: "LinkedIn",
  //       url: "https://dub.sh/dillion-linkedin",
  //       icon: Icons.linkedin,

  //       navbar: true,
  //     },
  //     X: {
  //       name: "X",
  //       url: "https://dub.sh/dillion-twitter",
  //       icon: Icons.x,

  //       navbar: true,
  //     },
  //     Youtube: {
  //       name: "Youtube",
  //       url: "https://dub.sh/dillion-youtube",
  //       icon: Icons.youtube,
  //       navbar: true,
  //     },
  //     email: {
  //       name: "Send Email",
  //       url: "#",
  //       icon: Icons.email,

  //       navbar: false,
  //     },
  //   },
  // },

  work: [] as {
    company: string;
    href?: string;
    logoUrl: string;
    title: string;
    start: string;
    end?: string;
    description: string;
  }[],
  education: [
    {
      school: "University of Nottingham Ningbo China",
      href: "https://www.nottingham.edu.cn/en/index.aspx",
      degree: "Bachelor of Science in Computer Science and Artificial Intelligence",
      logoUrl: "/unnc.png",
      start: "2023",
      end: "2027",
    },
  ],
  projects: [
    {
      title: "Chat Collect",
      href: "https://chatcollect.com",
      dates: "Jan 2024 - Feb 2024",
      active: true,
      description:
        "With the release of the [OpenAI GPT Store](https://openai.com/blog/introducing-the-gpt-store), I decided to build a SaaS which allows users to collect email addresses from their GPT users. This is a great way to build an audience and monetize your GPT API usage.",
      technologies: [
        "Next.js",
        "Typescript",
        "PostgreSQL",
        "Prisma",
        "TailwindCSS",
        "Stripe",
        "Shadcn UI",
        "Magic UI",
      ],
      links: [
        {
          type: "Website",
          href: "https://chatcollect.com",
          icon: <Icons.globe className="size-3" />,
        },
      ],
      image: "",
      video:
        "https://pub-83c5db439b40468498f97946200806f7.r2.dev/chat-collect.mp4",
    },
  ],
  achievements: [],
  certificates: [],
  contact: {
    social: {},
  } satisfies DataContact,
};
