from dms_backend.evaluation.thresholds import PredictionRecord, find_best_risk_threshold


def test_risk_threshold_selection_for_closed_class() -> None:
    records = [
        PredictionRecord("a", 0, 0, 0.9, [0.90, 0.10]),
        PredictionRecord("b", 0, 0, 0.7, [0.70, 0.30]),
        PredictionRecord("c", 1, 1, 0.8, [0.20, 0.80]),
        PredictionRecord("d", 1, 1, 0.9, [0.10, 0.90]),
    ]
    threshold, metrics = find_best_risk_threshold(
        records,
        class_names={0: "Closed", 1: "Open"},
        risk_class_index=0,
        minimum=0.2,
        maximum=0.8,
        step=0.1,
    )
    assert 0.2 <= threshold <= 0.8
    assert metrics["macro_f1"] == 1.0
