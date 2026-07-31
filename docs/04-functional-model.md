# 04 功能模型与行为模型

## 4.1 功能分解

```mermaid
mindmap
  root((DrivePilot Cockpit))
    智能座舱 HMI
      首页与路由
      空调与车辆控制
      音乐与视频
      天气与导航
      联系人与通话
      计算器与画图
      设置与持久化
    驾驶员监测
      人脸检测
      眼睛分类
      哈欠分类
      PERCLOS
      四级状态机
      告警与语音
    AI Agent
      文本/语音输入
      请求理解
      工具规划
      Qt 执行
      结果观察
      多轮回复
    工程支撑
      配置管理
      日志
      自动测试
      隐私保护
      GitHub 交付
```

## 4.2 参与者与用例

```mermaid
flowchart LR
    Driver((驾驶员))
    Maintainer((维护者))
    Cloud((第三方服务))

    Driver --> UC1[操作空调与车辆设置]
    Driver --> UC2[播放音乐与视频]
    Driver --> UC3[查询天气和导航]
    Driver --> UC4[管理联系人和拨号]
    Driver --> UC5[语音或文本使用 AI Agent]
    Driver --> UC6[接收疲劳警告]

    Maintainer --> UC7[配置 API Key 与模型]
    Maintainer --> UC8[启动/停止两个后端]
    Maintainer --> UC9[运行测试和查看日志]

    UC3 --> Cloud
    UC5 --> Cloud
    UC6 --> DMS[DMS 后台]
```

## 4.3 系统上下文数据流

```mermaid
flowchart LR
    User[用户] -->|触摸/键盘/语音| Qt[Qt 中控客户端]
    Qt -->|页面、音频、提示| User
    Camera[摄像头] -->|本机帧| DMS[DMS 服务]
    DMS -->|疲劳状态 JSON| Qt
    Qt -->|用户文本、工具结果| Agent[Agent 服务]
    Agent -->|分析/计划/工具调用/回复| Qt
    Agent <--> Kimi[Kimi API]
    Qt <--> XFYUN[讯飞 ASR]
    Qt <--> AMap[高德]
    Qt <--> Weather[Open-Meteo]
    Qt <--> LocalData[(SQLite/QSettings/媒体文件)]
```

## 4.4 Agent 交互时序

```mermaid
sequenceDiagram
    actor U as 用户
    participant Q as Qt
    participant A as FastAPI Agent
    participant M as Kimi
    participant T as Qt Tool Bridge

    U->>Q: 输入“空调调到22度并播放下一首”
    Q->>A: user_message
    A-->>Q: analysis
    A->>M: messages + tool schemas
    M-->>A: tool_call(set_ac_temperature)
    A-->>Q: tool_call + call_id
    Q->>T: 校验并执行
    T-->>A: tool_result(success)
    A->>M: 追加 tool result
    M-->>A: tool_call(control_music)
    A-->>Q: tool_call + call_id
    Q->>T: 执行下一首
    T-->>A: tool_result(success)
    A->>M: 追加 tool result
    M-->>A: final response
    A-->>Q: final
    Q-->>U: 显示并语音回复
```

## 4.5 DMS 状态模型

```mermaid
stateDiagram-v2
    [*] --> Normal
    Normal --> Energetic: 持续良好且低 PERCLOS
    Energetic --> Normal: 良好条件不再满足
    Normal --> Slight: 闭眼/PERCLOS/哈欠达到轻度阈值
    Energetic --> Slight: 轻度阈值持续
    Slight --> Severe: 达到严重阈值
    Severe --> Slight: 持续改善达到恢复时间
    Slight --> Normal: 持续改善达到恢复时间
    Normal --> NoFace: 驾驶员离开
    Energetic --> NoFace: 驾驶员离开
    Slight --> NoFace: 驾驶员离开
    Severe --> NoFace: 驾驶员离开
    NoFace --> Normal: 重新检测到人脸
```

状态解释：

| level | 状态 | 图标 | 典型条件 |
|---:|---|---|---|
| 0 | 活力充沛 | 绿色 | 长时间稳定跟踪且 PERCLOS 很低 |
| 1 | 状态正常 | 蓝色 | 未触发疲劳规则 |
| 2 | 略微疲劳 | 黄色 | 闭眼约 1.1 秒、PERCLOS ≥ 0.30 或 2 次哈欠 |
| 3 | 严重疲劳 | 红色 | 闭眼约 2.5 秒、PERCLOS ≥ 0.52 或 3 次哈欠 |

## 4.6 页面导航模型

```mermaid
flowchart TB
    Home[首页] --> Apps[应用中心]
    Home --> AC[空调]
    Home --> Vehicle[车辆健康]
    Home --> Settings[车辆设置]
    Home --> Music[音乐]
    Home --> Map[地图导航]
    Apps --> Weather[天气]
    Apps --> Assistant[AI 助手]
    Apps --> Contacts[联系人]
    Apps --> Video[视频中心]
    Apps --> Calculator[科学计算器]
    Apps --> Vector[Vector Studio]
    Assistant -.工具调用.-> AC
    Assistant -.工具调用.-> Music
    Assistant -.工具调用.-> Settings
    Assistant -.工具调用.-> Map
```

## 4.7 关键业务规则

1. Agent 工具名必须位于后端白名单；
2. Qt 对温度、风量、音量和页面名再次校验；
3. DMS 告警只在 `event_id` 增加时提醒；
4. 普通眨眼不应直接判为疲劳，必须通过连续时间状态机；
5. 无人脸时不增加疲劳指标，长时间无人脸清空窗口；
6. 关闭疲劳提醒后隐藏图标、关闭告警、停止通信并请求后端释放摄像头；
7. API Key 只存在未提交的本地配置中。
