from __future__ import annotations

import json
import time
from pathlib import Path
from typing import Any

import numpy as np
import onnx
import onnxruntime as ort
import torch

from dms_backend.training.model_factory import load_checkpoint_model
from dms_backend.training.trainer import resolve_device


def load_stage3_config(path: Path) -> dict[str, Any]:
    import yaml

    with path.open("r", encoding="utf-8") as file:
        raw = yaml.safe_load(file)
    if not isinstance(raw, dict):
        raise ValueError(f"Stage 3 配置不是有效对象: {path}")
    return raw


def _export_with_compatibility(
    model: torch.nn.Module,
    dummy_input: torch.Tensor,
    output_path: Path,
    *,
    opset_version: int,
    dynamic_batch: bool,
) -> None:
    dynamic_axes = None
    if dynamic_batch:
        dynamic_axes = {"images": {0: "batch"}, "logits": {0: "batch"}}

    kwargs = {
        "export_params": True,
        "opset_version": opset_version,
        "do_constant_folding": True,
        "input_names": ["images"],
        "output_names": ["logits"],
        "dynamic_axes": dynamic_axes,
    }
    try:
        torch.onnx.export(model, dummy_input, output_path, dynamo=False, **kwargs)
    except TypeError:
        torch.onnx.export(model, dummy_input, output_path, **kwargs)


def export_task_to_onnx(
    *,
    stage3_config_path: Path,
    task_name: str,
    checkpoint_path: Path | None = None,
) -> dict[str, Any]:
    config = load_stage3_config(stage3_config_path)
    checkpoint_root = Path(config["checkpoint_root"])
    checkpoint_path = checkpoint_path or checkpoint_root / task_name / "best.pt"
    output_dir = Path(config["onnx_root"])
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / f"{task_name}_mobilenetv2.onnx"

    # ONNX export is performed on CPU for maximum portability, even if training used CUDA.
    device = torch.device("cpu")
    model, checkpoint = load_checkpoint_model(checkpoint_path, device)
    model.eval()
    image_size = int(checkpoint["config"]["model"]["image_size"])
    dummy_input = torch.randn(1, 3, image_size, image_size, device=device)

    onnx_config = config["onnx"]
    _export_with_compatibility(
        model,
        dummy_input,
        output_path,
        opset_version=int(onnx_config.get("opset_version", 17)),
        dynamic_batch=bool(onnx_config.get("dynamic_batch", True)),
    )

    onnx_model = onnx.load(str(output_path))
    onnx.checker.check_model(onnx_model)

    session = ort.InferenceSession(str(output_path), providers=["CPUExecutionProvider"])
    numpy_input = dummy_input.detach().cpu().numpy().astype(np.float32)
    with torch.inference_mode():
        torch_output = model(dummy_input).detach().cpu().numpy()
    ort_output = session.run(["logits"], {"images": numpy_input})[0]

    max_abs_error = float(np.max(np.abs(torch_output - ort_output)))
    mean_abs_error = float(np.mean(np.abs(torch_output - ort_output)))
    np.testing.assert_allclose(
        torch_output,
        ort_output,
        atol=float(onnx_config.get("validate_atol", 1e-4)),
        rtol=float(onnx_config.get("validate_rtol", 1e-3)),
    )

    metadata = {
        "task": task_name,
        "source_checkpoint": str(checkpoint_path.resolve()),
        "onnx_path": str(output_path.resolve()),
        "opset_version": int(onnx_config.get("opset_version", 17)),
        "dynamic_batch": bool(onnx_config.get("dynamic_batch", True)),
        "input_name": "images",
        "input_shape": ["batch", 3, image_size, image_size],
        "output_name": "logits",
        "class_names": checkpoint["config"]["task"]["classes"],
        "preprocess": checkpoint.get("preprocess", {}),
        "max_abs_error": max_abs_error,
        "mean_abs_error": mean_abs_error,
        "file_size_bytes": output_path.stat().st_size,
    }
    with (output_dir / f"{task_name}_metadata.json").open("w", encoding="utf-8") as file:
        json.dump(metadata, file, ensure_ascii=False, indent=2)

    print(f"\n=== ONNX export: {task_name} ===")
    print(f"Output: {output_path}")
    print(f"Size: {output_path.stat().st_size / 1024 / 1024:.2f} MB")
    print(f"Max abs error: {max_abs_error:.8f}")
    print("ONNX Runtime consistency check: PASS")
    return metadata


def benchmark_onnx_model(
    *,
    stage3_config_path: Path,
    task_name: str,
) -> dict[str, Any]:
    config = load_stage3_config(stage3_config_path)
    onnx_path = Path(config["onnx_root"]) / f"{task_name}_mobilenetv2.onnx"
    metadata_path = Path(config["onnx_root"]) / f"{task_name}_metadata.json"
    if not onnx_path.is_file():
        raise FileNotFoundError(f"找不到 ONNX 文件: {onnx_path}\n请先运行 09_export_onnx.py")

    with metadata_path.open("r", encoding="utf-8") as file:
        metadata = json.load(file)
    image_size = int(metadata["input_shape"][2])
    sample = np.random.randn(1, 3, image_size, image_size).astype(np.float32)
    session = ort.InferenceSession(str(onnx_path), providers=["CPUExecutionProvider"])

    benchmark_config = config.get("benchmark", {})
    warmup_runs = int(benchmark_config.get("warmup_runs", 10))
    measured_runs = int(benchmark_config.get("measured_runs", 100))
    for _ in range(warmup_runs):
        session.run(["logits"], {"images": sample})

    timings_ms: list[float] = []
    for _ in range(measured_runs):
        started = time.perf_counter()
        session.run(["logits"], {"images": sample})
        timings_ms.append((time.perf_counter() - started) * 1000.0)

    result = {
        "task": task_name,
        "provider": session.get_providers(),
        "runs": measured_runs,
        "mean_ms": float(np.mean(timings_ms)),
        "median_ms": float(np.median(timings_ms)),
        "p95_ms": float(np.percentile(timings_ms, 95)),
        "min_ms": float(np.min(timings_ms)),
        "max_ms": float(np.max(timings_ms)),
    }
    output_path = Path(config["onnx_root"]) / f"{task_name}_benchmark.json"
    with output_path.open("w", encoding="utf-8") as file:
        json.dump(result, file, ensure_ascii=False, indent=2)

    print(f"\n=== ONNX CPU benchmark: {task_name} ===")
    print(f"Mean: {result['mean_ms']:.3f} ms")
    print(f"Median: {result['median_ms']:.3f} ms")
    print(f"P95: {result['p95_ms']:.3f} ms")
    return result
