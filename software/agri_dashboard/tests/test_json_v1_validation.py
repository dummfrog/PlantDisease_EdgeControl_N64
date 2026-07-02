from __future__ import annotations

import sys
import unittest
from copy import deepcopy
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "backend"))

from validators import ValidationError, validate_json_v1_payload  # noqa: E402


def valid_payload() -> dict[str, object]:
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


class JsonV1ValidationTest(unittest.TestCase):
    def assert_invalid(self, key: str, value: object) -> None:
        payload = valid_payload()
        payload[key] = value
        with self.assertRaises(ValidationError):
            validate_json_v1_payload(payload)

    def test_valid_high_risk_node01(self) -> None:
        validate_json_v1_payload(valid_payload())

    def test_valid_healthy(self) -> None:
        payload = valid_payload()
        payload.update({"disease_id": 7, "disease": "Tomato_Healthy", "confidence": 0.98, "risk_level": "LOW"})
        validate_json_v1_payload(payload)

    def test_invalid_schema(self) -> None:
        self.assert_invalid("schema_version", "2.0")

    def test_invalid_device(self) -> None:
        self.assert_invalid("device_id", "Node99")

    def test_invalid_disease_low(self) -> None:
        self.assert_invalid("disease_id", -1)

    def test_invalid_disease_high(self) -> None:
        self.assert_invalid("disease_id", 99)

    def test_invalid_confidence_low(self) -> None:
        self.assert_invalid("confidence", -0.1)

    def test_invalid_confidence_high(self) -> None:
        self.assert_invalid("confidence", 1.5)

    def test_invalid_risk(self) -> None:
        self.assert_invalid("risk_level", "SEVERE")

    def test_invalid_liquid_level(self) -> None:
        self.assert_invalid("liquid_level", "EMPTY")

    def test_request_actions_are_valid(self) -> None:
        payload = valid_payload()
        payload["pump_action"] = "REQUEST"
        payload["fan_action"] = "REQUEST"
        validate_json_v1_payload(payload)


if __name__ == "__main__":
    unittest.main()
