import type { SVGProps } from "react";

const Cplusplus = (props: SVGProps<SVGSVGElement>) => (
  <svg
    {...props}
    preserveAspectRatio="xMidYMid"
    viewBox="0 0 256 256"
  >
    {/* 六边形基座底板 */}
    <path 
      fill="#00599C" 
      d="M128 0L238.85 64v128L128 256L17.15 192V64z" 
    />
    {/* 左半区晶体切割阴影层 */}
    <path 
      fill="#000000" 
      opacity="0.15" 
      d="M128 0v256l-110.85-64V64z" 
    />
    {/* 完美闭合的正圆形 C 字母切口 */}
    <path
      fill="#FFFFFF"
      d="M105.4 102.6A36 36 0 1 0 105.4 153.4L94.1 142.1A20 20 0 1 1 94.1 113.9Z"
    />
    {/* 两个等比微调间距的 '+' 符号 */}
    <path
      fill="#FFFFFF"
      d="M140 110h12v12h12v12h-12v12h-12v-12h-12v-12h12zm48 0h12v12h12v12h-12v12h-12v-12h-12v-12h12z"
    />
  </svg>
);

export { Cplusplus };
