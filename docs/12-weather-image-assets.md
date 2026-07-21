# 天气图片素材清单

后续图片统一放入 `Images/Weather/`，建议使用透明背景 PNG 或 WebP、正方形画布、同一套视觉风格。

建议尺寸：256×256 或 512×512；主体四周保留约 10% 透明边距。

| 中文天气 | 建议文件名 | Open-Meteo/WMO 代码 |
|---|---|---|
| 晴 | `sunny.png` | 0 |
| 多云 | `partly_cloudy.png` | 1, 2 |
| 阴 | `cloudy.png` | 3 |
| 雾 | `fog.png` | 45, 48 |
| 毛毛雨 | `drizzle.png` | 51, 53, 55, 56, 57 |
| 小雨 | `light_rain.png` | 61 |
| 中雨 | `moderate_rain.png` | 63 |
| 大雨 | `heavy_rain.png` | 65, 66, 67 |
| 小雪 | `light_snow.png` | 71 |
| 中雪 | `moderate_snow.png` | 73 |
| 大雪 | `heavy_snow.png` | 75, 77 |
| 阵雨 | `rain_shower.png` | 80, 81, 82 |
| 阵雪 | `snow_shower.png` | 85, 86 |
| 雷暴 | `thunderstorm.png` | 95, 96, 99 |
| 未知天气 | `unknown.png` | 其他/异常代码 |

## 素材要求

- 不使用系统 Emoji 字体，避免 Windows/Qt Quick 首次字形栅格化导致渲染异常。
- 所有图片应来自同一套图标，线条粗细、阴影、色彩和透视保持一致。
- 推荐透明背景，不要带白底或黑底。
- 不要使用带水印、来源不明或明确禁止再分发的素材。
- 暂时不区分白天和夜间；后续可再增加 `clear_night.png`、`partly_cloudy_night.png`。
