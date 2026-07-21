from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.fatigue.config import load_stage5_config
from dms_backend.fatigue.engine import FatigueEngine
from dms_backend.fatigue.types import VisualCueSample


def feed(
    engine: FatigueEngine,
    start: float,
    duration: float,
    *,
    closed: bool = False,
    yawn: bool = False,
    face: bool = True,
    step: float = 0.1,
) -> float:
    now = start
    last_level = engine.last_snapshot.fatigue_level
    last_event_id = engine.last_snapshot.event_id
    while now <= start + duration:
        snapshot = engine.update(
            VisualCueSample(
                timestamp=now,
                face_detected=face,
                eyes_closed=closed,
                closed_probability=0.96 if closed else 0.04,
                yawn_detected=yawn,
                yawn_probability=0.95 if yawn else 0.05,
                face_score=0.99 if face else 0.0,
                inference_ms=12.0,
            ),
            processed_fps=10.0,
        )
        if snapshot.fatigue_level != last_level:
            print(
                f"t={now:5.1f}s level {int(last_level)} -> "
                f"{int(snapshot.fatigue_level)} {snapshot.status_text}"
            )
            last_level = snapshot.fatigue_level
        if snapshot.event_id != last_event_id:
            print(
                f"t={now:5.1f}s ALERT #{snapshot.event_id}: "
                f"{snapshot.message} reasons={snapshot.reasons}"
            )
            last_event_id = snapshot.event_id
        now += step
    return now


def main() -> None:
    config = load_stage5_config()
    engine = FatigueEngine(config.fatigue)
    now = 0.0

    print("=== Stage 5 fatigue-state simulation ===")
    print("Normal tracking 10s")
    now = feed(engine, now, 10.0)

    print("\nClosed eyes 1.8s -> slight fatigue")
    now = feed(engine, now, 1.8, closed=True)

    print("\nOpen eyes 10s -> recover")
    now = feed(engine, now, 10.0)

    print("\nClosed eyes 3.2s -> severe fatigue")
    now = feed(engine, now, 3.2, closed=True)

    print("\nNo face 13s -> temporal reset")
    feed(engine, now, 13.0, face=False)

    print("\nSimulation complete")


if __name__ == "__main__":
    main()
