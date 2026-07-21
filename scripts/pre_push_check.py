from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAX_FILE_BYTES = 95 * 1024 * 1024

FORBIDDEN_NAMES = {
    "config.json",
    ".env",
    "CMakeCache.txt",
}
FORBIDDEN_DIRS = {
    ".venv",
    "venv",
    "__pycache__",
    ".pytest_cache",
    ".idea",
    ".vscode",
}
REQUIRED_PATHS = [
    "README.md",
    "LICENSE",
    "THIRD_PARTY_NOTICES.md",
    "hmi-client/CMakeLists.txt",
    "hmi-client/config.example.json",
    "hmi-client/resource.qrc",
    "dms-backend/requirements-stage5.txt",
    "agent-backend/.env.example",
    "docs/02-requirements-specification.md",
    "docs/12-interface-gallery.md",
    "Image/Home.png",
    "Image/Agent.png",
    "hmi-client/Components/WeatherIcon.qml",
    "hmi-client/Images/Weather/sunny.png",
]
TEXT_SUFFIXES = {
    ".txt", ".md", ".py", ".cpp", ".c", ".h", ".hpp", ".qml",
    ".json", ".yaml", ".yml", ".cmake", ".ps1", ".ini", ".toml",
}

# Strong patterns only. Placeholder/example text is allowed.
SECRET_PATTERNS = [
    ("OpenAI-like key", re.compile(r"\bsk-[A-Za-z0-9_-]{24,}\b")),
    ("GitHub token", re.compile(r"\bgh[pousr]_[A-Za-z0-9]{30,}\b")),
    ("AWS access key", re.compile(r"\bAKIA[0-9A-Z]{16}\b")),
    ("Bearer token", re.compile(r"Bearer\s+[A-Za-z0-9._-]{32,}", re.I)),
]

PLACEHOLDER_WORDS = ("你的", "your", "example", "placeholder", "replace_me", "changeme")


def iter_paths():
    for path in ROOT.rglob("*"):
        if ".git" in path.parts:
            continue
        yield path


def is_placeholder_line(line: str) -> bool:
    normalized = line.lower()
    return any(word in normalized for word in PLACEHOLDER_WORDS)


def main() -> int:
    errors: list[str] = []
    warnings: list[str] = []
    file_count = 0
    total_bytes = 0

    for required in REQUIRED_PATHS:
        if not (ROOT / required).exists():
            errors.append(f"缺少必要文件：{required}")

    for path in iter_paths():
        rel = path.relative_to(ROOT)
        if path.is_dir():
            if path.name in FORBIDDEN_DIRS:
                errors.append(f"存在缓存/本地目录：{rel}")
            continue

        file_count += 1
        size = path.stat().st_size
        total_bytes += size

        if path.name in FORBIDDEN_NAMES:
            errors.append(f"存在不应提交的文件：{rel}")
        if size > MAX_FILE_BYTES:
            errors.append(f"单文件超过 95 MB：{rel} ({size / 1024 / 1024:.1f} MB)")
        elif size > 20 * 1024 * 1024:
            warnings.append(f"较大文件：{rel} ({size / 1024 / 1024:.1f} MB)")

        if path.suffix.lower() not in TEXT_SUFFIXES and path.name not in {
            "CMakeLists.txt", ".gitignore", ".gitattributes", ".env.example"
        }:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        for line_number, line in enumerate(text.splitlines(), start=1):
            if is_placeholder_line(line):
                continue
            for label, pattern in SECRET_PATTERNS:
                if pattern.search(line):
                    errors.append(f"疑似 {label}：{rel}:{line_number}")

        # Validate example JSON, while ensuring it contains only placeholders.
        if path.name == "config.example.json":
            try:
                obj = json.loads(text)
            except json.JSONDecodeError as exc:
                errors.append(f"示例 JSON 无效：{rel}: {exc}")
            else:
                for key, value in obj.items():
                    if any(token in key.lower() for token in ("key", "secret", "app_id")):
                        if isinstance(value, str) and value and not is_placeholder_line(value):
                            warnings.append(f"示例配置字段看起来不像占位值：{rel} -> {key}")

    print("=== DrivePilot pre-push check ===")
    print(f"Root: {ROOT}")
    print(f"Files: {file_count}")
    print(f"Size: {total_bytes / 1024 / 1024:.2f} MB")

    if warnings:
        print("\nWarnings:")
        for item in warnings:
            print(f"  - {item}")

    if errors:
        print("\nFAILED:")
        for item in errors:
            print(f"  - {item}")
        return 1

    print("\nPASS: 未发现禁止文件、超大文件或明显密钥。")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
