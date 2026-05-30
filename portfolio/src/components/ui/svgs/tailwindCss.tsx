import type { SVGProps } from "react";

const TailwindCss = (props: SVGProps<SVGSVGElement>) => (
  <svg {...props} viewBox="0 0 256 256" preserveAspectRatio="xMidYMid">
    <rect width="256" height="256" rx="56" fill="#0EA5E9" />
    <path
      fill="#fff"
      d="M128 76c-31 0-50 15-58 45 12-15 26-20 43-15 10 3 17 10 25 17 13 13 29 28 61 28 31 0 50-15 58-45-12 15-26 20-43 15-10-3-17-10-25-17-13-13-29-28-61-28zM57 129c-31 0-50 15-58 45 12-15 26-20 43-15 10 3 17 10 25 17 13 13 29 28 61 28 31 0 50-15 58-45-12 15-26 20-43 15-10-3-17-10-25-17-13-13-29-28-61-28z"
      transform="translate(0 -12)"
    />
  </svg>
);

export { TailwindCss };
