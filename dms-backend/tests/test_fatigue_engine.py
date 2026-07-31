from __future__ import annotations

from dataclasses import replace

from dms_backend.fatigue.config import FatigueConfig
from dms_backend.fatigue.engine import FatigueEngine
from dms_backend.fatigue.types import FatigueLevel, VisualCueSample


def sample(
    timestamp: float,
    *,
    face: bool = True,
    closed: bool = False,
    yawn: bool = False,
) -> VisualCueSample:
    return VisualCueSample(
        timestamp=timestamp,
        face_detected=face,
        eyes_closed=closed,
        closed_probability=0.95 if closed else 0.05,
        yawn_detected=yawn,
        yawn_probability=0.95 if yawn else 0.05,
        face_score=0.99 if face else 0.0,
        inference_ms=10.0,
    )


def fast_config() -> FatigueConfig:
    return replace(
        FatigueConfig(),
        minimum_valid_window_seconds=1.0,
        energetic_minimum_tracking_seconds=2.0,
        slight_closed_seconds=0.5,
        severe_closed_seconds=1.2,
        slight_perclos=0.35,
        severe_perclos=0.70,
        slight_escalation_hold_seconds=0.2,
        severe_escalation_hold_seconds=0.2,
        recovery_from_severe_seconds=0.4,
        recovery_from_slight_seconds=0.4,
        recovery_to_energetic_seconds=0.5,
        minimum_yawn_event_seconds=0.2,
        yawn_release_seconds=0.1,
        slight_yawn_count=2,
        severe_yawn_count=4,
        face_missing_reset_seconds=0.5,
        slight_reminder_cooldown_seconds=10.0,
        severe_reminder_cooldown_seconds=10.0,
    )


def feed(
    engine: FatigueEngine,
    start: float,
    duration: float,
    *,
    closed: bool = False,
    yawn: bool = False,
    face: bool = True,
    step: float = 0.1,
):
    now = start
    snapshot = engine.last_snapshot
    while now <= start + duration + 1e-9:
        snapshot = engine.update(
            sample(now, face=face, closed=closed, yawn=yawn),
            processed_fps=10.0,
        )
        now += step
    return now, snapshot


def test_continuous_closed_eyes_escalate_to_severe() -> None:
    engine = FatigueEngine(fast_config())
    now, _ = feed(engine, 100.0, 0.8, closed=False)
    _, snapshot = feed(engine, now, 2.0, closed=True)

    assert snapshot.fatigue_level == FatigueLevel.SEVERE
    assert snapshot.event_id >= 1
    assert snapshot.event_type == "severe_fatigue_alert"
    assert snapshot.closed_duration_seconds >= 1.2


def test_two_confirmed_yawns_trigger_slight_fatigue() -> None:
    engine = FatigueEngine(fast_config())
    now, _ = feed(engine, 200.0, 1.2)

    for _ in range(2):
        now, _ = feed(engine, now, 0.4, yawn=True)
        now, _ = feed(engine, now, 0.3, yawn=False)
    _, snapshot = feed(engine, now, 0.4)

    assert snapshot.yawn_count_window == 2
    assert snapshot.fatigue_level == FatigueLevel.SLIGHT
    assert snapshot.event_type == "slight_fatigue_alert"


def test_missing_face_resets_temporal_state() -> None:
    engine = FatigueEngine(fast_config())
    now, _ = feed(engine, 300.0, 1.8, closed=True)
    _, snapshot = feed(engine, now, 0.8, face=False)

    assert snapshot.monitoring_state == "no_face"
    assert snapshot.face_detected is False
    assert snapshot.fatigue_level == FatigueLevel.NORMAL


def test_long_clear_tracking_can_become_energetic() -> None:
    engine = FatigueEngine(fast_config())
    _, snapshot = feed(engine, 400.0, 3.2, closed=False, yawn=False)

    assert snapshot.raw_fatigue_level == FatigueLevel.ENERGETIC
    assert snapshot.fatigue_level == FatigueLevel.ENERGETIC
