import type { SVGProps } from "react";

const Git = (props: SVGProps<SVGSVGElement>) => (
  <svg {...props} viewBox="0 0 256 256" preserveAspectRatio="xMidYMid">
    <rect
      x="38"
      y="38"
      width="180"
      height="180"
      rx="22"
      fill="#F05032"
      transform="rotate(45 128 128)"
    />
    <path
      fill="#fff"
      d="M99 69a17 17 0 0 1 27 20l30 30a17 17 0 1 1-11 11l-30-30v56a17 17 0 1 1-15 0V94a17 17 0 0 1-1-25z"
    />
  </svg>
);

export { Git };
