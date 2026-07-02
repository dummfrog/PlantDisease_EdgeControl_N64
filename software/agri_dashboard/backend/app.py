from __future__ import annotations

import os
import sys
from io import StringIO
from pathlib import Path
from uuid import uuid4

from flask import Flask, Response, jsonify, request, send_from_directory

sys.path.insert(0, str(Path(__file__).resolve().parent))
from database import (  # noqa: E402
    CLASS_NAMES,
    DEVICE_IDS,
    EXPERT_KNOWLEDGE,
    filter_records,
    get_alerts,
    get_devices,
    get_heatmap,
    get_heatmap_matrix,
    get_history,
    get_record_by_id,
    get_stats,
    infer_image,
    insert_detection,
    mock_yolo_detect,
    model_status,
    now_iso,
    supabase_connected,
)


BASE_DIR = Path(__file__).resolve().parent.parent
UPLOAD_DIR = BASE_DIR / "uploads"
FRONTEND_DIR = BASE_DIR / "frontend"
ALLOWED_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
UPLOAD_DIR.mkdir(exist_ok=True)

app = Flask(__name__, static_folder=None)


@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    response.headers["Access-Control-Allow-Methods"] = "GET,POST,OPTIONS"
    return response


@app.route("/", methods=["GET"])
def index():
    return send_from_directory(FRONTEND_DIR, "index.html")


@app.route("/uploads/<path:filename>", methods=["GET"])
def uploaded_file(filename):
    return send_from_directory(UPLOAD_DIR, filename)


@app.route("/api/health", methods=["GET"])
def health():
    return jsonify(
        {
            "ok": True,
            "status": "ok",
            "supabase_connected": supabase_connected(),
            "model": model_status(),
            "class_count": len(CLASS_NAMES),
            "knowledge_items": len(EXPERT_KNOWLEDGE),
            "config": {
                "dashboard_port": int(os.getenv("DASHBOARD_PORT", "8001")),
                "supabase_url_configured": bool(os.getenv("SUPABASE_URL")),
                "supabase_key_configured": bool(
                    os.getenv("SUPABASE_SERVICE_ROLE_KEY")
                    or os.getenv("SUPABASE_KEY")
                    or os.getenv("SUPABASE_ANON_KEY")
                ),
                "service_role_visible_to_frontend": False,
            },
        }
    )


def save_upload_file():
    file = request.files.get("file")
    if not file:
        return None, None, (jsonify({"error": "file is required"}), 400)

    ext = Path(file.filename or "leaf.jpg").suffix.lower() or ".jpg"
    if ext not in ALLOWED_EXTS:
        return None, None, (jsonify({"error": f"unsupported file type: {ext}"}), 400)

    saved_name = f"{uuid4()}{ext}"
    saved_path = UPLOAD_DIR / saved_name
    file.save(saved_path)
    return saved_name, saved_path, None


def run_closed_loop(saved_name: str, saved_path: Path, write_to_db: bool = True):
    device_id = request.form.get("device_id") or request.args.get("device_id") or "STM32N647-UPLOAD"
    prediction = infer_image(saved_path)
    record_data = {
        **prediction,
        "timestamp": now_iso(),
        "device_id": device_id,
        "image_name": saved_name,
        "image_url": f"/uploads/{saved_name}",
    }
    record = insert_detection(record_data) if write_to_db else record_data
    return record


@app.route("/api/upload", methods=["POST", "OPTIONS"])
def upload():
    if request.method == "OPTIONS":
        return ("", 204)
    saved_name, saved_path, error = save_upload_file()
    if error:
        return error
    record = run_closed_loop(saved_name, saved_path, write_to_db=True)
    return jsonify({"image_url": f"/uploads/{saved_name}", "record": record, "result": record})


@app.route("/api/predict", methods=["POST", "OPTIONS"])
def predict():
    if request.method == "OPTIONS":
        return ("", 204)
    saved_name, saved_path, error = save_upload_file()
    if error:
        return error
    record = run_closed_loop(saved_name, saved_path, write_to_db=False)
    return jsonify({"image_url": f"/uploads/{saved_name}", "prediction": record})


@app.route("/api/history", methods=["GET"])
def history():
    limit = request.args.get("limit", default=200, type=int)
    limit = max(1, min(limit, 1000))
    hours = request.args.get("hours", type=int)
    device_id = request.args.get("device_id") or None
    risk_level = request.args.get("risk_level") or None
    source = request.args.get("source") or None
    records = filter_records(
        get_history(limit=limit),
        hours=hours,
        device_id=device_id,
        risk_level=risk_level,
        source=source,
    )
    return jsonify({"ok": True, "records": records})


@app.route("/api/stats", methods=["GET"])
def stats():
    return jsonify(get_stats())


@app.route("/api/devices", methods=["GET"])
def devices():
    return jsonify({"ok": True, "devices": get_devices()})


@app.route("/api/node_status", methods=["GET"])
def node_status():
    return jsonify({"ok": True, "nodes": get_devices(), "devices": get_devices()})


@app.route("/api/alerts", methods=["GET"])
def alerts():
    limit = request.args.get("limit", default=50, type=int)
    return jsonify({"ok": True, "alerts": get_alerts(limit=max(1, min(limit, 200)))})


@app.route("/api/heatmap", methods=["GET"])
def heatmap():
    return jsonify(get_heatmap())


@app.route("/api/heatmap_matrix", methods=["GET"])
def heatmap_matrix():
    return jsonify({"ok": True, **get_heatmap_matrix()})


@app.route("/api/record/<record_id>", methods=["GET"])
def record_detail(record_id: str):
    record = get_record_by_id(record_id)
    if not record:
        return jsonify({"ok": False, "error": "record not found"}), 404
    return jsonify({"ok": True, "record": record})


@app.route("/api/model/status", methods=["GET"])
def model_status_api():
    return jsonify({"ok": True, "model": model_status()})


@app.route("/api/export_csv", methods=["GET"])
def export_csv():
    records = filter_records(
        get_history(limit=1000),
        hours=request.args.get("hours", type=int),
        device_id=request.args.get("device_id") or None,
        risk_level=request.args.get("risk_level") or None,
        source=request.args.get("source") or None,
    )
    fields = [
        "id",
        "timestamp",
        "device_id",
        "disease_id",
        "disease",
        "disease_cn",
        "confidence",
        "risk_level",
        "temperature_c",
        "humidity_percent",
        "light_lux",
        "soil_moisture_percent",
        "liquid_level",
        "pump_action",
        "fan_action",
        "pump_duration_s",
        "fan_duration_s",
        "current_ma",
        "system_status",
        "sensor_status",
        "alarm",
        "pesticide",
        "suggestion",
        "inference_source",
        "model_name",
    ]
    output = StringIO()
    output.write(",".join(fields) + "\n")
    for record in records:
        values = []
        for field in fields:
            value = str(record.get(field, "")).replace('"', '""')
            values.append(f'"{value}"')
        output.write(",".join(values) + "\n")
    filename = f"diagnosis_records_{now_iso().replace(':', '').replace('+', '_')}.csv"
    return Response(
        output.getvalue(),
        mimetype="text/csv; charset=utf-8",
        headers={"Content-Disposition": f"attachment; filename={filename}"},
    )


@app.route("/api/simulate_device", methods=["POST", "OPTIONS"])
def simulate_device():
    if request.method == "OPTIONS":
        return ("", 204)

    payload = request.get_json(silent=True) or {}
    disease = payload.get("disease")
    result = mock_yolo_detect(disease=disease)
    record = insert_detection(
        {
            **result,
            "timestamp": payload.get("timestamp") or now_iso(),
            "device_id": payload.get("device_id") or DEVICE_IDS[0],
            "inference_source": payload.get("inference_source") or "stm32_npu_simulated",
            "model_name": payload.get("model_name") or "STM32N647-NPU-demo",
            "image_name": payload.get("image_name") or "simulated_leaf.jpg",
        }
    )
    return jsonify({"ok": True, "message": "STM32 device upload simulated", "record": record})


if __name__ == "__main__":
    port = int(os.getenv("DASHBOARD_PORT", "8001"))
    app.run(host="127.0.0.1", port=port, debug=False)
