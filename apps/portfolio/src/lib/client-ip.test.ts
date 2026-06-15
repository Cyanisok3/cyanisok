import { describe, expect, it } from "vitest";
import { getTrustedClientIp } from "./client-ip";

describe("getTrustedClientIp", () => {
  it("uses the Nginx-owned X-Real-IP header", () => {
    const headers = new Headers({
      "x-real-ip": "203.0.113.5",
      "x-forwarded-for": "198.51.100.8",
    });
    expect(getTrustedClientIp(headers)).toBe("203.0.113.5");
  });

  it("does not trust a forwarded-for value by itself", () => {
    const headers = new Headers({
      "x-forwarded-for": "198.51.100.8",
    });
    expect(getTrustedClientIp(headers)).toBe("local");
  });
});
