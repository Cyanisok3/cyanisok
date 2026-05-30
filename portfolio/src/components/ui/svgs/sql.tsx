import type { SVGProps } from "react";

const Sql = (props: SVGProps<SVGSVGElement>) => (
  <svg
    {...props}
    preserveAspectRatio="xMidYMid"
    viewBox="0 0 256 256"
  >
    {/* 最底层的圆柱体基座（暗蓝色） */}
    <path fill="#004A80" d="M32 72v112c0 22.1 43 40 96 40s96-17.9 96-40V72z" />
    
    {/* 底层与中层的盘面（中蓝色） */}
    <ellipse cx="128" cy="184" rx="96" ry="40" fill="#00599C" />
    <ellipse cx="128" cy="128" rx="96" ry="40" fill="#007ACC" />
    
    {/* 左侧的弧面阴影（不遮挡最上方的发光面） */}
    <path fill="#000000" opacity="0.25" d="M128 224c-53 0-96-17.9-96-40V72a96 40 0 0 0 96 40z" />
    
    {/* 顶层高光盘面（亮蓝色） */}
    <ellipse cx="128" cy="72" rx="96" ry="40" fill="#0096FF" />
  </svg>
);

export { Sql };
