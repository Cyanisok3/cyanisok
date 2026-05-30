import type { SVGProps } from "react";

const Css3 = (props: SVGProps<SVGSVGElement>) => (
  <svg {...props} viewBox="0 0 256 256" preserveAspectRatio="xMidYMid">
    <path fill="#1572B6" d="M35 16h186l-17 190-76 34-76-34z" />
    <path fill="#33A9DC" d="M128 32v188l61-27 14-161z" />
    <path
      fill="#EBEBEB"
      d="M75 75h53v23H101l2 24h25v23H82zm8 82h23l2 18 20 8v24l-41-18z"
    />
    <path
      fill="#fff"
      d="M128 75h54l-3 23h-51zm0 47h49l-5 67-44 18v-24l21-8 2-30h-23z"
    />
  </svg>
);

export { Css3 };
