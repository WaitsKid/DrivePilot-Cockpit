from __future__ import annotations

import sys
import threading
import time
from collections import deque
from pathlib import Path
from typing import Any

from dms_backend.fatigue.config import Stage5Config
from dms_backend.fatigue.engine import FatigueEngine
from dms_backend.fatigue.types import VisualCueSample
from dms_backend.inference.config import load_stage4_config

from .status_store import DmsStatusStore


class DmsBackgroundService:
    """Own the camera and run DMS inference without exposing camera frames."""

    def __init__(self, config: Stage5Config, store: DmsStatusStore | None = None) -> None:
        self.config = config
        self.store = store or DmsStatusStore()
        self._pipeline: Any = None
        self._engine = FatigueEngine(config.fatigue)
        self._thread: threading.Thread | None = None
        self._stop_event = threading.Event()
        self._lifecycle_lock = threading.RLock()
        self._models_ready = False
        self._started_at = 0.0
        self._last_error = ""

    @property
    def models_ready(self) -> bool:
        return self._models_ready

    @property
    def running(self) -> bool:
        return bool(self._thread and self._thread.is_alive())

    def initialize_models(self) -> bool:
        with self._lifecycle_lock:
            if self._models_ready:
                return True
            try:
                # Lazy import keeps API/config unit tests independent from ONNX Runtime.
                from dms_backend.inference.pipeline import DmsInferencePipeline

                self._pipeline = DmsInferencePipeline(self.config.stage4_config_path)
                self._pipeline.smoke_test_classifiers()
                self._models_ready = True
                self._last_error = ""
                self.store.merge(
                    {
                        "models_ready": True,
                        "status_text": "模型加载完成",
                        "monitoring_state": "ready",
                        "last_error": "",
                    }
                )
                return True
            except Exception as error:  # noqa: BLE001 - surfaced through health endpoint
                self._pipeline = None
                self._models_ready = False
                self._last_error = f"模型初始化失败: {error}"
                self.store.merge(
                    {
                        "models_ready": False,
                        "status_text": "模型加载失败",
                        "monitoring_state": "model_error",
                        "last_error": self._last_error,
                    }
                )
                return False

    def start(self) -> bool:
        with self._lifecycle_lock:
            if self.running:
                return True
            if not self._models_ready and not self.initialize_models():
                return False

            self._stop_event.clear()
            self._engine.reset(time.time())
            self._started_at = time.time()
            self._thread = threading.Thread(
                target=self._run,
                name="DriveGuardDmsCameraWorker",
                daemon=True,
            )
            self._thread.start()
            self.store.merge(
                {
                    "service_running": True,
                    "started_at_ms": int(self._started_at * 1000.0),
                    "status_text": "后台疲劳监测正在启动",
                    "monitoring_state": "starting",
                    "last_error": "",
                }
            )
            return True

    def stop(self, join_timeout_seconds: float = 5.0) -> None:
        with self._lifecycle_lock:
            thread = self._thread
            if thread is None:
                self.store.merge(
                    {
                        "service_running": False,
                        "camera_available": False,
                        "status_text": "疲劳监测已停止",
                        "monitoring_state": "stopped",
                    }
                )
                return
            self._stop_event.set()

        if thread is not threading.current_thread():
            thread.join(timeout=max(0.0, join_timeout_seconds))

        with self._lifecycle_lock:
            if self._thread is thread and not thread.is_alive():
                self._thread = None
            self.store.merge(
                {
                    "service_running": False,
                    "camera_available": False,
                    "status_text": "疲劳监测已停止",
                    "monitoring_state": "stopped",
                }
            )

    def reset_state(self) -> dict[str, Any]:
        snapshot = self._engine.reset(time.time())
        payload = self._build_payload(snapshot.to_dict(), camera_available=self._camera_available())
        self.store.publish(payload)
        return payload

    def shutdown(self) -> None:
        self.stop(join_timeout_seconds=5.0)

    def get_status(self) -> dict[str, Any]:
        return self.store.snapshot()[1]

    def get_public_config(self) -> dict[str, Any]:
        fatigue = self.config.fatigue
        return {
            "server": {
                "host": self.config.server.host,
                "port": self.config.server.port,
                "auto_start_monitoring": self.config.server.auto_start_monitoring,
            },
            "monitor": {
                "target_process_fps": self.config.monitor.target_process_fps,
                "mirror_input": self.config.monitor.mirror_input,
            },
            "fatigue": {
                "perclos_window_seconds": fatigue.perclos_window_seconds,
                "yawn_window_seconds": fatigue.yawn_window_seconds,
                "slight_closed_seconds": fatigue.slight_closed_seconds,
                "severe_closed_seconds": fatigue.severe_closed_seconds,
                "slight_perclos": fatigue.slight_perclos,
                "severe_perclos": fatigue.severe_perclos,
                "slight_yawn_count": fatigue.slight_yawn_count,
                "severe_yawn_count": fatigue.severe_yawn_count,
            },
            "privacy": {
                "store_camera_frames": False,
                "expose_camera_frames_over_api": False,
                "statement": "摄像头画面仅在 Python 进程内推理，不保存、不通过 API 返回。",
            },
        }

    def _run(self) -> None:
        try:
            import cv2

            stage4 = load_stage4_config(self.config.stage4_config_path)
            camera_config = stage4.get("camera", {})
            camera_index = int(camera_config.get("index", 0))
            width = int(camera_config.get("width", 1280))
            height = int(camera_config.get("height", 720))
            process_every = max(1, int(camera_config.get("process_every_n_frames", 1)))
            process_every *= max(1, int(self.config.monitor.publish_every_n_frames))

            while not self._stop_event.is_set():
                capture = self._open_camera(cv2, camera_index, width, height)
                if capture is None:
                    if self._stop_event.wait(self.config.monitor.camera_open_retry_seconds):
                        break
                    continue

                try:
                    self._camera_loop(cv2, capture, process_every)
                finally:
                    capture.release()
                    self.store.merge({"camera_available": False})

                if not self._stop_event.is_set():
                    self._stop_event.wait(self.config.monitor.camera_open_retry_seconds)
        except Exception as error:  # noqa: BLE001
            self._last_error = f"后台监测线程异常: {error}"
            self.store.merge(
                {
                    "last_error": self._last_error,
                    "status_text": "后台监测发生异常",
                    "monitoring_state": "runtime_error",
                    "camera_available": False,
                }
            )
        finally:
            self.store.merge(
                {
                    "service_running": False,
                    "camera_available": False,
                }
            )

    def _open_camera(self, cv2: Any, index: int, width: int, height: int) -> Any | None:
        backend = cv2.CAP_DSHOW if sys.platform.startswith("win") else 0
        capture = cv2.VideoCapture(index, backend)
        capture.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        if capture.isOpened():
            self._last_error = ""
            self.store.merge(
                {
                    "camera_available": True,
                    "last_error": "",
                    "status_text": "疲劳监测运行中",
                    "monitoring_state": "tracking",
                }
            )
            return capture

        capture.release()
        self._last_error = f"无法打开摄像头 index={index}，将在后台重试"
        self.store.merge(
            {
                "camera_available": False,
                "last_error": self._last_error,
                "status_text": "摄像头不可用",
                "monitoring_state": "camera_error",
            }
        )
        return None

    def _camera_loop(self, cv2: Any, capture: Any, process_every: int) -> None:
        assert self._pipeline is not None
        target_fps = max(0.5, float(self.config.monitor.target_process_fps))
        target_interval = 1.0 / target_fps
        next_process_at = time.perf_counter()
        frame_index = 0
        read_failures = 0
        processed_timestamps: deque[float] = deque(maxlen=40)

        while not self._stop_event.is_set():
            ok, frame = capture.read()
            if not ok:
                read_failures += 1
                if read_failures >= self.config.monitor.camera_read_failure_limit:
                    self._last_error = "摄像头连续读取失败，正在重新连接"
                    self.store.merge(
                        {
                            "last_error": self._last_error,
                            "status_text": "摄像头正在重新连接",
                            "monitoring_state": "camera_reconnecting",
                        }
                    )
                    return
                time.sleep(0.02)
                continue

            read_failures = 0
            now_perf = time.perf_counter()
            should_process = frame_index % process_every == 0 and now_perf >= next_process_at
            frame_index += 1
            if not should_process:
                time.sleep(0.001)
                continue

            next_process_at = now_perf + target_interval
            if self.config.monitor.mirror_input:
                frame = cv2.flip(frame, 1)

            result = self._pipeline.analyze(frame)
            now_wall = time.time()
            processed_timestamps.append(now_perf)
            processed_fps = self._calculate_fps(processed_timestamps)
            sample = VisualCueSample(
                timestamp=now_wall,
                face_detected=bool(result.face_detected),
                eyes_closed=bool(result.both_eyes_closed),
                closed_probability=float(result.combined_closed_probability),
                yawn_detected=bool(result.yawn_detected),
                yawn_probability=(
                    float(result.yawn.risk_probability) if result.yawn is not None else 0.0
                ),
                face_score=float(result.face_score),
                inference_ms=float(result.total_ms),
            )
            snapshot = self._engine.update(sample, processed_fps=processed_fps)
            self.store.publish(
                self._build_payload(snapshot.to_dict(), camera_available=True)
            )

    def _build_payload(
        self,
        fatigue_payload: dict[str, Any],
        camera_available: bool,
    ) -> dict[str, Any]:
        return {
            "backend_version": "5.0",
            "service_running": self.running,
            "models_ready": self._models_ready,
            "camera_available": bool(camera_available),
            "last_error": self._last_error,
            "started_at_ms": int(self._started_at * 1000.0) if self._started_at else 0,
            "updated_at_ms": int(time.time() * 1000.0),
            **fatigue_payload,
        }

    def _camera_available(self) -> bool:
        return bool(self.store.snapshot()[1].get("camera_available", False))

    @staticmethod
    def _calculate_fps(timestamps: deque[float]) -> float:
        if len(timestamps) < 2:
            return 0.0
        duration = timestamps[-1] - timestamps[0]
        if duration <= 0.0:
            return 0.0
        return (len(timestamps) - 1) / duration
