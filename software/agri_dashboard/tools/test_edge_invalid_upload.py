from __future__ import annotations

import argparse
import hashlib
import hmac
import json
from copy import deepcopy
from datetime import datetime, timedelta, timezone
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


def base_payload() -> dict[str, object]:
    return {
        "schema_version": "1.0",
        "device_id": "Node01",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "uptime_ms": 123456,
        "disease_id": 6,
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


def send_case(
    url: str,
    payload: dict[str, object],
    env: dict[str, str],
    *,
    token: str | None = None,
    device_id_header: str = "Node01",
    signature_override: str | None = None,
    content_type: str = "application/json",
    omit_token: bool = False,
) -> tuple[int, dict[str, object] | str]:
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    headers = {
        "Content-Type": content_type,
        "X-Device-Id": device_id_header,
        "X-Timestamp": str(payload.get("timestamp", "")),
    }
    if not omit_token:
        headers["X-Device-Token"] = token if token is not None else env.get("DEVICE_TOKEN_NODE01", "")
    if env.get("REQUIRE_HMAC", "true").lower() != "false":
        headers["X-Signature"] = signature_override or sign_body(body, env.get("DEVICE_HMAC_SECRET_NODE01", ""))

    request = Request(url, data=body, method="POST", headers=headers)
    try:
        with urlopen(request, timeout=30) as response:
            text = response.read().decode("utf-8")
            return response.status, json.loads(text)
    except HTTPError as exc:
        text = exc.read().decode("utf-8")
        try:
            return exc.code, json.loads(text)
        except json.JSONDecodeError:
            return exc.code, text
    except URLError as exc:
        return 0, f"request failed: {exc.reason}"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run invalid device-upload Edge Function cases.")
    parser.add_argument("url", help="Local or hosted Edge Function URL.")
    args = parser.parse_args()

    env = load_env(ENV_PATH)
    missing = [key for key in ["DEVICE_TOKEN_NODE01", "DEVICE_HMAC_SECRET_NODE01"] if not env.get(key)]
    if missing:
        print(f"Missing required local secret(s): {', '.join(missing)}")
        print("Set them in .env for testing; do not commit real secrets.")
        return 2

    expired_payload = base_payload()
    expired_payload["timestamp"] = (datetime.now(timezone.utc) - timedelta(hours=2)).isoformat()

    bad_schema_payload = base_payload()
    bad_schema_payload["schema_version"] = "2.0"

    bad_disease_payload = base_payload()
    bad_disease_payload["disease_id"] = 99

    bad_confidence_payload = base_payload()
    bad_confidence_payload["confidence"] = 1.5

    cases = [
        ("missing X-Device-Token", base_payload(), {"omit_token": True}),
        ("wrong X-Device-Token", base_payload(), {"token": "INVALID_DEVICE_TOKEN"}),
        ("header/body device mismatch", base_payload(), {"device_id_header": "Node02"}),
        ("wrong HMAC", base_payload(), {"signature_override": "0" * 64}),
        ("expired timestamp", expired_payload, {}),
        ("wrong schema_version", bad_schema_payload, {}),
        ("disease_id = 99", bad_disease_payload, {}),
        ("confidence = 1.5", bad_confidence_payload, {}),
        ("wrong Content-Type", base_payload(), {"content_type": "text/plain"}),
    ]

    failures = 0
    for name, payload, options in cases:
        status, result = send_case(args.url, deepcopy(payload), env, **options)
        ok = isinstance(result, dict) and result.get("ok") is True
        rejected = status >= 400 and not ok
        print(f"[{name}] HTTP {status} -> {'REJECTED' if rejected else 'FAILED'}")
        if isinstance(result, dict):
            print(json.dumps({"ok": result.get("ok"), "error": result.get("error")}, ensure_ascii=False))
        else:
            print(result)
        if not rejected:
            failures += 1

    if failures:
        print(f"SERIOUS FAILURE: {failures} invalid case(s) were not rejected.")
        return 1
    print("All invalid cases were rejected.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
