import { describe, expect, it } from "vitest";
import { TokenBucketRateLimiter } from "./rate-limiter";

describe("TokenBucketRateLimiter", () => {
  it("enforces the limit and refills over the configured window", () => {
    const limiter = new TokenBucketRateLimiter({ refillWindowMs: 60_000 });

    expect(limiter.consume("client:login", 2, 0)).toBe(true);
    expect(limiter.consume("client:login", 2, 0)).toBe(true);
    expect(limiter.consume("client:login", 2, 0)).toBe(false);
    expect(limiter.consume("client:login", 2, 30_000)).toBe(true);
    expect(limiter.consume("client:login", 2, 30_000)).toBe(false);
  });

  it("rejects new buckets at capacity without resetting active clients", () => {
    const limiter = new TokenBucketRateLimiter({
      capacity: 2,
      refillWindowMs: 60_000,
    });

    expect(limiter.consume("first", 1, 0)).toBe(true);
    expect(limiter.consume("second", 1, 0)).toBe(true);
    expect(limiter.consume("third", 1, 1)).toBe(false);
    expect(limiter.size).toBe(2);
    expect(limiter.consume("first", 1, 1)).toBe(false);
  });

  it("reuses capacity only after the least-recent bucket is fully refilled", () => {
    const limiter = new TokenBucketRateLimiter({
      capacity: 2,
      refillWindowMs: 60_000,
    });

    expect(limiter.consume("first", 1, 0)).toBe(true);
    expect(limiter.consume("second", 1, 1)).toBe(true);
    expect(limiter.consume("second", 1, 59_999)).toBe(false);
    expect(limiter.consume("third", 1, 60_000)).toBe(true);
    expect(limiter.size).toBe(2);
    expect(limiter.consume("first", 1, 60_000)).toBe(false);
  });
});
