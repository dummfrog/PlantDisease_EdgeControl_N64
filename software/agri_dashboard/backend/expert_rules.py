from __future__ import annotations

from typing import Any


DEVICE_IDS = ("Node01", "Node02", "Node03")
RISK_LEVELS = ("LOW", "MEDIUM", "HIGH", "CRITICAL")
RELAY_ACTIONS = ("ON", "OFF", "NONE", "REQUEST")
LIQUID_LEVELS = ("OK", "LOW", "UNKNOWN")
ALARMS = (
    "NONE",
    "LOW_LIQUID",
    "CURRENT_ABNORMAL",
    "LOW_CONFIDENCE",
    "SENSOR_ERROR",
    "UNKNOWN",
)

DISEASE_MAP: dict[int, dict[str, Any]] = {
    0: {
        "disease": "Pepper_Bell_Bacterial_Spot",
        "disease_cn": "甜椒细菌性斑点病",
        "risk_level": "MEDIUM",
        "pesticide": "铜制剂、春雷霉素等细菌性病害防治方向，需人工确认后按当地农技规范使用。",
        "suggestion": "清除重病叶，降低叶面湿度，避免喷淋飞溅，并加强通风。",
        "pump_action": "OFF",
        "fan_action": "ON",
        "pump_duration_s": 0,
        "fan_duration_s": 8,
    },
    1: {
        "disease": "Pepper_Bell_Healthy",
        "disease_cn": "甜椒健康",
        "risk_level": "LOW",
        "pesticide": "无需用药。",
        "suggestion": "保持常规巡检，维持适宜通风和水肥管理。",
        "pump_action": "OFF",
        "fan_action": "OFF",
        "pump_duration_s": 0,
        "fan_duration_s": 0,
    },
    2: {
        "disease": "Potato_Early_Blight",
        "disease_cn": "马铃薯早疫病",
        "risk_level": "HIGH",
        "pesticide": "代森锰锌、百菌清、嘧菌酯或吡唑醚菌酯等方向，需人工确认后使用。",
        "suggestion": "及时处理病叶，保护功能叶片，保持水分均衡，病情扩展时人工复核后规范用药。",
        "pump_action": "ON",
        "fan_action": "ON",
        "pump_duration_s": 5,
        "fan_duration_s": 8,
    },
    3: {
        "disease": "Potato_Late_Blight",
        "disease_cn": "马铃薯晚疫病",
        "risk_level": "HIGH",
        "pesticide": "甲霜灵/精甲霜灵与保护剂混用，或霜脲氰、烯酰吗啉等方向，需人工确认后使用。",
        "suggestion": "晚疫病风险高，建议立即人工复核并处理，降低叶面湿度，防止快速扩散。",
        "pump_action": "ON",
        "fan_action": "ON",
        "pump_duration_s": 5,
        "fan_duration_s": 10,
    },
    4: {
        "disease": "Potato_Healthy",
        "disease_cn": "马铃薯健康",
        "risk_level": "LOW",
        "pesticide": "无需用药。",
        "suggestion": "当前识别为健康马铃薯，保持常规巡检和水分管理。",
        "pump_action": "OFF",
        "fan_action": "OFF",
        "pump_duration_s": 0,
        "fan_duration_s": 0,
    },
    5: {
        "disease": "Tomato_Early_Blight",
        "disease_cn": "番茄早疫病",
        "risk_level": "HIGH",
        "pesticide": "百菌清、代森锰锌、铜制剂或嘧菌酯类保护性杀菌剂方向，需人工确认后轮换使用。",
        "suggestion": "清除病叶，控制湿度，改善通风，病情扩展时人工复核后规范用药。",
        "pump_action": "ON",
        "fan_action": "ON",
        "pump_duration_s": 5,
        "fan_duration_s": 8,
    },
    6: {
        "disease": "Tomato_Late_Blight",
        "disease_cn": "番茄晚疫病",
        "risk_level": "HIGH",
        "pesticide": "代森锰锌、百菌清、霜脲氰、烯酰吗啉等方向，需人工确认后按当地农技规范轮换使用。",
        "suggestion": "番茄晚疫病为高风险病害，建议立即人工复核并处理，及时喷药并加强通风降湿。",
        "pump_action": "ON",
        "fan_action": "ON",
        "pump_duration_s": 5,
        "fan_duration_s": 10,
    },
    7: {
        "disease": "Tomato_Healthy",
        "disease_cn": "番茄健康",
        "risk_level": "LOW",
        "pesticide": "无需用药。",
        "suggestion": "当前识别为健康番茄，不建议自动喷药，仅保持监测。",
        "pump_action": "OFF",
        "fan_action": "OFF",
        "pump_duration_s": 0,
        "fan_duration_s": 0,
    },
}


def apply_expert_rules(payload: dict[str, Any]) -> dict[str, Any]:
    disease_id = int(payload["disease_id"])
    confidence = float(payload["confidence"])
    expert = DISEASE_MAP[disease_id]

    normalized = {
        **payload,
        "disease": expert["disease"],
        "disease_cn": expert["disease_cn"],
        "risk_level": expert["risk_level"],
        "pesticide": expert["pesticide"],
        "suggestion": expert["suggestion"],
        "pump_action": payload.get("pump_action") if payload.get("pump_action") == "REQUEST" else expert["pump_action"],
        "fan_action": payload.get("fan_action") if payload.get("fan_action") == "REQUEST" else expert["fan_action"],
        "pump_duration_s": int(payload.get("pump_duration_s") or expert["pump_duration_s"]),
        "fan_duration_s": int(payload.get("fan_duration_s") or expert["fan_duration_s"]),
    }

    if confidence < 0.6:
        normalized.update(
            {
                "risk_level": "LOW",
                "pump_action": "OFF",
                "fan_action": "OFF",
                "pump_duration_s": 0,
                "fan_duration_s": 0,
                "alarm": "LOW_CONFIDENCE",
                "suggestion": f"{expert['suggestion']} 置信度低于 0.6，建议重新采集或人工复核，禁止自动喷药。",
            }
        )

    if normalized.get("liquid_level") == "LOW":
        normalized.update(
            {
                "pump_action": "OFF",
                "pump_duration_s": 0,
                "alarm": "LOW_LIQUID",
                "suggestion": f"{normalized['suggestion']} 药液液位不足，已禁止喷药，请补液后复核。",
            }
        )

    current_ma = normalized.get("current_ma")
    if normalized.get("alarm") == "CURRENT_ABNORMAL" or (
        isinstance(current_ma, (int, float)) and current_ma > 1200
    ):
        normalized.update(
            {
                "pump_action": "OFF",
                "fan_action": "OFF",
                "pump_duration_s": 0,
                "fan_duration_s": 0,
                "alarm": "CURRENT_ABNORMAL",
                "suggestion": f"{normalized['suggestion']} 检测到电流异常，请检查泵、风扇、继电器和供电回路。",
            }
        )

    return normalized
