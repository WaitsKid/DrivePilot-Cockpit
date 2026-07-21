# 06 部署与运行说明

## 6.1 推荐环境

### Windows

- Windows 10/11 64 位；
- Git for Windows；
- Qt 6.9.1；
- Qt 模块：Quick、Multimedia、Network、WebSockets、Sql、Concurrent、WebEngineQuick、WebChannel；
- 可选模块：Positioning、TextToSpeech；
- CMake 3.16+；
- MSVC 2022 64 位或匹配 Qt 套件的 MinGW 64 位；
- Python 3.11；
- PyCharm（两个 Python 服务）；
- Qt Creator 或 CLion（Qt 客户端）。

## 6.2 克隆项目

```powershell
git clone https://github.com/WaitsKid/DrivePilot-Cockpit.git
cd DrivePilot-Cockpit
```

## 6.3 DMS 后端

### 创建环境

```powershell
cd dms-backend
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements-stage5.txt
```

### 模型准备

仓库不包含：

- Kaggle 数据集；
- `best.pt`；
- 眼睛/哈欠 ONNX；
- YuNet ONNX。

需要放置：

```text
dms-backend/
├── artifacts/
│   ├── onnx/
│   │   ├── eye_state_mobilenetv2.onnx
│   │   ├── eye_state_metadata.json
│   │   ├── yawn_state_mobilenetv2.onnx
│   │   └── yawn_state_metadata.json
│   └── evaluation/
│       ├── eye_state/deployment_config.json
│       └── yawn_state/deployment_config.json
└── models/
    └── yunet/
        └── face_detection_yunet_2026may.onnx
```

没有现成模型时，按照 `dms-backend/README.md` 从数据审计、训练、评估、导出逐步生成。

### 启动

```powershell
python scripts/15_validate_stage5.py
python scripts/16_run_server.py
```

检查：

```powershell
python scripts/17_check_service.py --watch-seconds 10
```

## 6.4 AI Agent 后端

```powershell
cd ..\agent-backend
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements-dev.txt
Copy-Item .env.example .env
```

`.env`：

```env
KIMI_API_KEY=你的真实Key
KIMI_BASE_URL=https://api.moonshot.cn/v1
KIMI_MODEL=kimi-k2.6
KIMI_THINKING_ENABLED=true
KIMI_KEEP_REASONING=true
KIMI_MAX_TOKENS=32768
```

启动：

```powershell
pytest -q
python scripts/run_server.py
```

未配置 Key 时仍可启动本地演示模式。

## 6.5 Qt 客户端

```powershell
cd ..\hmi-client
Copy-Item config.example.json config.json
```

填写配置后，在 Qt Creator：

1. 打开 `CMakeLists.txt`；
2. 选择 Qt 6.9.1 Desktop 64-bit Kit；
3. 第一次配置时检查 WebEngine、WebSockets、Sql 和 Multimedia 是否找到；
4. Build；
5. Run。

在 CLion：

1. 配置 Qt 对应的 CMake Toolchain；
2. 将 `CMAKE_PREFIX_PATH` 指向 Qt 套件；
3. Reload CMake Project；
4. 构建 `appBYD`。

## 6.6 启动顺序

推荐：

1. DMS 后端；
2. Agent 后端；
3. Qt 客户端。

Qt 支持后端稍后启动并自动恢复，但按上述顺序更容易观察日志。

## 6.7 端口

| 服务 | 端口 |
|---|---:|
| DMS | 8765 |
| Agent | 8770 |

端口冲突时分别修改：

- `dms-backend/configs/stage5.yaml`；
- `agent-backend/.env` 或 `app/config.py` 支持的环境变量；
- `hmi-client/config.json`。

## 6.8 常见问题

### Qt 提示找不到 `resource.qrc`

确认 `hmi-client/resource.qrc` 与 `Images/`、`Audio/` 均存在，并重新执行 CMake。

### TextToSpeech 没有声音

- 安装当前 Qt 套件的 TextToSpeech 模块；
- Windows 检查 SAPI 语音和音量合成器；
- 重新清空构建目录配置。

### 摄像头无法打开

- 关闭摄像头预览、会议软件和其他 DMS 进程；
- 检查 `configs/stage4.yaml` 中的摄像头索引；
- 检查 Windows 摄像头权限。

### Agent 显示本地演示模式

- 检查 `.env` 文件位置；
- 确认 `KIMI_API_KEY` 不是占位文字；
- 重启 Agent 后端。

### 地图空白

- 检查高德 JS Key 和安全密钥；
- 确认 WebEngine 模块已安装；
- 查看 Qt 控制台和 WebEngine 控制台错误。
