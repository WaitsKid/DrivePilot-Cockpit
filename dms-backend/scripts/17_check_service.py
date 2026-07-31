from __future__ import annotations

import argparse
import json
import time
import urllib.error
import urllib.request


def request_json(url: str, method: str = "GET") -> tuple[int, dict]:
    request = urllib.request.Request(url, method=method)
    with urllib.request.urlopen(request, timeout=5.0) as response:
        return response.status, json.loads(response.read().decode("utf-8"))


def main() -> None:
    parser = argparse.ArgumentParser(description="Check a running DriveGuard DMS service")
    parser.add_argument("--base-url", default="http://127.0.0.1:8765")
    parser.add_argument("--watch-seconds", type=float, default=10.0)
    args = parser.parse_args()

    base_url = args.base_url.rstrip("/")
    try:
        status_code, health = request_json(f"{base_url}/health")
        print(f"Health HTTP {status_code}")
        print(json.dumps(health, ensure_ascii=False, indent=2))
    except urllib.error.HTTPError as error:
        print(f"Health HTTP {error.code}")
        print(error.read().decode("utf-8", errors="replace"))
    except OSError as error:
        raise SystemExit(f"无法连接服务 {base_url}: {error}") from error

    deadline = time.time() + max(0.0, args.watch_seconds)
    previous_event_id = -1
    while time.time() < deadline:
        _, status = request_json(f"{base_url}/api/v1/dms/status")
        event_id = int(status.get("event_id", 0))
        print(
            "level={fatigue_level} status={status_text} face={face_detected} "
            "closed={closed_probability:.2f} perclos={perclos:.2f} "
            "yawns={yawn_count_window} fps={processed_fps:.1f}".format(
                fatigue_level=status.get("fatigue_level", -1),
                status_text=status.get("status_text", ""),
                face_detected=status.get("face_detected", False),
                closed_probability=float(status.get("closed_probability", 0.0)),
                perclos=float(status.get("perclos", 0.0)),
                yawn_count_window=int(status.get("yawn_count_window", 0)),
                processed_fps=float(status.get("processed_fps", 0.0)),
            )
        )
        if event_id != previous_event_id and event_id > 0:
            print(
                "NEW ALERT:",
                event_id,
                status.get("event_type", ""),
                status.get("message", ""),
            )
        previous_event_id = event_id
        time.sleep(1.0)


if __name__ == "__main__":
    main()
