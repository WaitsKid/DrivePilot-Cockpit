from __future__ import annotations

from collections import deque
from dataclasses import replace

from .config import FatigueConfig
from .types import FatigueEvent, FatigueLevel, FatigueSnapshot, VisualCueSample


class FatigueEngine:
    """Fuse per-frame visual cues into a debounced four-level DMS state."""

    def __init__(self, config: FatigueConfig) -> None:
        self.config = config
        self._samples: deque[VisualCueSample] = deque()
        self._yawn_events: deque[float] = deque()
        self._current_level = FatigueLevel.NORMAL
        self._candidate_level: FatigueLevel | None = None
        self._candidate_since = 0.0
        self._closed_started_at: float | None = None
        self._eye_open_started_at: float | None = None
        self._yawn_started_at: float | None = None
        self._yawn_release_started_at: float | None = None
        self._yawn_latched = False
        self._face_missing_since: float | None = None
        self._event_id = 0
        self._last_event: FatigueEvent | None = None
        self._last_alert_at = 0.0
        self._last_risk_reasons: list[str] = []
        self._last_snapshot = FatigueSnapshot(timestamp=0.0)

    @property
    def current_level(self) -> FatigueLevel:
        return self._current_level

    @property
    def last_snapshot(self) -> FatigueSnapshot:
        return replace(self._last_snapshot, reasons=list(self._last_snapshot.reasons))

    def reset(self, timestamp: float = 0.0) -> FatigueSnapshot:
        self._samples.clear()
        self._yawn_events.clear()
        self._current_level = FatigueLevel.NORMAL
        self._candidate_level = None
        self._candidate_since = 0.0
        self._closed_started_at = None
        self._eye_open_started_at = None
        self._yawn_started_at = None
        self._yawn_release_started_at = None
        self._yawn_latched = False
        self._face_missing_since = None
        self._last_alert_at = 0.0
        self._last_risk_reasons = []
        self._last_snapshot = FatigueSnapshot(
            timestamp=timestamp,
            fatigue_level=FatigueLevel.NORMAL,
            raw_fatigue_level=FatigueLevel.NORMAL,
            status_text=FatigueLevel.NORMAL.text,
            monitoring_state="reset",
            event_id=self._event_id,
            event_type=self._last_event.event_type if self._last_event else "",
            message=self._last_event.message if self._last_event else "",
            last_event_timestamp=self._last_event.timestamp if self._last_event else 0.0,
        )
        return self.last_snapshot

    def update(self, sample: VisualCueSample, processed_fps: float = 0.0) -> FatigueSnapshot:
        now = float(sample.timestamp)
        self._prune(now)

        if not sample.face_detected:
            return self._handle_missing_face(sample, processed_fps)

        self._face_missing_since = None
        self._samples.append(sample)
        self._update_closed_episode(sample)
        self._update_yawn_event(sample)
        self._prune(now)

        closed_duration = self._closed_duration(now)
        perclos, valid_window = self._calculate_perclos(now)
        yawn_count = len(self._yawn_events)
        raw_level, reasons = self._classify(
            sample=sample,
            closed_duration=closed_duration,
            perclos=perclos,
            valid_window=valid_window,
            yawn_count=yawn_count,
        )
        if raw_level >= FatigueLevel.SLIGHT:
            self._last_risk_reasons = list(reasons)
        previous_level = self._apply_state_hysteresis(raw_level, now)
        self._maybe_emit_alert(now, reasons, previous_level)

        risk_score = self._calculate_risk_score(
            closed_duration=closed_duration,
            perclos=perclos,
            valid_window=valid_window,
            yawn_count=yawn_count,
        )
        display_reasons = (
            reasons
            if raw_level >= FatigueLevel.SLIGHT or self._current_level < FatigueLevel.SLIGHT
            else self._last_risk_reasons
        )
        self._last_snapshot = FatigueSnapshot(
            timestamp=now,
            fatigue_level=self._current_level,
            raw_fatigue_level=raw_level,
            status_text=self._current_level.text,
            monitoring_state="tracking",
            face_detected=True,
            face_score=sample.face_score,
            closed_probability=sample.closed_probability,
            yawn_probability=sample.yawn_probability,
            eyes_closed=sample.eyes_closed,
            yawn_detected=sample.yawn_detected,
            closed_duration_seconds=closed_duration,
            perclos=perclos,
            valid_window_seconds=valid_window,
            yawn_count_window=yawn_count,
            risk_score=risk_score,
            inference_ms=sample.inference_ms,
            processed_fps=processed_fps,
            reasons=list(display_reasons),
            event_id=self._event_id,
            event_type=self._last_event.event_type if self._last_event else "",
            message=self._last_event.message if self._last_event else "",
            last_event_timestamp=self._last_event.timestamp if self._last_event else 0.0,
        )
        return self.last_snapshot

    def _handle_missing_face(
        self,
        sample: VisualCueSample,
        processed_fps: float,
    ) -> FatigueSnapshot:
        now = float(sample.timestamp)
        if self._face_missing_since is None:
            self._face_missing_since = now
        missing_seconds = max(0.0, now - self._face_missing_since)

        self._closed_started_at = None
        self._eye_open_started_at = None
        self._yawn_started_at = None
        self._yawn_release_started_at = None
        self._yawn_latched = False
        self._candidate_level = None

        if missing_seconds >= self.config.face_missing_reset_seconds:
            self._samples.clear()
            self._yawn_events.clear()
            self._current_level = FatigueLevel.NORMAL

        self._last_snapshot = FatigueSnapshot(
            timestamp=now,
            fatigue_level=self._current_level,
            raw_fatigue_level=self._current_level,
            status_text="未检测到驾驶员",
            monitoring_state="no_face",
            face_detected=False,
            inference_ms=sample.inference_ms,
            processed_fps=processed_fps,
            reasons=[f"连续 {missing_seconds:.1f} 秒未检测到人脸"],
            event_id=self._event_id,
            event_type=self._last_event.event_type if self._last_event else "",
            message=self._last_event.message if self._last_event else "",
            last_event_timestamp=self._last_event.timestamp if self._last_event else 0.0,
        )
        return self.last_snapshot

    def _update_closed_episode(self, sample: VisualCueSample) -> None:
        now = sample.timestamp
        if sample.eyes_closed:
            if self._closed_started_at is None:
                self._closed_started_at = now
            self._eye_open_started_at = None
            return

        if self._closed_started_at is None:
            self._eye_open_started_at = None
            return

        if self._eye_open_started_at is None:
            self._eye_open_started_at = now
        if now - self._eye_open_started_at >= self.config.eye_open_release_seconds:
            self._closed_started_at = None
            self._eye_open_started_at = None

    def _update_yawn_event(self, sample: VisualCueSample) -> None:
        now = sample.timestamp
        if sample.yawn_detected:
            self._yawn_release_started_at = None
            if self._yawn_started_at is None:
                self._yawn_started_at = now
            if (
                not self._yawn_latched
                and now - self._yawn_started_at >= self.config.minimum_yawn_event_seconds
            ):
                self._yawn_events.append(now)
                self._yawn_latched = True
            return

        if self._yawn_started_at is None and not self._yawn_latched:
            return
        if self._yawn_release_started_at is None:
            self._yawn_release_started_at = now
        if now - self._yawn_release_started_at >= self.config.yawn_release_seconds:
            self._yawn_started_at = None
            self._yawn_release_started_at = None
            self._yawn_latched = False

    def _closed_duration(self, now: float) -> float:
        if self._closed_started_at is None:
            return 0.0
        return max(0.0, now - self._closed_started_at)

    def _calculate_perclos(self, now: float) -> tuple[float, float]:
        if not self._samples:
            return 0.0, 0.0

        samples = list(self._samples)
        closed_seconds = 0.0
        valid_seconds = 0.0
        for index, sample in enumerate(samples):
            next_timestamp = samples[index + 1].timestamp if index + 1 < len(samples) else now
            duration = max(0.0, next_timestamp - sample.timestamp)
            duration = min(duration, self.config.maximum_sample_gap_seconds)
            if duration <= 0.0 or not sample.face_detected:
                continue
            valid_seconds += duration
            if sample.eyes_closed:
                closed_seconds += duration

        if valid_seconds <= 1e-9:
            return 0.0, 0.0
        return closed_seconds / valid_seconds, valid_seconds

    def _classify(
        self,
        sample: VisualCueSample,
        closed_duration: float,
        perclos: float,
        valid_window: float,
        yawn_count: int,
    ) -> tuple[FatigueLevel, list[str]]:
        severe_reasons: list[str] = []
        slight_reasons: list[str] = []

        if closed_duration >= self.config.severe_closed_seconds:
            severe_reasons.append(f"持续闭眼 {closed_duration:.1f} 秒")
        if (
            valid_window >= self.config.minimum_valid_window_seconds
            and perclos >= self.config.severe_perclos
        ):
            severe_reasons.append(f"PERCLOS 达到 {perclos:.0%}")
        if yawn_count >= self.config.severe_yawn_count:
            severe_reasons.append(
                f"{self.config.yawn_window_seconds:.0f} 秒内检测到 {yawn_count} 次哈欠"
            )
        if severe_reasons:
            return FatigueLevel.SEVERE, severe_reasons

        if closed_duration >= self.config.slight_closed_seconds:
            slight_reasons.append(f"持续闭眼 {closed_duration:.1f} 秒")
        if (
            valid_window >= self.config.minimum_valid_window_seconds
            and perclos >= self.config.slight_perclos
        ):
            slight_reasons.append(f"PERCLOS 达到 {perclos:.0%}")
        if yawn_count >= self.config.slight_yawn_count:
            slight_reasons.append(
                f"{self.config.yawn_window_seconds:.0f} 秒内检测到 {yawn_count} 次哈欠"
            )
        if slight_reasons:
            return FatigueLevel.SLIGHT, slight_reasons

        energetic = (
            valid_window >= self.config.energetic_minimum_tracking_seconds
            and perclos <= self.config.energetic_perclos_max
            and yawn_count == 0
            and not sample.eyes_closed
            and sample.closed_probability < 0.35
            and not sample.yawn_detected
        )
        if energetic:
            return FatigueLevel.ENERGETIC, ["长时间保持清醒且未检测到哈欠"]
        return FatigueLevel.NORMAL, ["闭眼比例与哈欠频率处于正常范围"]

    def _apply_state_hysteresis(
        self, raw_level: FatigueLevel, now: float
    ) -> FatigueLevel | None:
        if raw_level == self._current_level:
            self._candidate_level = None
            self._candidate_since = 0.0
            return None

        target = self._transition_target(raw_level)
        if self._candidate_level != target:
            self._candidate_level = target
            self._candidate_since = now
            return None

        hold_seconds = self._transition_hold_seconds(target)
        if now - self._candidate_since >= hold_seconds:
            previous_level = self._current_level
            self._current_level = target
            self._candidate_level = None
            self._candidate_since = 0.0
            return previous_level
        return None

    def _transition_target(self, raw_level: FatigueLevel) -> FatigueLevel:
        if raw_level > self._current_level:
            return raw_level
        if self._current_level == FatigueLevel.SEVERE:
            return FatigueLevel.SLIGHT
        if self._current_level == FatigueLevel.SLIGHT:
            return FatigueLevel.NORMAL
        if self._current_level == FatigueLevel.NORMAL:
            return FatigueLevel.ENERGETIC
        return FatigueLevel.ENERGETIC

    def _transition_hold_seconds(self, target: FatigueLevel) -> float:
        if target == FatigueLevel.SEVERE:
            return self.config.severe_escalation_hold_seconds
        if target == FatigueLevel.SLIGHT and target > self._current_level:
            return self.config.slight_escalation_hold_seconds
        if self._current_level == FatigueLevel.SEVERE:
            return self.config.recovery_from_severe_seconds
        if self._current_level == FatigueLevel.SLIGHT:
            return self.config.recovery_from_slight_seconds
        if target == FatigueLevel.ENERGETIC:
            return self.config.recovery_to_energetic_seconds
        return 0.80

    def _maybe_emit_alert(
        self,
        now: float,
        reasons: list[str],
        previous_level: FatigueLevel | None,
    ) -> None:
        if self._current_level < FatigueLevel.SLIGHT:
            return

        escalated = (
            previous_level is not None and self._current_level > previous_level
        )
        recovered = (
            previous_level is not None and self._current_level < previous_level
        )
        if recovered:
            return
        cooldown = (
            self.config.severe_reminder_cooldown_seconds
            if self._current_level == FatigueLevel.SEVERE
            else self.config.slight_reminder_cooldown_seconds
        )
        reminder_due = now - self._last_alert_at >= cooldown
        if not escalated and self._last_event is not None and not reminder_due:
            return

        active_reasons = self._last_risk_reasons or reasons
        reason = "；".join(active_reasons) if active_reasons else "疲劳风险指标持续偏高"
        if self._current_level == FatigueLevel.SEVERE:
            message = "警告，检测到严重疲劳，请立即停车休息。"
            event_type = "severe_fatigue_alert"
        else:
            message = "检测到轻微疲劳，请注意休息。"
            event_type = "slight_fatigue_alert"

        self._event_id += 1
        self._last_alert_at = now
        self._last_event = FatigueEvent(
            event_id=self._event_id,
            timestamp=now,
            level=self._current_level,
            event_type=event_type,
            message=message,
            reason=reason,
        )

    def _calculate_risk_score(
        self,
        closed_duration: float,
        perclos: float,
        valid_window: float,
        yawn_count: int,
    ) -> float:
        closed_score = closed_duration / max(self.config.severe_closed_seconds, 1e-6)
        perclos_score = (
            perclos / max(self.config.severe_perclos, 1e-6)
            if valid_window >= self.config.minimum_valid_window_seconds
            else 0.0
        )
        yawn_score = yawn_count / max(float(self.config.severe_yawn_count), 1.0)
        return max(0.0, min(1.0, max(closed_score, perclos_score, yawn_score)))

    def _prune(self, now: float) -> None:
        sample_cutoff = now - self.config.perclos_window_seconds
        while self._samples and self._samples[0].timestamp < sample_cutoff:
            self._samples.popleft()

        yawn_cutoff = now - self.config.yawn_window_seconds
        while self._yawn_events and self._yawn_events[0] < yawn_cutoff:
            self._yawn_events.popleft()
