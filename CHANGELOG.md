# Changelog

## [1.0.0] - 2026-07-22

### Added

- Qt Quick intelligent cockpit with adaptive canvas and multi-page navigation;
- vehicle controls, climate, control center, health visualization and settings;
- local music, video, contacts, calculator and Vector Studio;
- Amap navigation and administrative-district weather search;
- local PNG weather icon set and render-stability fix;
- final GitHub interface screenshot gallery;
- XFYUN streaming ASR;
- MobileNetV2 + YuNet DMS training, ONNX deployment and four-level fatigue state machine;
- global DMS status icon, Toast and TTS notification;
- FastAPI/Kimi tool-calling AI Agent with WebSocket tool-result loop;
- software engineering documentation, security rules and GitHub workflow.

### Security

- moved XFYUN/Amap credentials to ignored `config.json`;
- moved Kimi credentials to ignored `.env`;
- excluded datasets, personal images, model weights, runtime databases, media and build output;
- kept DMS camera frames inside the Python process.

### Known Limitations

- educational simulation only;
- no real vehicle bus;
- model and asset licensing must be reviewed before redistribution or commercial use;
- DMS is not safety certified.
