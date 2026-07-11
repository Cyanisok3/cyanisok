type TokenBucket = {
  tokens: number;
  updatedAt: number;
};

type TokenBucketRateLimiterOptions = {
  capacity?: number;
  refillWindowMs?: number;
};

export class TokenBucketRateLimiter {
  private readonly buckets = new Map<string, TokenBucket>();
  private readonly capacity: number;
  private readonly refillWindowMs: number;

  constructor({
    capacity = 10_000,
    refillWindowMs = 60_000,
  }: TokenBucketRateLimiterOptions = {}) {
    if (capacity <= 0 || refillWindowMs <= 0) {
      throw new Error("Rate limiter capacity and refill window must be positive");
    }
    this.capacity = capacity;
    this.refillWindowMs = refillWindowMs;
  }

  consume(key: string, limit: number, now = Date.now()) {
    if (limit <= 0) return false;

    const bucket = this.buckets.get(key);
    if (bucket) {
      const effectiveNow = Math.max(now, bucket.updatedAt);
      const elapsedMs = effectiveNow - bucket.updatedAt;
      const available = Math.min(
        limit,
        bucket.tokens + (elapsedMs * limit) / this.refillWindowMs
      );

      bucket.tokens = available;
      bucket.updatedAt = effectiveNow;
      this.touch(key, bucket);

      if (available < 1) return false;

      bucket.tokens -= 1;
      return true;
    }

    if (this.buckets.size >= this.capacity && !this.releaseIdleBucket(now)) {
      return false;
    }

    this.buckets.set(key, {
      tokens: Math.max(0, limit - 1),
      updatedAt: now,
    });
    return true;
  }

  get size() {
    return this.buckets.size;
  }

  private touch(key: string, bucket: TokenBucket) {
    this.buckets.delete(key);
    this.buckets.set(key, bucket);
  }

  private releaseIdleBucket(now: number) {
    const oldestEntry = this.buckets.entries().next().value as
      | [string, TokenBucket]
      | undefined;
    if (!oldestEntry) return true;

    const [oldestKey, oldestBucket] = oldestEntry;
    if (now - oldestBucket.updatedAt < this.refillWindowMs) return false;

    this.buckets.delete(oldestKey);
    return true;
  }
}
