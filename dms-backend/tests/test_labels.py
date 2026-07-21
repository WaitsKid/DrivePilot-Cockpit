from dms_backend.common.labels import DMS_LEVELS, EYE_STATE_TASK, YAWN_STATE_TASK


def test_eye_labels() -> None:
    assert EYE_STATE_TASK.class_to_index == {"Closed": 0, "Open": 1}


def test_yawn_labels() -> None:
    assert YAWN_STATE_TASK.class_to_index == {"no_yawn": 0, "yawn": 1}


def test_final_dms_levels() -> None:
    assert list(DMS_LEVELS) == [0, 1, 2, 3]
