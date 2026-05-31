import type { SVGProps } from "react";

const Html5 = (props: SVGProps<SVGSVGElement>) => (
  <svg {...props} viewBox="0 0 256 256" preserveAspectRatio="xMidYMid">
    <path fill="#E44D26" d="M35 16h186l-17 190-76 34-76-34z" />
    <path fill="#F16529" d="M128 32v188l61-27 14-161z" />
    <path
      fill="#EBEBEB"
      d="M74 75h54v23H99l2 24h27v23H80zm7 82h23l2 18 22 9v24l-43-19z"
    />
    <path
      fill="#fff"
      d="M128 75h55l-2 23h-53zm0 47h49l-5 67-44 19v-24l22-9 2-30h-24z"
    />
  </svg>
);

export { Html5 };
