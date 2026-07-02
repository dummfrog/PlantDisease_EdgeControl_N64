from __future__ import annotations

from datetime import datetime
from typing import Any

from expert_rules import ALARMS, DEVICE_IDS, LIQUID_LEVELS, RELAY_ACTIONS, RISK_LEVELS


class ValidationError(ValueError):
    pass


def _number(payload: dict[str, Any], key: str, *, minimum: float | None = None, maximum: float | None = None) -> None:
    value = payload.get(key)
    if value is None:
        return
    if not isinstance(value, (int, float)):
        raise ValidationError(f"{key} must be a number")
    if minimum is not None and value < minimum:
        raise ValidationError(f"{key} must be >= {minimum}")
    if maximum is not None and value > maximum:
        raise ValidationError(f"{key} must be <= {maximum}")


def validate_json_v1_payload(payload: dict[str, Any]) -> None:
    if payload.get("schema_version") != "1.0":
        raise ValidationError('schema_version must be "1.0"')
    if payload.get("device_id") not in DEVICE_IDS:
        raise ValidationError("device_id must be Node01, Node02, or Node03")
    try:
        timestamp = datetime.fromisoformat(str(payload.get("timestamp", "")).replace("Z", "+00:00"))
    except ValueError as exc:
        raise ValidationError("timestamp must be ISO 8601") from exc
    if timestamp.tzinfo is None:
        raise ValidationError("timestamp must include timezone")
    if not isinstance(payload.get("disease_id"), int) or not 0 <= payload["disease_id"] <= 7:
        raise ValidationError("disease_id must be an integer from 0 to 7")
    _number(payload, "confidence", minimum=0, maximum=1)
    if payload.get("risk_level") not in RISK_LEVELS:
        raise ValidationError("risk_level must be LOW, MEDIUM, HIGH, or CRITICAL")
    if payload.get("pump_action") not in RELAY_ACTIONS:
        raise ValidationError("pump_action must be ON, OFF, NONE, or REQUEST")
    if payload.get("fan_action") not in RELAY_ACTIONS:
        raise ValidationError("fan_action must be ON, OFF, NONE, or REQUEST")
    if payload.get("liquid_level") not in LIQUID_LEVELS:
        raise ValidationError("liquid_level must be OK, LOW, or UNKNOWN")
    if payload.get("alarm") not in ALARMS:
        raise ValidationError("alarm has unsupported value")

    _number(payload, "temperature_c", minimum=-20, maximum=80)
    _number(payload, "humidity_percent", minimum=0, maximum=100)
    _number(payload, "light_lux", minimum=0, maximum=200000)
    _number(payload, "soil_moisture_percent", minimum=0, maximum=100)
    _number(payload, "current_ma", minimum=0, maximum=5000)
    _number(payload, "uptime_ms", minimum=0)
    _number(payload, "pump_duration_s", minimum=0, maximum=3600)
    _number(payload, "fan_duration_s", minimum=0, maximum=3600)
