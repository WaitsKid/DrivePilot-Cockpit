from dms_backend.service.status_store import DmsStatusStore


def test_status_store_revision_and_merge() -> None:
    store = DmsStatusStore()
    revision0, initial = store.snapshot()
    assert initial["fatigue_level"] == 1

    revision1 = store.merge({"fatigue_level": 2, "status": "slight_fatigue"})
    assert revision1 > revision0

    revision2, payload = store.wait_for_change(revision0, timeout_seconds=0.01)
    assert revision2 == revision1
    assert payload["fatigue_level"] == 2
