from dms_backend.training.metrics import compute_classification_metrics


def test_binary_metrics_are_correct() -> None:
    metrics = compute_classification_metrics(
        targets=[0, 0, 1, 1],
        predictions=[0, 1, 1, 1],
        class_names={0: "negative", 1: "positive"},
    )

    assert metrics.confusion_matrix == [[1, 1], [0, 2]]
    assert metrics.accuracy == 0.75
    assert 0.0 < metrics.macro_f1 <= 1.0
