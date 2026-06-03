/* eslint-disable @next/next/no-img-element */

import BlurFade from "@/components/magicui/blur-fade";

interface MediaContainerProps {
  src: string;
  alt?: string;
  aspectRatio?: "wide" | "portrait" | "square";
  type?: "image" | "video";
  className?: string;
}

const mediaFrameClassNames = {
  wide: "aspect-video w-full",
  portrait: "aspect-[9/16] w-full",
  square: "aspect-square w-full",
};

const mediaFigureClassNames = {
  wide: "w-full",
  portrait: "w-full max-w-[min(100%,20rem)]",
  square: "w-full max-w-[min(100%,34rem)]",
};

export function MediaContainer({
  src,
  alt = "",
  aspectRatio = "wide",
  type = "image",
  className = "",
}: MediaContainerProps) {
  return (
    <BlurFade inView className="my-8">
      <figure
        className={`not-prose mx-auto overflow-hidden rounded-lg border border-border bg-card shadow-sm ${mediaFigureClassNames[aspectRatio]} ${className}`}
      >
        <div
          className={`flex max-h-[82vh] items-center justify-center bg-muted/40 ${mediaFrameClassNames[aspectRatio]}`}
        >
          {type === "image" ? (
            <img
              src={src}
              alt={alt}
              className="h-full w-full max-w-full object-cover object-center"
            />
          ) : (
            <video
              src={src}
              className="h-full w-full max-w-full object-contain"
              controls
              playsInline
              preload="metadata"
            />
          )}
        </div>
        {alt && (
          <figcaption className="border-t border-border px-4 py-2 text-xs text-muted-foreground">
            {alt}
          </figcaption>
        )}
      </figure>
    </BlurFade>
  );
}
