import type { SVGProps } from "react";

const Hugo = (props: SVGProps<SVGSVGElement>) => (
  <svg {...props} viewBox="0 0 256 256" preserveAspectRatio="xMidYMid">
    <rect width="256" height="256" rx="56" fill="#FF4088" />
    <path
      fill="#fff"
      d="M62 58h34v55h64V58h34v140h-34v-56H96v56H62z"
    />
    <circle cx="76" cy="76" r="10" fill="#00B4B6" />
    <circle cx="180" cy="180" r="10" fill="#00B4B6" />
  </svg>
);

export { Hugo };
