from __future__ import annotations

import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "backend"))

import app as app_module  # noqa: E402

app = app_module.app


class DashboardApiTest(unittest.TestCase):
    def setUp(self) -> None:
        self.client = app.test_client()

    def assert_ok_json(self, path: str) -> dict:
        response = self.client.get(path)
        self.assertEqual(response.status_code, 200, path)
        self.assertTrue(response.is_json, path)
        return response.get_json()

    def test_health(self) -> None:
        data = self.assert_ok_json("/api/health")
        self.assertEqual(data["status"], "ok")

    def test_stats(self) -> None:
        data = self.assert_ok_json("/api/stats")
        self.assertIn("summary", data)

    def test_history(self) -> None:
        data = self.assert_ok_json("/api/history?limit=5")
        self.assertIn("records", data)

    def test_node_status(self) -> None:
        data = self.assert_ok_json("/api/node_status")
        self.assertIn("nodes", data)

    def test_alerts(self) -> None:
        data = self.assert_ok_json("/api/alerts")
        self.assertIn("alerts", data)

    def test_heatmap_matrix(self) -> None:
        data = self.assert_ok_json("/api/heatmap_matrix")
        self.assertIn("hours", data)
        self.assertIn("nodes", data)

    def test_model_status(self) -> None:
        data = self.assert_ok_json("/api/model/status")
        self.assertIn("model", data)

    def test_simulate_device(self) -> None:
        original_insert = app_module.insert_detection
        app_module.insert_detection = lambda record: record
        response = self.client.post("/api/simulate_device", json={"device_id": "Node01"})
        try:
            self.assertEqual(response.status_code, 200)
            self.assertTrue(response.is_json)
            self.assertIn("record", response.get_json())
        finally:
            app_module.insert_detection = original_insert


if __name__ == "__main__":
    unittest.main()
