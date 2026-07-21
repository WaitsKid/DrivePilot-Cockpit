# Third-Party Notices

DrivePilot Cockpit 使用或对接以下第三方技术与服务。各项目的许可证、服务条款和数据政策以其官方版本为准。

## Frameworks and Libraries

- Qt 6 / Qt Quick / QML / Qt WebEngine / Qt Multimedia / Qt WebSockets / Qt SQL / Qt TextToSpeech
- Python
- FastAPI
- Uvicorn
- Pydantic
- HTTPX
- PyTorch and torchvision
- OpenCV
- OpenCV Zoo YuNet face detector
- ONNX and ONNX Runtime
- NumPy, pandas, Pillow, Matplotlib, PyYAML, tqdm
- pytest

## External Services

- Kimi / Moonshot AI
- 科大讯飞 WebSocket 语音识别
- 高德地图 Web 服务与 JavaScript API
- Open-Meteo

使用者必须自行申请凭据并遵守服务条款。真实凭据不包含在仓库中。

## Dataset

DMS 训练阶段使用过 Kaggle `yawn-eye-dataset-new`，其目录包含：

- `Closed`
- `Open`
- `no_yawn`
- `yawn`

仓库不重新分发该数据集。由于公开页面上的授权信息需要由使用者自行确认，训练权重也默认不随公共源码仓库发布。

## Model Files

仓库不直接包含：

- 用户训练的 MobileNetV2 `.pt` 和 `.onnx`；
- OpenCV Zoo YuNet `.onnx`。

请按照各自官方来源和许可证下载，并在本地放置。

## UI and Media Assets

部分 UI 图标和图片来自教学项目或第三方设计素材，当前仅用于非商业学习和作品集展示。其版权不因本仓库而转移。商业发布前应完成来源审计并替换为自有或明确授权素材。

`hmi-client/Images/Weather/` 中的天气 PNG 图标由项目作者从图标素材网站收集。公开仓库前应保存原始下载页面与许可信息；如果无法确认授权，应替换为自制或明确允许再分发的图标。

仓库内的示例音频仅用于项目演示；重新分发或商业使用前同样应确认来源与授权。
