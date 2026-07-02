from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


PROJECT_ROOT = Path(__file__).resolve().parents[1]
ENV_PATH = PROJECT_ROOT / ".env"


def load_env(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.exists():
        return values

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip().strip('"').strip("'")
    return values


def build_payload(bad_disease_id: bool = False) -> dict[str, object]:
    return {
        "schema_version": "1.0",
        "device_id": "Node01",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "uptime_ms": 123456,
        "disease_id": 99 if bad_disease_id else 6,
        "disease": "Tomato_Late_Blight",
        "disease_cn": "番茄晚疫病",
        "confidence": 0.92,
        "risk_level": "HIGH",
        "temperature_c": 28.6,
        "humidity_percent": 78.2,
        "light_lux": 13500,
        "soil_moisture_percent": 42,
        "liquid_level": "OK",
        "pump_action": "REQUEST",
        "fan_action": "REQUEST",
        "pump_duration_s": 5,
        "fan_duration_s": 10,
        "current_ma": 680,
        "system_status": "ONLINE",
        "sensor_status": "OK",
        "alarm": "NONE",
    }


def sign_body(body: bytes, secret: str) -> str:
    return hmac.new(secret.encode("utf-8"), body, hashlib.sha256).hexdigest()


def post_json(url: str, payload: dict[str, object], env: dict[str, str]) -> tuple[int, str]:
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    token = env.get("DEVICE_TOKEN_NODE01", "")
    hmac_secret = env.get("DEVICE_HMAC_SECRET_NODE01", "")
    headers = {
        "Content-Type": "application/json",
        "X-Device-Id": "Node01",
        "X-Device-Token": token,
        "X-Timestamp": str(payload["timestamp"]),
    }
    if env.get("REQUIRE_HMAC", "true").lower() != "false":
        headers["X-Signature"] = sign_body(body, hmac_secret)

    request = Request(url, data=body, method="POST", headers=headers)
    try:
        with urlopen(request, timeout=30) as response:
            return response.status, response.read().decode("utf-8")
    except HTTPError as exc:
        return exc.code, exc.read().decode("utf-8")
    except URLError as exc:
        raise RuntimeError(f"request failed: {exc.reason}") from exc


def main() -> int:
    parser = argparse.ArgumentParser(description="Test Supabase Edge Function device-upload.")
    parser.add_argument("url", help="Local or hosted Edge Function URL.")
    parser.add_argument("--bad-disease-id", action="store_true", help="Send an invalid disease_id.")
    args = parser.parse_args()

    env = load_env(ENV_PATH)
    missing = [key for key in ["DEVICE_TOKEN_NODE01", "DEVICE_HMAC_SECRET_NODE01"] if not env.get(key)]
    if missing:
        print(f"Missing required local secret(s): {', '.join(missing)}")
        print("Set them in .env for testing; do not commit real secrets.")
        return 2

    try:
        status, text = post_json(args.url, build_payload(args.bad_disease_id), env)
    except RuntimeError as exc:
        print(str(exc))
        return 1

    print(f"HTTP status: {status}")
    try:
        parsed = json.loads(text)
        print(json.dumps(parsed, ensure_ascii=False, indent=2))
        if parsed.get("ok") is True:
            print("Success. Open the Dashboard and check the newest diagnosis_records row.")
            return 0
        return 1
    except json.JSONDecodeError:
        print(text)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
