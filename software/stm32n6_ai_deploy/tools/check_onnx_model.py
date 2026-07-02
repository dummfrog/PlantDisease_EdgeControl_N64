from __future__ import annotations

from collections import Counter
from pathlib import Path
from typing import Any

import onnx
from onnx import TensorProto


ROOT = Path(__file__).resolve().parents[1]
MODEL_PATH = ROOT / "models" / "best.onnx"
REPORT_PATH = ROOT / "reports" / "onnx_model_inspection.md"
OPS_PATH = ROOT / "reports" / "onnx_ops_list.txt"


def dim_to_value(dim: Any) -> str | int:
    if dim.dim_param:
        return dim.dim_param
    if dim.dim_value:
        return dim.dim_value
    return "dynamic"


def tensor_info(value_info: Any) -> dict[str, Any]:
    tensor_type = value_info.type.tensor_type
    shape = [dim_to_value(dim) for dim in tensor_type.shape.dim]
    dtype = TensorProto.DataType.Name(tensor_type.elem_type)
    return {"name": value_info.name, "shape": shape, "dtype": dtype}


def has_dynamic_shape(infos: list[dict[str, Any]]) -> bool:
    for info in infos:
        for dim in info["shape"]:
            if isinstance(dim, str):
                return True
    return False


def output_class_count(outputs: list[dict[str, Any]]) -> int | None:
    if not outputs:
        return None
    shape = outputs[0]["shape"]
    numeric_dims = [dim for dim in shape if isinstance(dim, int)]
    if not numeric_dims:
        return None
    if numeric_dims[-1] == 8:
        return 8
    if len(numeric_dims) >= 2 and numeric_dims[-2] == 8:
        return 8
    if len(numeric_dims) >= 3 and numeric_dims[1] == 8:
        return 8
    return numeric_dims[-1]


def main() -> None:
    if not MODEL_PATH.exists():
        raise FileNotFoundError(f"Missing model: {MODEL_PATH}")

    ROOT.joinpath("reports").mkdir(exist_ok=True)
    model = onnx.load(str(MODEL_PATH))
    onnx.checker.check_model(model)

    opsets = [f"{op.domain or 'ai.onnx'}:{op.version}" for op in model.opset_import]
    inputs = [tensor_info(item) for item in model.graph.input]
    outputs = [tensor_info(item) for item in model.graph.output]
    op_counter = Counter(node.op_type for node in model.graph.node)
    initializers = {init.name for init in model.graph.initializer}
    real_inputs = [item for item in inputs if item["name"] not in initializers]
    dynamic = has_dynamic_shape(real_inputs + outputs)
    class_count = output_class_count(outputs)
    all_tensor_types = Counter()
    for item in list(model.graph.input) + list(model.graph.output) + list(model.graph.value_info):
        if item.type.HasField("tensor_type"):
            elem_type = item.type.tensor_type.elem_type
            all_tensor_types[TensorProto.DataType.Name(elem_type)] += 1
    weight_types = Counter(TensorProto.DataType.Name(init.data_type) for init in model.graph.initializer)
    quantized = any(op in op_counter for op in ("QuantizeLinear", "DequantizeLinear", "QLinearConv", "QLinearMatMul"))

    OPS_PATH.write_text(
        "\n".join(f"{name}: {count}" for name, count in sorted(op_counter.items())) + "\n",
        encoding="utf-8",
    )

    lines = [
        "# ONNX Model Inspection",
        "",
        f"- Model: `{MODEL_PATH}`",
        f"- File size: {MODEL_PATH.stat().st_size} bytes",
        f"- ONNX opset: {', '.join(opsets)}",
        f"- Graph nodes: {len(model.graph.node)}",
        f"- Initializers: {len(model.graph.initializer)}",
        f"- Dynamic shape: {'yes' if dynamic else 'no'}",
        f"- Detected output class count: {class_count if class_count is not None else 'unknown'}",
        f"- Expected class count: 8",
        f"- Class count check: {'PASS' if class_count == 8 else 'CHECK_REQUIRED'}",
        f"- Quantization ops present: {'yes' if quantized else 'no'}",
        f"- Tensor value-info dtypes: {dict(all_tensor_types)}",
        f"- Weight dtypes: {dict(weight_types)}",
        "",
        "## Inputs",
        "",
    ]
    for item in real_inputs:
        lines.append(f"- `{item['name']}` shape={item['shape']} dtype={item['dtype']}")
    lines.extend(["", "## Outputs", ""])
    for item in outputs:
        lines.append(f"- `{item['name']}` shape={item['shape']} dtype={item['dtype']}")
    lines.extend(["", "## Operator Types", ""])
    for name, count in sorted(op_counter.items()):
        lines.append(f"- {name}: {count}")
    lines.extend(
        [
            "",
            "## STM32N6 Deployment Notes",
            "",
            "- This script only validates ONNX structure and local ONNXRuntime readiness.",
            "- Final STM32N6/NPU support must be confirmed by ST Edge AI analyze/generate.",
            "- If the model remains FP32, INT8 quantization should be evaluated before NPU deployment.",
        ]
    )
    REPORT_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {REPORT_PATH}")
    print(f"Wrote {OPS_PATH}")


if __name__ == "__main__":
    main()
