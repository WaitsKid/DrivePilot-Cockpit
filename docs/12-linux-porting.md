# Ubuntu 24.04 移植笔记

DrivePilot Cockpit 最早一直在 Windows 下开发，Qt 端使用过 MinGW 和 MSVC。为了确认这套 QML/C++ 工程没有被 Windows 路径和编译器环境绑死，我后来把 HMI 客户端迁到了 Ubuntu 24.04，并改用 Qt `gcc_64`、GCC、CMake 和 Ninja 构建。

这里记录的是 Qt HMI 本身的迁移过程。Python Agent、DMS 推理和模型训练没有纳入 Linux 运行范围。它们在 Windows 版本中主要用于完整功能展示，而 Linux 分支关注的是 QML/C++ 客户端能否独立完成配置、编译和启动。

## 开发环境

```text
Ubuntu 24.04.4 LTS x86_64
Linux kernel 7.0.0-28-generic
GCC / G++ 13.3.0
CMake 3.28.3
Ninja 1.11.1
Qt 6.9.1 gcc_64
```

Qt 使用 Online Installer 安装的 Desktop `gcc_64` Kit，工程用到的主要模块包括：

```text
Qt Quick
Qt Multimedia
Qt Network
Qt WebSockets
Qt Sql
Qt Concurrent
Qt WebEngineQuick
Qt WebChannel
```

`Positioning` 和 `TextToSpeech` 在 CMake 中按可选模块处理，没有安装时不会阻止整个工程配置。

## 迁移时处理的问题

### CMake 平台判断

Windows 下的 GUI 可执行程序使用 `WIN32_EXECUTABLE`，这个属性不能直接套到 Linux 目标上。我把它放进 `if(WIN32)` 分支，并分别保留 Windows、Linux 的平台宏：

```cmake
if (WIN32)
    set_target_properties(appDrivePilot PROPERTIES WIN32_EXECUTABLE TRUE)
    target_compile_definitions(appDrivePilot PRIVATE DRIVEPILOT_PLATFORM_WINDOWS=1)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(appDrivePilot PRIVATE DRIVEPILOT_PLATFORM_LINUX=1)
endif()
```

QML、C++ 和资源文件继续共用同一份清单，没有另外维护一套 Linux 工程。

### 文件路径与大小写

Windows 文件系统通常不区分大小写，Linux 会严格区分。迁移时重新检查了 QRC 路径、图片名称、QML 文件名和源码中的相对路径，避免出现 Windows 能加载、Linux 找不到资源的问题。

运行时数据仍通过 `QStandardPaths`、`QDir` 和 `QFileInfo` 获取位置，不把 `C:\`、`D:\` 或固定用户目录写进代码。

### 平台功能降级

电话页面使用：

```cpp
QDesktopServices::openUrl(QUrl("tel:..."));
```

桌面 Linux 一般没有注册 `tel:` 协议处理程序，因此调用失败时只在界面中提示，不影响主程序继续运行。

### WebEngine 与虚拟机显示

地图页面仍沿用 Windows 版的 WebEngine 方案。Ubuntu 虚拟机中，Wayland、XCB、显卡加速和 Chromium 后端之间可能出现兼容问题，所以运行脚本保留了 XCB 诊断开关，但没有把显示后端写死在源码中。

需要切换时可以这样启动：

```bash
DRIVEPILOT_FORCE_XCB=1 ./scripts/linux/run_hmi.sh
```

这只是桌面原型的兼容处理。量产车机中的地图通常会由原生地图 SDK、瓦片数据和 C++/OpenGL 渲染链路承担，不会直接照搬这个 WebEngine 实现。

## Ubuntu 依赖

我在 Ubuntu 中安装了下面这些基础开发和运行库：

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build gdb git pkg-config \
  libgl1-mesa-dev libxkbcommon-x11-0 libxcb-cursor0 \
  libnss3 libasound2-dev libpulse-dev \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav speech-dispatcher
```

其中 Mesa、XCB 和 NSS 主要关系到 Qt Quick、窗口系统和 WebEngine；GStreamer、ALSA、PulseAudio 用于媒体播放。

## 构建脚本

Linux 分支保留了三个脚本：

```text
scripts/linux/check_environment.sh
scripts/linux/configure_hmi_debug.sh
scripts/linux/run_hmi.sh
```

它们分别负责：

- 检查 GCC、CMake、Ninja、Qt 路径和必要模块；
- 生成 Ninja Debug 构建目录并编译 HMI；
- 设置运行环境后启动 `appDrivePilot`。

脚本只处理 Qt 客户端，不创建 Python 虚拟环境，也不会启动 Agent 或 DMS 服务。

## 配置与编译

Qt 安装在默认用户目录时，可以这样配置：

```bash
cd ~/Projects/DrivePilot-Cockpit
export QT_ROOT="$HOME/Qt/6.9.1/gcc_64"
./scripts/linux/check_environment.sh
rm -rf out/hmi-linux-debug
./scripts/linux/configure_hmi_debug.sh
```

生成的程序位于：

```text
out/hmi-linux-debug/appDrivePilot
```

启动命令：

```bash
./scripts/linux/run_hmi.sh
```

我主要检查了主窗口启动、QML 模块加载、资源读取和基础页面切换。Agent 和 DMS 没有启动时，相关入口可以显示离线状态，但不应影响 HMI 主进程。

## 迁移中遇到的几个问题

### Qt 模块找不到

最常见的原因是 CMake 没有使用 Online Installer 安装的 `gcc_64` Kit。可以先确认：

```bash
export QT_ROOT="$HOME/Qt/6.9.1/gcc_64"
ls "$QT_ROOT/lib/cmake/Qt6"
```

再删除旧构建目录重新配置，避免混入系统 Qt 或之前的 CMake 缓存。

### WebEngine 页面空白

先确认安装了 `Qt WebEngine`，再观察终端中的 Chromium、OpenGL 和窗口后端报错。虚拟机中可用 XCB 启动方式判断问题是否来自 Wayland。

### 媒体没有声音

检查 `/dev/snd` 是否存在，并确认 GStreamer 插件已安装。虚拟机没有音频设备时，HMI 仍然可以启动，只是音乐和视频页面无法正常输出声音。

### VMware 共享目录变成 ext4

有一次 `/mnt/hgfs` 没有真正挂载，Linux 中虽然能看到同名目录，但 `findmnt` 显示它属于本地 `ext4`，复制进去的文件自然不会出现在 Windows。

我用下面几条命令确认并重新挂载：

```bash
findmnt -T /mnt/hgfs
vmware-hgfsclient
sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other,uid=$(id -u),gid=$(id -g),umask=0022
```

正常情况下，文件系统类型应显示为：

```text
fuse.vmhgfs-fuse
```

## 这个分支代表什么

这个分支证明的是同一套 Qt Quick/QML + C++ HMI 可以从 Windows 工具链迁移到 Ubuntu `gcc_64` 环境，并完成独立构建和基础运行。

它不是一套量产车机 Linux 方案。真实项目中，地图、媒体、语音、视觉算法和车辆服务通常由不同团队和系统组件提供，Qt HMI 更偏向界面呈现、状态管理与交互整合。这里保留的桌面功能主要用于展示复杂 Qt 工程的组织方式和跨模块协作。
