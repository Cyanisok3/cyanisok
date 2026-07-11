"use client";

import { cn } from "@/lib/utils";
import { motion, useReducedMotion, Variants } from "motion/react";
import { useMemo } from "react";

interface BlurFadeTextProps {
  text: string;
  as?: "div" | "h1";
  className?: string;
  variant?: {
    hidden: { y: number };
    visible: { y: number };
  };
  duration?: number;
  characterDelay?: number;
  delay?: number;
  yOffset?: number;
  animateByCharacter?: boolean;
}
const BlurFadeText = ({
  text,
  as: Wrapper = "div",
  className,
  variant,
  duration = 0.4,
  characterDelay = 0.03,
  delay = 0,
  yOffset = 8,
  animateByCharacter = false,
}: BlurFadeTextProps) => {
  const shouldReduceMotion = useReducedMotion();
  const defaultVariants: Variants = {
    hidden: { y: -yOffset, opacity: 0, filter: "blur(8px)" },
    visible: { y: 0, opacity: 1, filter: "blur(0px)" },
  };
  const combinedVariants = variant || defaultVariants;
  const characters = useMemo(() => Array.from(text), [text]);

  if (animateByCharacter) {
    return (
      <Wrapper className="flex">
        {characters.map((char, i) => {
          const charVariants: Variants = {
            hidden: { y: -yOffset, opacity: 0, filter: "blur(8px)" },
            visible: { y: 0, opacity: 1, filter: "blur(0px)" },
          };
          return (
            <motion.span
              key={i}
              initial={shouldReduceMotion ? false : "hidden"}
              animate="visible"
              variants={charVariants}
              transition={{
                duration: shouldReduceMotion ? 0 : duration,
                delay: shouldReduceMotion ? 0 : delay + i * characterDelay,
                ease: "easeOut",
              }}
              className={cn("inline-block", className)}
              style={{ width: char.trim() === "" ? "0.2em" : "auto" }}
            >
              {char}
            </motion.span>
          );
        })}
      </Wrapper>
    );
  }

  return (
    <Wrapper className="flex">
      <motion.span
        initial={shouldReduceMotion ? false : "hidden"}
        animate="visible"
        variants={combinedVariants}
        transition={{
          duration: shouldReduceMotion ? 0 : duration,
          delay: shouldReduceMotion ? 0 : delay,
          ease: "easeOut",
        }}
        className={cn("inline-block", className)}
      >
        {text}
      </motion.span>
    </Wrapper>
  );
};

export default BlurFadeText;
