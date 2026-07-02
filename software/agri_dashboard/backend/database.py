from __future__ import annotations

import json
import os
import random
from collections import Counter, defaultdict
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any
from uuid import uuid4

from dotenv import load_dotenv
from supabase import Client, create_client


DASHBOARD_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = DASHBOARD_ROOT.parent
UPLOAD_DIR = DASHBOARD_ROOT / "uploads"

DEFAULT_MODEL_PATH = (
    WORKSPACE_ROOT
    / "plant_disease_yolo"
    / "runs"
    / "detect"
    / "train30_gpu"
    / "weights"
    / "best.pt"
)
DEFAULT_ONNX_PATH = DEFAULT_MODEL_PATH.with_suffix(".onnx")
KB_V1_PATH = WORKSPACE_ROOT / "agri_expert_system" / "disease_knowledge_base.json"
KB_V2_PATH = WORKSPACE_ROOT / "agri_expert_system" / "disease_knowledge_base_v2.json"

load_dotenv(DASHBOARD_ROOT / ".env")
load_dotenv()

SUPABASE_URL = os.getenv("SUPABASE_URL", "").strip()
SUPABASE_KEY = (
    os.getenv("SUPABASE_SERVICE_ROLE_KEY")
    or os.getenv("SUPABASE_KEY")
    or os.getenv("SUPABASE_ANON_KEY")
    or ""
).strip()
SUPABASE_TABLE = os.getenv("SUPABASE_TABLE", "diagnosis_records").strip()
SUPABASE_ACTION_TABLE = os.getenv("SUPABASE_ACTION_TABLE", "action_records").strip()
MODEL_PATH = Path(os.getenv("YOLO_MODEL_PATH", str(DEFAULT_MODEL_PATH))).expanduser()
ONNX_PATH = Path(os.getenv("YOLO_ONNX_PATH", str(DEFAULT_ONNX_PATH))).expanduser()
YOLO_IMGSZ = int(os.getenv("YOLO_IMGSZ", "224"))

CHINA_TZ = timezone(timedelta(hours=8))
DEVICE_IDS = ["Node01", "Node02", "Node03", "STM32N647_001"]
AREA_MAP = {
    "Node01": "番茄区",
    "Node02": "甜椒区",
    "Node03": "马铃薯区",
    "STM32N647_001": "演示节点",
    "STM32N647-UPLOAD": "网页上传",
}
ALERT_RISKS = {"MEDIUM", "MEDIUM_HIGH", "HIGH", "CRITICAL"}
RISK_RANK = {"UNKNOWN": 0, "LOW": 1, "MEDIUM": 2, "MEDIUM_HIGH": 3, "HIGH": 4, "CRITICAL": 5}
RISK_SCORE = {"LOW": 0, "UNKNOWN": 0, "MEDIUM": 2, "MEDIUM_HIGH": 3, "HIGH": 4, "CRITICAL": 5}

MODEL_NAME_ALIASES = {
    "Pepper,_bell___Bacterial_spot": "Pepper_Bell_Bacterial_Spot",
    "Pepper,_bell___healthy": "Pepper_Bell_Healthy",
    "Potato___Early_blight": "Potato_Early_Blight",
    "Potato___Late_blight": "Potato_Late_Blight",
    "Potato___healthy": "Healthy_Potato",
    "Tomato___Early_blight": "Tomato_Early_Blight",
    "Tomato___Late_blight": "Tomato_Late_Blight",
    "Tomato___healthy": "Healthy_Tomato",
}

CLASS_NAMES = [
    "Healthy_Potato",
    "Healthy_Tomato",
    "Potato_Early_Blight",
    "Potato_Late_Blight",
    "Tomato_Early_Blight",
    "Tomato_Late_Blight",
    "Tomato_Leaf_Mold",
    "Tomato_Septoria_Leaf_Spot",
]

DISPLAY_FALLBACK = {
    "Pepper_Bell_Bacterial_Spot": "甜椒细菌性斑点病",
    "Pepper_Bell_Healthy": "甜椒健康",
    "Healthy_Potato": "健康马铃薯",
    "Potato_Healthy": "健康马铃薯",
    "Healthy_Tomato": "健康番茄",
    "Tomato_Healthy": "健康番茄",
    "Potato_Early_Blight": "马铃薯早疫病",
    "Potato_Late_Blight": "马铃薯晚疫病",
    "Tomato_Early_Blight": "番茄早疫病",
    "Tomato_Late_Blight": "番茄晚疫病",
    "Tomato_Leaf_Mold": "番茄叶霉病",
    "Tomato_Septoria_Leaf_Spot": "番茄斑枯病",
}

MEMORY_RECORDS: list[dict[str, Any]] = []
_supabase: Client | None = None
_model: Any | None = None
_model_error: str | None = None


def now_dt() -> datetime:
    return datetime.now(CHINA_TZ)


def now_iso() -> str:
    return now_dt().isoformat(timespec="seconds")


def normalize_key(value: Any) -> str:
    return (
        str(value or "")
        .strip()
        .replace(",", "")
        .replace("___", "_")
        .replace("__", "_")
        .replace("-", "_")
        .replace(" ", "_")
        .lower()
    )


def canonical_disease(value: Any) -> str:
    raw = str(value or "").strip()
    if not raw:
        return "Tomato_Late_Blight"
    if raw in MODEL_NAME_ALIASES:
        return MODEL_NAME_ALIASES[raw]

    normalized = normalize_key(raw)
    aliases = {
        "healthy_potato": "Healthy_Potato",
        "potato_healthy": "Healthy_Potato",
        "potato_early_blight": "Potato_Early_Blight",
        "potato_late_blight": "Potato_Late_Blight",
        "healthy_tomato": "Healthy_Tomato",
        "tomato_healthy": "Healthy_Tomato",
        "tomato_early_blight": "Tomato_Early_Blight",
        "tomato_late_blight": "Tomato_Late_Blight",
        "tomato_leaf_mold": "Tomato_Leaf_Mold",
        "tomato_septoria_leaf_spot": "Tomato_Septoria_Leaf_Spot",
        "pepper_bell_bacterial_spot": "Pepper_Bell_Bacterial_Spot",
        "pepper_bell_healthy": "Pepper_Bell_Healthy",
        "pepper_bell_bacterial_spot": "Pepper_Bell_Bacterial_Spot",
    }
    if normalized in aliases:
        return aliases[normalized]
    for name in CLASS_NAMES:
        if normalize_key(name) == normalized:
            return name
    return raw


def class_id_for_disease(disease: str, fallback: int | None = None) -> int | None:
    disease = canonical_disease(disease)
    if disease in CLASS_NAMES:
        return CLASS_NAMES.index(disease)
    return fallback


def parse_timestamp(value: Any) -> datetime | None:
    if not value:
        return None
    try:
        timestamp = datetime.fromisoformat(str(value).replace("Z", "+00:00"))
        if timestamp.tzinfo is None:
            timestamp = timestamp.replace(tzinfo=CHINA_TZ)
        return timestamp.astimezone(CHINA_TZ)
    except ValueError:
        return None


def load_knowledge_items() -> dict[str, dict[str, Any]]:
    items: dict[str, dict[str, Any]] = {}
    for path in (KB_V1_PATH, KB_V2_PATH):
        if not path.exists():
            continue
        with path.open("r", encoding="utf-8") as file:
            data = json.load(file)
        for item in data.get("diseases", []):
            names = {
                item.get("disease"),
                item.get("disease_name"),
                item.get("disease_id"),
            }
            for name in names:
                if name is None:
                    continue
                canonical = canonical_disease(name)
                items[canonical] = item
                if canonical == "Healthy_Potato":
                    items["Potato_Healthy"] = item
                if canonical == "Healthy_Tomato":
                    items["Tomato_Healthy"] = item
    return items


EXPERT_KNOWLEDGE = load_knowledge_items()


def knowledge_for_disease(disease: str, confidence: float = 0) -> dict[str, Any]:
    disease = canonical_disease(disease)
    item = EXPERT_KNOWLEDGE.get(disease) or {}
    action = item.get("action") or ("NONE" if "Healthy" in disease else "SPRAY_AND_VENTILATE")
    relay = item.get("relay_action") or {}
    pump_on = bool(relay.get("pump")) or item.get("pump_action") == "ON" or action in {"SPRAY", "SPRAY_AND_VENTILATE"}
    fan_on = bool(relay.get("fan")) or item.get("fan_action") == "ON" or action in {"VENTILATE", "SPRAY_AND_VENTILATE"}
    if action == "NONE":
        pump_on = False
        fan_on = False

    risk_level = item.get("risk_level") or ("LOW" if "Healthy" in disease else "HIGH")
    alarm = item.get("alarm") or "NONE"
    if confidence and confidence < 0.6:
        alarm = "LOW_CONFIDENCE_REVIEW"

    return {
        "disease": disease,
        "disease_cn": item.get("disease_cn") or item.get("chinese_name") or DISPLAY_FALLBACK.get(disease, disease),
        "chinese_name": item.get("chinese_name") or item.get("disease_cn") or DISPLAY_FALLBACK.get(disease, disease),
        "description": item.get("description") or item.get("expert_summary") or "",
        "symptom": item.get("symptom") or item.get("typical_symptoms") or "",
        "severity": item.get("severity") or "",
        "risk_level": risk_level,
        "recommended_pesticide": item.get("recommended_pesticide") or item.get("pesticide") or "请人工复核后处理。",
        "pesticide": item.get("pesticide") or item.get("recommended_pesticide") or "请人工复核后处理。",
        "spray_interval": item.get("spray_interval") or "按农技规范复核后执行。",
        "ventilation_required": bool(item.get("ventilation_required", fan_on)),
        "irrigation_recommendation": item.get("irrigation_recommendation") or "",
        "suggestion": item.get("suggestion") or item.get("expert_summary") or item.get("irrigation_recommendation") or "请人工复核。",
        "expert_summary": item.get("expert_summary") or item.get("description") or "",
        "action": action,
        "pump_action": "ON" if pump_on else "OFF",
        "fan_action": "ON" if fan_on else "OFF",
        "pump": pump_on,
        "fan": fan_on,
        "pump_duration_s": int(item.get("pump_duration_s") or (5 if pump_on else 0)),
        "fan_duration_s": int(item.get("fan_duration_s") or (8 if fan_on else 0)),
        "alarm": alarm,
    }


def get_supabase() -> Client | None:
    global _supabase
    if _supabase is not None:
        return _supabase
    if not SUPABASE_URL or not SUPABASE_KEY:
        return None
    _supabase = create_client(SUPABASE_URL, SUPABASE_KEY)
    return _supabase


def supabase_connected() -> bool:
    return bool(SUPABASE_URL and SUPABASE_KEY)


def model_status() -> dict[str, Any]:
    path = MODEL_PATH if MODEL_PATH.exists() else ONNX_PATH
    return {
        "configured_path": str(MODEL_PATH),
        "exists": MODEL_PATH.exists() or ONNX_PATH.exists(),
        "active_path": str(path) if path.exists() else "",
        "model_name": os.getenv("MODEL_NAME", MODEL_PATH.name),
        "model_version": os.getenv("MODEL_VERSION", "field-demo-v1"),
        "class_count": len(CLASS_NAMES),
        "class_mapping": {index: name for index, name in enumerate(CLASS_NAMES)},
        "task_type": "classify",
        "onnx_supported": ONNX_PATH.exists(),
        "last_inference_at": None,
        "average_latency_ms": None,
        "last_error": _model_error,
    }


def get_yolo_model() -> Any | None:
    global _model, _model_error
    if _model is not None:
        return _model
    path = MODEL_PATH if MODEL_PATH.exists() else ONNX_PATH
    if not path.exists():
        _model_error = f"model not found: {path}"
        return None
    try:
        from ultralytics import YOLO

        _model = YOLO(str(path))
        _model_error = None
        return _model
    except Exception as exc:  # pragma: no cover - depends on local ML runtime
        _model_error = str(exc)
        return None


def mock_environment() -> dict[str, Any]:
    return {
        "temperature_c": round(random.uniform(22.0, 31.5), 1),
        "humidity_percent": round(random.uniform(58.0, 89.0), 1),
        "light_lux": random.randint(7500, 21000),
        "soil_moisture_percent": random.randint(32, 64),
        "liquid_level": "OK",
        "current_ma": random.randint(460, 820),
    }


def mock_yolo_detect(disease: str | None = None) -> dict[str, Any]:
    selected = canonical_disease(disease or random.choice(CLASS_NAMES))
    confidence = random.uniform(0.86, 0.97) if "Healthy" in selected else random.uniform(0.92, 0.99)
    return build_prediction_result(
        disease=selected,
        confidence=round(confidence, 4),
        raw_class=selected,
        inference_source="simulated",
        model_name="mock",
    )


def infer_image(image_path: Path) -> dict[str, Any]:
    model = get_yolo_model()
    if model is None:
        return mock_yolo_detect()

    try:
        result = model.predict(source=str(image_path), imgsz=YOLO_IMGSZ, verbose=False)[0]
        names = result.names or getattr(model, "names", {})
        if getattr(result, "probs", None) is None:
            raise RuntimeError("YOLO result has no classification probabilities")
        class_index = int(result.probs.top1)
        raw_class = str(names.get(class_index, class_index) if isinstance(names, dict) else names[class_index])
        confidence = round(float(result.probs.top1conf), 4)
        disease = canonical_disease(raw_class)
        return build_prediction_result(
            disease=disease,
            confidence=confidence,
            raw_class=raw_class,
            inference_source="yolo",
            model_name=Path(str(model.ckpt_path or MODEL_PATH)).name if hasattr(model, "ckpt_path") else MODEL_PATH.name,
            class_id=class_index,
            image_name=image_path.name,
        )
    except Exception as exc:  # pragma: no cover - depends on local ML runtime
        fallback = mock_yolo_detect()
        fallback["inference_source"] = "simulated_yolo_error"
        fallback["error"] = str(exc)
        return fallback


def build_prediction_result(
    *,
    disease: str,
    confidence: float,
    raw_class: str,
    inference_source: str,
    model_name: str,
    class_id: int | None = None,
    image_name: str = "",
) -> dict[str, Any]:
    disease = canonical_disease(disease)
    knowledge = knowledge_for_disease(disease, confidence)
    resolved_class_id = class_id if class_id is not None else class_id_for_disease(disease)
    return {
        "schema_version": "1.0",
        "timestamp": now_iso(),
        "disease_id": str(resolved_class_id if resolved_class_id is not None else disease),
        "disease_class_id": resolved_class_id,
        "disease": disease,
        "disease_cn": knowledge["disease_cn"],
        "chinese_name": knowledge["chinese_name"],
        "confidence": confidence,
        "raw_class": raw_class,
        "risk_level": knowledge["risk_level"],
        "risk": knowledge["risk_level"],
        "action": knowledge["action"],
        "pesticide": knowledge["pesticide"],
        "recommended_pesticide": knowledge["recommended_pesticide"],
        "spray_interval": knowledge["spray_interval"],
        "ventilation_required": knowledge["ventilation_required"],
        "irrigation_recommendation": knowledge["irrigation_recommendation"],
        "suggestion": knowledge["suggestion"],
        "advice": knowledge["suggestion"],
        "expert_summary": knowledge["expert_summary"],
        "pump": knowledge["pump"],
        "fan": knowledge["fan"],
        "pump_action": knowledge["pump_action"],
        "fan_action": knowledge["fan_action"],
        "pump_duration_s": knowledge["pump_duration_s"],
        "fan_duration_s": knowledge["fan_duration_s"],
        "action_duration_s": knowledge["pump_duration_s"],
        "alarm": knowledge["alarm"],
        "inference_source": inference_source,
        "model_name": model_name,
        "image_name": image_name,
        **mock_environment(),
    }


def normalize_record(record: dict[str, Any]) -> dict[str, Any]:
    disease = canonical_disease(record.get("disease") or record.get("disease_name") or record.get("disease_id"))
    confidence = float(record.get("confidence") or 0)
    knowledge = knowledge_for_disease(disease, confidence)
    timestamp = record.get("timestamp") or record.get("created_at") or now_iso()
    pump_action = record.get("pump_action") or ("ON" if record.get("pump") else knowledge["pump_action"])
    fan_action = record.get("fan_action") or ("ON" if record.get("fan") else knowledge["fan_action"])
    class_id = record.get("disease_class_id")
    if class_id is None:
        class_id = class_id_for_disease(disease)

    normalized = {
        "id": record.get("id") or str(uuid4()),
        "schema_version": record.get("schema_version") or "1.0",
        "timestamp": timestamp,
        "created_at": record.get("created_at") or timestamp,
        "device_id": record.get("device_id") or "STM32N647_001",
        "image_url": record.get("image_url") or "",
        "disease_id": str(record.get("disease_id") if record.get("disease_id") is not None else class_id if class_id is not None else disease),
        "disease_class_id": class_id,
        "disease": disease,
        "disease_cn": record.get("disease_cn") or record.get("chinese_name") or knowledge["disease_cn"],
        "chinese_name": record.get("chinese_name") or record.get("disease_cn") or knowledge["chinese_name"],
        "confidence": confidence,
        "risk_level": record.get("risk_level") or record.get("risk") or knowledge["risk_level"],
        "action": record.get("action") or knowledge["action"],
        "pesticide": record.get("pesticide") or record.get("recommended_pesticide") or knowledge["pesticide"],
        "recommended_pesticide": record.get("recommended_pesticide") or record.get("pesticide") or knowledge["recommended_pesticide"],
        "spray_interval": record.get("spray_interval") or knowledge["spray_interval"],
        "ventilation_required": record.get("ventilation_required", knowledge["ventilation_required"]),
        "irrigation_recommendation": record.get("irrigation_recommendation") or knowledge["irrigation_recommendation"],
        "suggestion": record.get("suggestion") or record.get("advice") or knowledge["suggestion"],
        "advice": record.get("advice") or record.get("suggestion") or knowledge["suggestion"],
        "expert_summary": record.get("expert_summary") or knowledge["expert_summary"],
        "pump": pump_action == "ON",
        "fan": fan_action == "ON",
        "pump_action": pump_action,
        "fan_action": fan_action,
        "pump_duration_s": int(record.get("pump_duration_s") or record.get("action_duration_s") or 0),
        "fan_duration_s": int(record.get("fan_duration_s") or (8 if fan_action == "ON" else 0)),
        "action_duration_s": int(record.get("action_duration_s") or record.get("pump_duration_s") or 0),
        "temperature_c": record.get("temperature_c", record.get("temperature")),
        "humidity_percent": record.get("humidity_percent", record.get("humidity")),
        "light_lux": record.get("light_lux", record.get("light")),
        "soil_moisture_percent": record.get("soil_moisture_percent", record.get("soil_moisture")),
        "temperature": record.get("temperature", record.get("temperature_c")),
        "humidity": record.get("humidity", record.get("humidity_percent")),
        "light": record.get("light", record.get("light_lux")),
        "soil_moisture": record.get("soil_moisture", record.get("soil_moisture_percent")),
        "liquid_level": record.get("liquid_level") or "OK",
        "current_ma": record.get("current_ma"),
        "system_status": record.get("system_status") or "UNKNOWN",
        "sensor_status": record.get("sensor_status") or "UNKNOWN",
        "alarm": record.get("alarm") or knowledge["alarm"],
        "raw_class": record.get("raw_class") or "",
        "inference_source": record.get("inference_source") or record.get("source") or "legacy",
        "model_name": record.get("model_name") or "",
        "image_name": record.get("image_name") or "",
    }
    return normalized


def db_payload_from_record(record: dict[str, Any]) -> dict[str, Any]:
    item = normalize_record(record)
    payload = {
        "schema_version": item["schema_version"],
        "timestamp": item["timestamp"],
        "device_id": item["device_id"],
        "image_url": item["image_url"] or None,
        "disease_id": item["disease_id"],
        "disease_class_id": item["disease_class_id"],
        "disease": item["disease"],
        "disease_cn": item["disease_cn"],
        "confidence": item["confidence"],
        "risk_level": item["risk_level"],
        "pesticide": item["pesticide"],
        "suggestion": item["suggestion"],
        "pump_action": item["pump_action"],
        "fan_action": item["fan_action"],
        "pump_duration_s": item["pump_duration_s"],
        "fan_duration_s": item["fan_duration_s"],
        "action_duration_s": item["action_duration_s"],
        "temperature_c": item["temperature_c"],
        "humidity_percent": item["humidity_percent"],
        "light_lux": item["light_lux"],
        "soil_moisture_percent": item["soil_moisture_percent"],
        "temperature": item["temperature"],
        "humidity": item["humidity"],
        "light": item["light"],
        "soil_moisture": item["soil_moisture"],
        "liquid_level": item["liquid_level"],
        "current_ma": item["current_ma"],
        "system_status": item["system_status"],
        "sensor_status": item["sensor_status"],
        "alarm": item["alarm"],
        "inference_source": item["inference_source"],
        "model_name": item["model_name"],
        "image_name": item["image_name"],
    }
    return {key: value for key, value in payload.items() if value is not None}


def insert_action_record(record: dict[str, Any]) -> None:
    client = get_supabase()
    if not client or not SUPABASE_ACTION_TABLE:
        return
    try:
        item = normalize_record(record)
        client.table(SUPABASE_ACTION_TABLE).insert(
            {
                "device_id": item["device_id"],
                "timestamp": item["timestamp"],
                "action": item["action"],
                "result": "EXECUTED",
            }
        ).execute()
    except Exception as exc:
        print(f"Supabase action insert failed: {exc}")


def insert_detection(record: dict[str, Any]) -> dict[str, Any]:
    normalized = normalize_record(record)
    client = get_supabase()
    if client:
        try:
            response = client.table(SUPABASE_TABLE).insert(db_payload_from_record(normalized)).execute()
            if response.data:
                inserted = normalize_record(response.data[0])
                insert_action_record(inserted)
                return inserted
        except Exception as exc:
            print(f"Supabase insert failed, fallback to memory: {exc}")

    MEMORY_RECORDS.insert(0, normalized)
    return normalized


def seed_memory_data() -> None:
    if MEMORY_RECORDS:
        return
    for index in range(24):
        record = mock_yolo_detect()
        record["timestamp"] = (now_dt() - timedelta(hours=index * 3 + random.randint(0, 2))).isoformat(timespec="seconds")
        record["device_id"] = random.choice(DEVICE_IDS[:3])
        MEMORY_RECORDS.append(normalize_record(record))
    MEMORY_RECORDS.sort(key=lambda item: item["timestamp"], reverse=True)


def get_history(limit: int = 200) -> list[dict[str, Any]]:
    client = get_supabase()
    if client:
        try:
            response = (
                client.table(SUPABASE_TABLE)
                .select("*")
                .order("timestamp", desc=True)
                .limit(limit)
                .execute()
            )
            return [normalize_record(item) for item in response.data]
        except Exception as exc:
            print(f"Supabase query failed, fallback to memory: {exc}")
    seed_memory_data()
    return MEMORY_RECORDS[:limit]


def filter_records(
    records: list[dict[str, Any]],
    *,
    hours: int | None = None,
    device_id: str | None = None,
    risk_level: str | None = None,
    source: str | None = None,
) -> list[dict[str, Any]]:
    current = now_dt()
    filtered = []
    for record in records:
        timestamp = parse_timestamp(record.get("timestamp") or record.get("created_at"))
        if hours is not None and (not timestamp or current - timestamp > timedelta(hours=hours)):
            continue
        if device_id and record.get("device_id") != device_id:
            continue
        if risk_level and record.get("risk_level") != risk_level:
            continue
        if source and source not in str(record.get("inference_source") or ""):
            continue
        filtered.append(record)
    return filtered


def get_record_by_id(record_id: str) -> dict[str, Any] | None:
    for record in get_history(limit=1000):
        if str(record.get("id")) == str(record_id):
            return record
    return None


def get_devices() -> list[dict[str, Any]]:
    records = get_history(limit=500)
    latest_by_device: dict[str, dict[str, Any]] = {}
    for record in records:
        device_id = str(record.get("device_id") or "")
        if device_id and device_id not in latest_by_device:
            latest_by_device[device_id] = record

    devices = []
    now = now_dt()
    for device_id in sorted(set(DEVICE_IDS[:3]) | set(latest_by_device)):
        latest = latest_by_device.get(device_id, {})
        timestamp = parse_timestamp(latest.get("timestamp") or latest.get("created_at"))
        age = now - timestamp if timestamp else None
        if age is None:
            status = "UNKNOWN"
        elif age <= timedelta(minutes=2):
            status = "ONLINE"
        elif age <= timedelta(minutes=10):
            status = "RECENT"
        else:
            status = "OFFLINE"
        devices.append(
            {
                "device_id": device_id,
                "area_name": AREA_MAP.get(device_id, "未分配区域"),
                "region_name": AREA_MAP.get(device_id, "未分配区域"),
                "crop_type": {"Node01": "番茄", "Node02": "甜椒", "Node03": "马铃薯"}.get(device_id, "演示作物"),
                "status": status,
                "last_upload_time": latest.get("timestamp") or latest.get("created_at") or "",
                "latest_disease": latest.get("disease") or "--",
                "latest_disease_cn": latest.get("disease_cn") or latest.get("chinese_name") or "--",
                "latest_risk_level": latest.get("risk_level") or "UNKNOWN",
                "latest_action": f"{latest.get('pump_action') or '--'} / {latest.get('fan_action') or '--'}",
                "system_status": latest.get("system_status") or status,
                "sensor_status": latest.get("sensor_status") or "UNKNOWN",
            }
        )
    return devices


def get_alerts(limit: int = 50) -> list[dict[str, Any]]:
    alerts: list[dict[str, Any]] = []
    for record in get_history(limit=500):
        alarm = record.get("alarm") or "NONE"
        risk = record.get("risk_level") or "UNKNOWN"
        confidence = float(record.get("confidence") or 0)
        current_ma = record.get("current_ma")
        alert_types: list[tuple[str, str]] = []
        if risk in {"HIGH", "CRITICAL", "MEDIUM_HIGH"}:
            alert_types.append(("HIGH_RISK", "高风险病害，请人工复核并按处方处理。"))
        if record.get("liquid_level") == "LOW":
            alert_types.append(("LOW_LIQUID", "药液液位不足，禁止自动喷药并补液。"))
        if alarm != "NONE":
            alert_types.append((str(alarm), "根据告警类型检查传感器、执行机构和设备状态。"))
        if record.get("sensor_status") not in {None, "", "OK", "NORMAL", "HEALTHY"}:
            alert_types.append(("SENSOR_STATUS", "传感器状态异常或未就绪，请检查采集链路。"))
        if record.get("system_status") in {"ERROR", "FAULT", "OFFLINE"}:
            alert_types.append(("SYSTEM_STATUS", "系统状态异常，请检查主控、4G 上报和供电状态。"))
        if confidence < 0.6:
            alert_types.append(("LOW_CONFIDENCE", "置信度偏低，重新采集或人工复核。"))
        if isinstance(current_ma, (int, float)) and current_ma > 1200:
            alert_types.append(("CURRENT_ABNORMAL", "电流异常，检查泵、风扇、继电器和供电回路。"))
        for alert_type, action in alert_types:
            alerts.append(
                {
                    "time": record.get("timestamp") or record.get("created_at"),
                    "device_id": record.get("device_id"),
                    "alert_type": alert_type,
                    "risk_level": risk,
                    "disease_cn": record.get("disease_cn") or record.get("chinese_name"),
                    "suggested_action": action,
                    "record_id": record.get("id"),
                }
            )

    now = now_dt()
    for device in get_devices():
        timestamp = parse_timestamp(device.get("last_upload_time"))
        if device["status"] in {"OFFLINE", "UNKNOWN"}:
            alerts.append(
                {
                    "time": device.get("last_upload_time") or now.isoformat(timespec="seconds"),
                    "device_id": device["device_id"],
                    "alert_type": "DEVICE_OFFLINE",
                    "risk_level": "UNKNOWN",
                    "disease_cn": device.get("latest_disease_cn") or "--",
                    "suggested_action": "检查 4G 模组、SIM 卡、PDP 激活和 Edge Function 访问链路。",
                    "record_id": None,
                }
            )
    return alerts[:limit]


def get_heatmap() -> list[dict[str, Any]]:
    devices = get_devices()
    records = get_history(limit=1000)
    current = now_dt()
    result = []
    for device in devices:
        device_id = device["device_id"]
        node_records = [item for item in records if item.get("device_id") == device_id]
        recent = [
            item
            for item in node_records
            if (timestamp := parse_timestamp(item.get("timestamp") or item.get("created_at")))
            and current - timedelta(hours=24) <= timestamp <= current
        ]
        latest = node_records[0] if node_records else {}
        highest = max((item.get("risk_level") or "UNKNOWN" for item in recent), key=lambda risk: RISK_RANK.get(risk, 0), default="UNKNOWN")
        result.append(
            {
                **device,
                "area_risk_level": highest if recent else device["latest_risk_level"],
                "recent_24h_max_risk_level": highest,
                "recent_24h_alert_count": sum(1 for item in recent if item.get("risk_level") in ALERT_RISKS),
                "latest_disease_cn": latest.get("disease_cn") or latest.get("chinese_name") or "--",
                "latest_time": latest.get("timestamp") or latest.get("created_at") or "",
            }
        )
    return result


def get_heatmap_matrix() -> dict[str, Any]:
    records = get_history(limit=2000)
    current = now_dt()
    start_hour = current.replace(minute=0, second=0, microsecond=0) - timedelta(hours=23)
    buckets = [start_hour + timedelta(hours=index) for index in range(24)]
    hour_labels = [bucket.strftime("%H:00") for bucket in buckets]
    bucket_keys = [bucket.strftime("%Y-%m-%d %H:00") for bucket in buckets]
    device_ids = [device["device_id"] for device in get_devices()]

    matrix = {
        device_id: {key: {"hour": label, "risk_score": 0, "record_count": 0} for key, label in zip(bucket_keys, hour_labels, strict=True)}
        for device_id in device_ids
    }
    for record in records:
        device_id = str(record.get("device_id") or "")
        timestamp = parse_timestamp(record.get("timestamp") or record.get("created_at"))
        if device_id not in matrix or not timestamp or timestamp < start_hour or timestamp > current:
            continue
        key = timestamp.replace(minute=0, second=0, microsecond=0).strftime("%Y-%m-%d %H:00")
        if key in matrix[device_id]:
            matrix[device_id][key]["risk_score"] += RISK_SCORE.get(record.get("risk_level") or "UNKNOWN", 0)
            matrix[device_id][key]["record_count"] += 1

    return {
        "hours": hour_labels,
        "nodes": [
            {
                "device_id": device_id,
                "area_name": AREA_MAP.get(device_id, "未分配区域"),
                "values": list(matrix[device_id].values()),
            }
            for device_id in device_ids
        ],
    }


def get_stats() -> dict[str, Any]:
    records = get_history(limit=1000)
    total = len(records)
    disease_counter = Counter(item.get("disease") or "UNKNOWN" for item in records)
    risk_counter = Counter(item.get("risk_level") or "UNKNOWN" for item in records)
    daily = defaultdict(int)
    current = now_dt()
    recent_24h_alerts = 0
    healthy_count = 0

    for item in records:
        disease = str(item.get("disease") or "")
        if "healthy" in disease.lower():
            healthy_count += 1
        timestamp = parse_timestamp(item.get("timestamp") or item.get("created_at"))
        if not timestamp:
            continue
        daily[timestamp.date().isoformat()] += 1
        if current - timedelta(hours=24) <= timestamp <= current and item.get("risk_level") in ALERT_RISKS:
            recent_24h_alerts += 1

    online_count = sum(1 for device in get_devices() if device["status"] == "ONLINE")
    return {
        "summary": {
            "total_detections": total,
            "recent_24h_alerts": recent_24h_alerts,
            "today_alerts": recent_24h_alerts,
            "online_devices": online_count,
            "healthy_ratio": round(healthy_count / total * 100, 1) if total else 0,
            "supabase_connected": supabase_connected(),
            "model_ready": model_status()["exists"],
        },
        "disease_distribution": [{"name": name, "value": count} for name, count in disease_counter.most_common()],
        "risk_distribution": [{"name": name, "value": count} for name, count in risk_counter.items()],
        "daily_trend": [{"date": key, "count": daily[key]} for key in sorted(daily)],
    }
