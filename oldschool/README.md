# Oldschool（Motif）

`OldschoolStyle` 是仓库内的 Motif 风格实现，源自 Qt 的 `QMotifStyle`，
在 `QCommonStyle` 之上实现经典的 Motif 绘制与度量：斜边面板、默认按钮外圈
指示、三态复选框、反白选中（`HighlightedText` 绑定 `Base`）、Motif 箭头与
滚动条、粗菜单项度量等。

本样式保持 Qt 原版 Motif 观感，不含 SGI 米灰配色与红色勾选——那是同目录下
独立的 `highschool` 样式（见下文）。

## 同目录的三个样式

本目录编译进单个插件 `oldschoolstyle`，注册三个独立可用的 style key：
`oldschool`、`newschool`、`highschool`。

- `oldschoolstyle`：Motif，见上。
- `newschoolstyle`：CDE 观感，独立类 `NewschoolStyle`（见其头文件注释）。
- `highschoolstyle`：在 `OldschoolStyle` 基类上叠加 SGI 勾选/单选与米灰配色
  （见下）。

# Highschool（SGI 勾选与米灰配色）

`HighschoolStyle` 在 `OldschoolStyle` 基类之上，叠加 SGI / Irix 的 **勾选与单选
pixmap** 与**米灰配色**：`drawPrimitive` 只覆盖 checkbox、radio 的绘制（红色对勾、
菱形单选钮里的红色圆点），其余所有绘制入口全部委托 `OldschoolStyle`；经典米灰
配色由 `standardPalette()` 提供。代码全部自研（Qt LGPL）。

## 实现策略

继承 `OldschoolStyle`（QCommonStyle 系）。SGI 的其他 pixmap 观感（按钮斜边、箭头、
滚动条条带、菜单项）在集成后观感不佳，已全部去掉，只保留最具辨识度的勾选/单选：

- 复选框：基类画斜边面板（选中凹陷、三态对角线），选中时再叠画**红色对勾**
  （粗两段折线 + Motif 式 1px 暗色投影），禁用时浅红（`PE_IndicatorCheckBox`）
- 单选钮：菱形指示器 + 选中中心画**红色实心圆点**（`PE_IndicatorRadioButton`）

菜单项的可勾选列由基类 `CE_MenuItem` 转画 `PE_IndicatorCheckBox` /
`PE_IndicatorRadioButton`，因此菜单里的勾选/选中也呈现同样的红色对勾与红点。

## 米灰配色（standardPalette，建议值）

`standardPalette()` 返回自洽的 SGI 米灰色板（按钮色直接取窗口色加深 120%、
列表 Base 取暖纸白加深 130%，不再依赖 polish 二次加工），是**建议值**：Qt 不会
自动采用（主题调色板优先），本样式也不再强制应用——用户
`QApplication::setPalette(style->standardPalette())` 时呈现经典米灰观感，
不采用时则跟随宿主主题。返回的调色板填满全部 `QPalette::ColorRole`
（Disabled 组由 `lib/qtstyles_palette.h` 的共享辅助统一派生），因此完全自足、
运行时切换样式不会串入上一个样式的配色。Motif 反白选中
（Highlight=Text / HighlightedText=Base）由 `OldschoolStyle::polish(QPalette&)`
在安装样式时自动处理，不属于配色建议本身。

| 角色 | 颜色 |
|------|------|
| Window（米灰） | #C9BEB1 |
| Button（比 Window 深 20%） | #C9BEB1.darker(120) |
| Light（Window 亮 135%） | #C9BEB1.lighter(135) |
| Base（暖纸白，加深 130%） | #F2ECE4.darker(130) |
| Highlight / HighlightedText | 由 OldschoolStyle::polish 反转（Motif 式反白选中） |

## 与 SGI 的差别

SGI 其余观感**没有**带入 Highschool：

- 厚实双色按钮斜边、多边形箭头、凹陷输入槽、凸起面板条带式滚动条、SGI 菜单 /
  菜单栏项 —— 均未覆盖，全部沿用 `OldschoolStyle` 的绘制。
- 粗斜体菜单标签、`WA_Hover` 悬停 —— `polish(QWidget)` 行为，未带入。
- 21px 宽滚动条、14×14 指示器 —— `pixelMetric` 行为，Highschool 沿用基类度量。
- 输入框米色/粉调凹陷槽（`#D3B5B5`）与菜单栏/工具栏/菜单的 Window 色 —— 已放弃，
  输入框与菜单跟随全局配色。

叠加"画什么 / 用什么颜色"的部分，不改"怎么算布局"的部分。

## 尺寸

全部沿用 `OldschoolStyle` / `QCommonStyle` 默认度量，未覆盖任何 `pixelMetric`。

## 实现说明

- 继承 `OldschoolStyle`，只覆盖 `drawPrimitive`（checkbox / radio 两个 case）与
  `standardPalette`，其余全部委托基类。
- 自研绘制全部用 QPainter 矢量（对勾为折线路径、单选圆点为椭圆），不依赖任何
  位图资源。
- 注释为英文，与仓库风格一致；每个 `.h`/`.cpp` 均带 Qt LGPL 许可证头。
