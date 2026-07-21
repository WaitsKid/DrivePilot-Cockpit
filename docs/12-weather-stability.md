# 天气页面稳定性说明

## 问题表现

在 Windows / MSVC 环境中，首次打开天气详情页可能出现短暂黑屏、页面切换无响应，尤其在图形驱动、Direct3D 场景图和字体缓存状态不稳定时更明显。

## 本版本处理

1. 主页面 Loader 改为异步实例化，避免复杂页面对象树一次性阻塞 GUI 线程。
2. 移除天气页入场透明度动画，避免首帧未完成时页面长时间保持透明。
3. 候选模型初始化延后到下一轮事件循环。
4. 移除两个原生 BusyIndicator，改用静态轻量提示。
5. 天气图标改用单色符号，避免批量彩色 Emoji 首次字形栅格化峰值。
6. 高德行政区请求的 subdistrict 深度由 3 降为 1，避免宽泛关键词返回超大嵌套 JSON 后在 GUI 线程递归解析。
7. 页面 Loader 出错时显示 Toast，并提示查看 Application Output 的首条 QML 错误。

## 排查命令

在 Qt Creator 的 Run Environment 中临时加入：

```text
QSG_INFO=1
QT_LOGGING_RULES=qt.scenegraph.general=true;qt.rhi.*=true
```

若问题只在默认 Direct3D 后端出现，可临时测试：

```text
QSG_RHI_BACKEND=opengl
```

这些变量仅用于定位，不建议直接作为项目永久配置。
