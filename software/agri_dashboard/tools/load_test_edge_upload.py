from __future__ import annotations

import argparse
import json
import time
from pathlib import Path

from test_edge_device_upload import build_payload, load_env, post_json


PROJECT_ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description="Simple load test for device-upload Edge Function.")
    parser.add_argument("url", help="Local or hosted Edge Function URL.")
    parser.add_argument("--count", type=int, default=100, help="Number of JSON v1.0 uploads.")
    args = parser.parse_args()

    env = load_env(PROJECT_ROOT / ".env")
    success = 0
    failure = 0
    durations: list[float] = []

    for index in range(max(1, args.count)):
        payload = build_payload()
        payload["uptime_ms"] = 123456 + index
        started = time.perf_counter()
        status, text = post_json(args.url, payload, env)
        durations.append((time.perf_counter() - started) * 1000)
        try:
            body = json.loads(text)
        except json.JSONDecodeError:
            body = {"ok": False, "error": text}
        if 200 <= status < 300 and body.get("ok") is True:
            success += 1
        else:
            failure += 1
            print(f"[{index + 1}] HTTP {status}: {body}")

    average = sum(durations) / len(durations)
    print(json.dumps({"count": args.count, "success": success, "failure": failure, "average_ms": round(average, 2)}, ensure_ascii=False, indent=2))
    return 0 if failure == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
