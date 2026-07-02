from __future__ import annotations

import argparse
import csv
import json
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}


def upload_image(api_url: str, image_path: Path) -> dict:
    boundary = "----codex-field-test-boundary"
    file_bytes = image_path.read_bytes()
    body = (
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="file"; filename="{image_path.name}"\r\n'
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode("utf-8") + file_bytes + f"\r\n--{boundary}--\r\n".encode("utf-8")
    request = Request(
        api_url,
        data=body,
        method="POST",
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
    )
    with urlopen(request, timeout=60) as response:
        return json.loads(response.read().decode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description="Log field image inference results to CSV.")
    parser.add_argument("image_dir", help="Directory containing test images.")
    parser.add_argument("--api-url", default="http://127.0.0.1:8001/api/upload")
    parser.add_argument("--output", default="runs/field_tests/model_field_test_results.csv")
    parser.add_argument("--expected-class", default="")
    args = parser.parse_args()

    image_dir = Path(args.image_dir)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    images = [path for path in sorted(image_dir.iterdir()) if path.suffix.lower() in IMAGE_EXTS]

    rows = []
    for image in images:
        started = time.perf_counter()
        try:
            data = upload_image(args.api_url, image)
            latency_ms = round((time.perf_counter() - started) * 1000, 2)
            record = data.get("record") or data.get("result") or {}
            predicted = record.get("disease") or ""
            expected = args.expected_class
            rows.append(
                {
                    "image_name": image.name,
                    "predicted_id": record.get("disease_id", ""),
                    "predicted_class": predicted,
                    "confidence": record.get("confidence", ""),
                    "latency_ms": latency_ms,
                    "expected_class": expected,
                    "correct": "" if not expected else str(predicted == expected),
                }
            )
        except (HTTPError, URLError, TimeoutError, json.JSONDecodeError) as exc:
            rows.append(
                {
                    "image_name": image.name,
                    "predicted_id": "",
                    "predicted_class": "",
                    "confidence": "",
                    "latency_ms": "",
                    "expected_class": args.expected_class,
                    "correct": "",
                    "error": str(exc),
                }
            )

    fieldnames = ["image_name", "predicted_id", "predicted_class", "confidence", "latency_ms", "expected_class", "correct", "error"]
    with output.open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    report = output.with_suffix(".md")
    report.write_text(
        "# 现场模型测试报告草稿\n\n"
        f"- 图片目录：`{image_dir}`\n"
        f"- API：`{args.api_url}`\n"
        f"- 样本数：{len(rows)}\n"
        f"- CSV：`{output}`\n\n"
        "请补充现场光照、拍摄距离、样本卡类型、误判样例和复测结论。\n",
        encoding="utf-8",
    )
    print(f"Wrote {output}")
    print(f"Wrote {report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
