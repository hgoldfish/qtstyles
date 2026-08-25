# Keramik（自研重写）

Keramik 是 KDE 3 / TDE 时代（2002 年左右）的陶瓷主题风格，以平滑的斜角渐变、
轻微圆角的按钮、细线边框，以及用高亮色（accent）突出的滑块 / 滚动条把手为特征。
本模块以**观感规格**的方式还原它：只提炼原始外观的颜色、几何与绘制顺序，代码
全部自研（LGPL），不包含 KDE/TDE 原版 Qt 3 风格或任何社区移植版的代码，
也不依赖任何 KDE 库。

## 观感规格

### 经典配色（`standardPalette()`，建议值）

KDE 3 默认配色的陶瓷观感由 `standardPalette()` 提供，是**建议值**：Qt 不会
自动采用，本样式也不会强制应用；应用可调用
`QApplication::setPalette(style->standardPalette())` 呈现经典观感。默认绘制
仍从当前 QPalette 自动推导（见下文色阶表）。

返回的调色板填满全部 `QPalette::ColorRole`（Disabled 组由
`lib/qtstyles_palette.h` 的共享辅助统一派生），因此完全自足、不继承调用时
的 app 调色板——运行时切换样式不会串入上一个样式的配色。

| 角色 | 值 | 说明 |
|------|-----|------|
| Window | `#dfdfdf` | 浅灰窗口底色 |
| Button | `#d8d8d8` | 陶瓷按钮面 |
| Base | `#ffffff` | 输入框/列表底（白） |
| Text / WindowText / ButtonText | `#000000` | |
| Light | `#ffffff` | bevel 高光 |
| Midlight | `#ececec` | 悬停填充 |
| Mid | `#bdbdbd` | bevel 阴影 |
| Dark | `#8a8a8a` | |
| Shadow | `#5a5a5a` | 外边框 |
| Highlight | `#3d7ebb` | KDE 蓝高亮 |
| HighlightedText | `#ffffff` | |

### 色板（由 QPalette 自动推导，不硬编码 RGB）

对按钮色与高亮色做 HSL 明度缩放，生成一小组派生色；按
`(button, highlight)` 颜色哈希缓存：

| 色阶 | 来源 |
|------|------|
| gradientTop    | button×1.18（顶部亮） |
| gradientBottom | button×0.89（底部暗） |
| border         | button×0.63（暗边框） |
| innerTop       | button×1.28（内高光） |
| innerBottom    | button×0.71（内阴影） |
| wellBase       | palette.base()（白色/米色底） |
| highlightTop   | highlight×1.12 |
| highlightBottom| highlight×0.89 |
| highlightBorder| highlight×0.69 |

### 按钮（`PE_PanelButtonCommand/Bevel/Tool`）

- 圆角半径 3，垂直渐变 gradientTop→gradientBottom。
- 按下/选中（Sunken/On）时渐变反转变暗；悬停（MouseOver）整体提亮 105；禁用再提亮。
- 细边框：禁用 border×1.25、默认按钮 border×0.89、否则 border。
- 顶部陶瓷高光横线（innerTop）、左/右内斜线；抬起时底部再画一道内阴影横线。
- `PE_FrameDefaultButton` 不另画（默认按钮由更暗的边框体现）。
- 焦点框：圆角 3 的深色点线框。

### 输入框（`PE_PanelLineEdit` → `drawWell`）

- 圆角 2，base 填充（禁用时 base×0.95）。
- 边框：聚焦时用 highlightBorder，否则 border。
- 凹陷内斜：顶/左 innerBottom（暗）、底/右 palette.light()（亮）。

### 复选框 / 单选钮（13×13）

- 复选框：圆角 3、base 填充（悬停 base×1.04）、border 边框、内部凹陷斜边；
  选中画圆头对勾（ButtonText 色，线宽 1.8），三态画短横线。
- 单选钮：圆形底 + border 边框 + 内层暗/亮弧；选中时中心实心圆点（r≈2.6）。
- 菜单项对勾、ItemView 复选框复用同一套绘制。

### 高亮渐变（`drawHighlightPanel`）

- 垂直渐变 highlightTop→highlightBottom，圆角半径可调（2/3）。
- 激活（按下/悬停/选中）时提亮 105；禁用变暗。
- 细边框 highlightBorder + 顶部高光横线。
- 用于：滑块把手（r3）、滚动条滑块（r2）、进度条内容块（r1）、
  菜单/菜单栏选中项（r3）、ItemView 选中项（r3）。

### 凹槽（`drawGroove`）

- 圆角 1，wellBase×0.94 填充、border×1.18 边框、顶部/左侧内阴影。
- 用于：滚动条轨道、滑块槽、进度条槽。

### 滑块

- 槽：`drawGroove` 凹槽；有焦点时外圈画点线框。
- 把手：高亮渐变面板（r3），21px（`PM_SliderLength`），拖动/悬停时提亮。

### 滚动条

- 宽度 15。两端按钮：按钮面板 + 内缩 3px 矢量箭头（按下时箭头变暗）。
- 轨道：`drawGroove` 凹槽。
- 滑块：高亮渐变面板（r2），最小 24px，悬停/拖动时提亮。

### 组合框

- 不可编辑：整体按钮面板，右侧箭头区先画 Keramik 标志性"水波纹"
  （`drawRipple`：三条微微起伏的竖波浪线），再画下箭头。
- 可编辑：整体凹陷 well，右侧独立下拉按钮（raised，按下时 Sunken），
  内嵌 line edit 无边框填充内部。

### 菜单 / 菜单栏

- 菜单背景：水平渐变（左亮右暗）；菜单边框细线。
- 菜单项：选中画高亮渐变（r3）；勾选项画对勾（带图标时在图标后垫一块
  "按入"小方框）；分隔线为双线（左右内缩 6px）；快捷键右对齐；
  子菜单右箭头 8×8；禁用文字用 Disabled/Text。
- 菜单栏项：选中画高亮渐变，文字用 HighlightedText。
- 菜单栏/工具栏背景：渐变 + 底部细边线。

### 标签页

- 上/下圆角 3；选中项渐变提亮（gradientTop×1.02→gradientBottom×1.12），
  未选项变暗并带顶部高光横线；边框选中 border×1.4 / 未选 border×1.2。

### 表头

- `CE_HeaderSection`：按钮面板 + 底部/右侧分隔细线；表头箭头用 border 色。
- 空表头区：gradientBottom×1.12 填充 + 底部边线。

### 尺寸

| 度量 | 值 |
|------|-----|
| ButtonMargin | 4 |
| ButtonShiftHorizontal / Vertical | 0 / 1 |
| ButtonDefaultIndicator | 0 |
| DefaultFrameWidth | 1 |
| ScrollBarExtent | 15 |
| ScrollBarSliderMin | 24 |
| SliderThickness / SliderLength | 21 |
| SliderControlThickness | 9 |
| Indicator / ExclusiveIndicator | 13×13 |
| MenuButtonIndicator | 16 |
| MenuPanelWidth | 1 |
| MenuBarItemSpacing | 0 |
| TabBarTabHSpace / VSpace | 10 / 10 |
| TabBarTabOverlap | 0 |
| TabBarBaseHeight | 1 |
| SplitterWidth | 6 |
| DockWidgetSeparatorExtent | 4 |
| ProgressBarChunkWidth | 4 |
| TitleBarHeight | 22 |

文本按钮最小 76×26；组合框最小宽 64；LineEdit 高 +6、宽 +4；
工具按钮 +12×+8；菜单项最小高 16。

### 悬停行为

polish 时对按钮、工具按钮、组合框设置 `WA_Hover`；对滚动条、滑块、表头
额外开启鼠标跟踪，使悬停/拖动高亮生效。

## 实现说明

- 继承 `QProxyStyle`，文字排版、图标与所有未覆盖的元素委托基础平台风格。
- 自研绘制全部用 QPainter 矢量（箭头为多边形、对勾为贝塞尔路径、单选钮为
  椭圆弧、波纹为三次样条），不依赖任何位图资源。
- Qt5/Qt6 兼容：复选框/单选钮使用 Qt5/Qt6 均存在的
  `PE_IndicatorCheckBox` / `PE_IndicatorRadioButton`，无需条件编译。
- 目标行数 1200-1500，与仓库内 `winxp` 风格规模相当。
