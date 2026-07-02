from __future__ import annotations

import sys
import unittest
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "backend"))

from expert_rules import apply_expert_rules  # noqa: E402


def payload(**overrides: object) -> dict[str, object]:
    data: dict[str, object] = {
        "schema_version": "1.0",
        "device_id": "Node01",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "uptime_ms": 1,
        "disease_id": 6,
        "disease": "wrong",
        "disease_cn": "wrong",
        "confidence": 0.92,
        "risk_level": "LOW",
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
    data.update(overrides)
    return data


class ExpertRulesTest(unittest.TestCase):
    def test_disease_id_overrides_device_text(self) -> None:
        result = apply_expert_rules(payload(disease_id=6, disease="Tomato_Healthy"))
        self.assertEqual(result["disease"], "Tomato_Late_Blight")
        self.assertEqual(result["risk_level"], "HIGH")
        self.assertEqual(result["pump_action"], "REQUEST")

    def test_healthy_disables_actions(self) -> None:
        result = apply_expert_rules(payload(disease_id=7, confidence=0.97, pump_action="OFF", fan_action="OFF"))
        self.assertEqual(result["risk_level"], "LOW")
        self.assertEqual(result["pump_action"], "OFF")
        self.assertEqual(result["fan_action"], "OFF")

    def test_low_liquid_blocks_spray(self) -> None:
        result = apply_expert_rules(payload(liquid_level="LOW"))
        self.assertEqual(result["pump_action"], "OFF")
        self.assertEqual(result["alarm"], "LOW_LIQUID")
        self.assertIn("液位", result["suggestion"])

    def test_low_confidence_requests_review(self) -> None:
        result = apply_expert_rules(payload(confidence=0.51))
        self.assertEqual(result["pump_action"], "OFF")
        self.assertEqual(result["alarm"], "LOW_CONFIDENCE")
        self.assertIn("人工复核", result["suggestion"])

    def test_current_abnormal_blocks_actuation(self) -> None:
        result = apply_expert_rules(payload(current_ma=1400))
        self.assertEqual(result["pump_action"], "OFF")
        self.assertEqual(result["fan_action"], "OFF")
        self.assertEqual(result["alarm"], "CURRENT_ABNORMAL")


if __name__ == "__main__":
    unittest.main()
