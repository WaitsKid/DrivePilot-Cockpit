from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class BinaryTask:
    name: str
    class_to_index: dict[str, int]

    @property
    def index_to_class(self) -> dict[int, str]:
        return {index: name for name, index in self.class_to_index.items()}


EYE_STATE_TASK = BinaryTask(
    name="eye_state",
    class_to_index={"Closed": 0, "Open": 1},
)

YAWN_STATE_TASK = BinaryTask(
    name="yawn_state",
    class_to_index={"no_yawn": 0, "yawn": 1},
)

TASKS = {
    EYE_STATE_TASK.name: EYE_STATE_TASK,
    YAWN_STATE_TASK.name: YAWN_STATE_TASK,
}

# 最终 DMS 状态由时间窗口融合产生，不直接等于数据集四个目录。
DMS_LEVELS = {
    0: "energetic",
    1: "normal",
    2: "slight_fatigue",
    3: "severe_fatigue",
}
