from __future__ import annotations

import csv
import json
from pathlib import Path
from typing import Any

import numpy as np
import onnxruntime as ort
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
MODEL_PATH = ROOT / "models" / "best.onnx"
LABELS_PATH = ROOT / "models" / "labels.txt"
LABELS_CN_PATH = ROOT / "models" / "labels_cn.json"
TEST_IMAGES_DIR = ROOT / "test_images"
CSV_PATH = ROOT / "expected_results" / "expected_results.csv"
REPORT_PATH = ROOT / "reports" / "onnxruntime_test_report.md"
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}


def load_labels() -> list[str]:
    labels: list[str] = []
    for line in LABELS_PATH.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        _, label = line.split(maxsplit=1)
        labels.append(label.strip())
    return labels


def resolve_input_layout(shape: list[Any]) -> tuple[int, int, str]:
    dims = [dim if isinstance(dim, int) else None for dim in shape]
    if len(dims) != 4:
        raise ValueError(f"Expected 4D model input, got shape={shape}")
    if dims[1] == 3:
        height = dims[2] or 224
        width = dims[3] or height
        return height, width, "NCHW"
    if dims[3] == 3:
        height = dims[1] or 224
        width = dims[2] or height
        return height, width, "NHWC"
    if dims[1] in (1, None) and dims[3] not in (3,):
        height = dims[2] or 224
        width = dims[3] or height
        return height, width, "NCHW"
    height = dims[2] or 224
    width = dims[3] or height
    return height, width, "NCHW"


def preprocess(path: Path, height: int, width: int, layout: str) -> np.ndarray:
    image = Image.open(path).convert("RGB").resize((width, height))
    arr = np.asarray(image, dtype=np.float32) / 255.0
    if layout == "NCHW":
        arr = np.transpose(arr, (2, 0, 1))
    return np.expand_dims(arr, axis=0).astype(np.float32)


def flatten_output(output: Any) -> np.ndarray:
    arr = np.asarray(output, dtype=np.float32)
    if arr.ndim == 0:
        return arr.reshape(1)
    if arr.ndim == 4 and arr.shape[1] == 8:
        arr = arr.mean(axis=(2, 3))
    elif arr.ndim == 4 and arr.shape[-1] == 8:
        arr = arr.mean(axis=(1, 2))
    elif arr.ndim == 3 and arr.shape[1] == 8:
        arr = arr.mean(axis=2)
    elif arr.ndim == 3 and arr.shape[-1] == 8:
        arr = arr.mean(axis=1)
    arr = np.squeeze(arr)
    if arr.ndim > 1:
        arr = arr.reshape(-1)
    return arr


def softmax_if_needed(values: np.ndarray) -> tuple[np.ndarray, bool]:
    values = values.astype(np.float64)
    if values.size == 0:
        return values, False
    if np.all(values >= 0) and np.all(values <= 1.0) and abs(float(values.sum()) - 1.0) < 1e-3:
        return values.astype(np.float32), False
    stable = values - np.max(values)
    exp = np.exp(stable)
    probs = exp / exp.sum()
    return probs.astype(np.float32), True


def main() -> None:
    if not MODEL_PATH.exists():
        raise FileNotFoundError(f"Missing model: {MODEL_PATH}")
    labels = load_labels()
    labels_cn = json.loads(LABELS_CN_PATH.read_text(encoding="utf-8"))
    images = sorted(path for path in TEST_IMAGES_DIR.iterdir() if path.suffix.lower() in IMAGE_EXTS)
    if not images:
        raise FileNotFoundError(f"No test images found in {TEST_IMAGES_DIR}")

    ROOT.joinpath("expected_results").mkdir(exist_ok=True)
    ROOT.joinpath("reports").mkdir(exist_ok=True)
    session = ort.InferenceSession(str(MODEL_PATH), providers=["CPUExecutionProvider"])
    input_meta = session.get_inputs()[0]
    output_meta = session.get_outputs()
    height, width, layout = resolve_input_layout(list(input_meta.shape))

    rows: list[dict[str, Any]] = []
    softmax_used_any = False
    for image_path in images:
        batch = preprocess(image_path, height, width, layout)
        outputs = session.run(None, {input_meta.name: batch})
        scores = flatten_output(outputs[0])
        if scores.size != len(labels):
            row = {
                "image": image_path.name,
                "status": "shape_check_required",
                "top1_id": "",
                "top1_label": "",
                "top1_label_cn": "",
                "top1_confidence": "",
                "top3": "",
                "raw_output_shape": str(np.asarray(outputs[0]).shape),
            }
            rows.append(row)
            continue
        probs, softmax_used = softmax_if_needed(scores)
        softmax_used_any = softmax_used_any or softmax_used
        top_indices = np.argsort(probs)[::-1][:3]
        top3 = [
            f"{int(idx)}:{labels[int(idx)]}:{float(probs[int(idx)]):.6f}"
            for idx in top_indices
        ]
        top1 = int(top_indices[0])
        rows.append(
            {
                "image": image_path.name,
                "status": "ok",
                "top1_id": top1,
                "top1_label": labels[top1],
                "top1_label_cn": labels_cn.get(labels[top1], ""),
                "top1_confidence": f"{float(probs[top1]):.6f}",
                "top3": " | ".join(top3),
                "raw_output_shape": str(np.asarray(outputs[0]).shape),
            }
        )

    with CSV_PATH.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "image",
                "status",
                "top1_id",
                "top1_label",
                "top1_label_cn",
                "top1_confidence",
                "top3",
                "raw_output_shape",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    lines = [
        "# ONNXRuntime Test Report",
        "",
        f"- Model: `{MODEL_PATH}`",
        f"- Provider: CPUExecutionProvider",
        f"- Input name: `{input_meta.name}`",
        f"- Input shape: `{input_meta.shape}`",
        f"- Input dtype: `{input_meta.type}`",
        f"- Resolved preprocessing: RGB, resize {width}x{height}, float32, 0..1 normalization, {layout}",
        f"- Outputs: {[(out.name, out.shape, out.type) for out in output_meta]}",
        f"- Test images: {len(images)}",
        f"- Softmax applied by script: {'yes' if softmax_used_any else 'no'}",
        f"- CSV: `{CSV_PATH}`",
        "",
        "## Results",
        "",
        "| image | status | top-1 | confidence | top-3 |",
        "| --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        top1 = f"{row['top1_id']} {row['top1_label']}" if row["status"] == "ok" else ""
        lines.append(
            f"| {row['image']} | {row['status']} | {top1} | {row['top1_confidence']} | {row['top3']} |"
        )
    lines.extend(
        [
            "",
            "## Risk Notes",
            "",
            "- These are host-side ONNXRuntime smoke-test results, not STM32N6 validation results.",
            "- If top-1 labels do not match the filename class, verify training/export preprocessing and whether the ONNX model is detection-style rather than pure classification.",
            "- Hardware firmware must preserve the exact labels order from `labels.txt`.",
        ]
    )
    REPORT_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {CSV_PATH}")
    print(f"Wrote {REPORT_PATH}")


if __name__ == "__main__":
    main()
