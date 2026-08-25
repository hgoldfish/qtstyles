# Platinum（自研重写）

Platinum 是经典 Macintosh 系统软件（Mac OS 8/9）的界面外观：暖米色/灰白色调、
方形带角点切角的命令按钮、13×13 复选/单选指示器、7px 深槽滑块与带横纹（riffles）
装饰的六边形把手、方形滚动条按钮与三横线箭头。
本模块以**观感规格**的方式还原它：视觉规格提炼自 Qt 3 的 `QPlatinumStyle` 外观，
代码全部自研（LGPL），不包含原版或任何社区移植版的代码。

## 观感规格

### 经典配色（`standardPalette()`，建议值）

整体为暖米灰调，高亮/链接沿用经典的 Mac 深海军蓝。这套配色由
`standardPalette()` 提供，是**建议值**：Qt 不会自动采用（平台主题调色板优先），
本样式也不会强制应用；应用可调用 `QApplication::setPalette(style->standardPalette())`
来呈现经典观感。

| 角色 | 值 | 说明 |
|------|-----|------|
| Window | `#d4d0c6` | 窗口底色（暖米灰） |
| WindowText | `#1a1a1a` | |
| Base | `#fbfaf6` | 输入框/指示器底色（近白） |
| AlternateBase | `#e8e4d8` | |
| Button | `#dcd8cc` | 按钮/面板面 |
| ButtonText | `#000000` | |
| BrightText | `#ffffff` | 按下按钮上的文字 |
| Light | `#f2efe6` | bevel 高光 |
| Midlight | `#e7e3d7` | |
| Mid | `#b8b3a5` | bevel 阴影、按下填充 |
| Dark | `#8f8a7c` | 按下填充、槽 |
| Shadow | `#57534a` | 外边框 |
| Highlight | `#00007b` | 选中/进度条内容（Mac 深海军蓝） |
| HighlightedText | `#ffffff` | |
| Link / LinkVisited | `#00007b` / `#551a8b` | Mac 深海军蓝链接 / 访问后紫 |

返回的调色板填满全部 `QPalette::ColorRole`（Disabled 组由
`lib/qtstyles_palette.h` 的共享辅助统一派生），因此完全自足、不继承调用时
的 app 调色板——运行时切换样式不会串入上一个样式的配色。

### 按钮（`CE_PushButton` / `PE_PanelButtonCommand`）

- **命令按钮**（普通 QPushButton，`drawCommandButton`）：方形面 + 4px 圆角切角
  （四角用背景色点阵"切"出圆角），由多层 1px 线框构成：
  `Shadow` 外环 → `Button` → `Light`（左上高光）→ `Mid`/`Dark`（右下阴影）
  → `Shadow` 最外缘，角点处按背景/阴影/按钮/白逐层叠点。
- **bevel 按钮**（toggle 按钮与小型图标按钮，`drawBevel`）：小面
  （`w×h < 1600` 或强非方形）用 2px 明暗阶梯线框；大面用 3px 阶梯线框并在
  左下/右上角画 `mixedColor` 混合色角点。
- 状态：按下/选中时命令按钮填充 `Dark` 并翻转文字为 `BrightText`；
  切换按钮（toggle）按下填充 `Dark`、选中填充 `Mid` 的 `Dense4Pattern` 点阵面；
  工具按钮（`PE_PanelButtonTool`）按下/选中填充 `Dark`（face 不变色）。
- **默认按钮**（Default）：先画一圈内缩 1px 的外层环（命令按钮用 `Mid` 面），
  再内缩 `PM_ButtonDefaultIndicator` = 3px 画内层面（总缩进 4px）。
  对话框中的 autoDefault 按钮同样内缩 3px（不画外层环）。
- 带菜单的按钮：箭头按钮左侧画 `Mid`/`Button`/`Light` 三色竖分隔线
  （按下时隐藏），箭头为三横线样式。
- Flat 按钮：不画面板，仅文字。
- 按下时标签不位移（`PM_ButtonShiftHorizontal/Vertical` = 0），压暗反馈即可。

### 复选 / 单选指示器

- 复选框（`PE_IndicatorCheckBox`，15×13）：左侧小 bevel（宽 = 指示器宽 − 2，
  即 13px），右侧 2px 填 `Window` 色，再套 1px `Shadow` 框；选中画 Qt 3 点阵列对勾
  （`Text` 色主体 + 右下 `Dark` 阴影各一遍，共 17 段），三态画两条
  2px 高短横线；按下时 bevel 凹陷、对勾向右下偏移 1px。
- 单选钮（`PE_IndicatorRadioButton`，15×15）：整个盒子擦为 `Window` 色，
  画 13×13 无抗锯齿圆（按 `Button`/`Dark` 填充，`Shadow` 色 28 点折线勾勒
  圆周），顶部 7 段亮弧（Raised 用 `Light`，Sunken/On 用 `Dark.darker()`）、
  底部右侧 6~7 段暗弧（Raised 用 `Dark`，Sunken/On 用 `Light`）；
  选中画 `Text` 色八边形实心点 + 4 条 `Dark` 延伸短斜线。
- 全部使用整数坐标、关闭抗锯齿，与 Qt 3 的逐点光栅化一致。

### 滑块（`CC_Slider`）

- 槽：先填 `Window` 色，再画 **7px 深槽**（`Dark` 填充）：左上 `Dark`/`Shadow`
  内阴线、右下 `Light` 亮线、四角背景/阴影/亮色角点，中央再加 1px `Dark`
  角点；槽的位置按刻度偏移 `len/8`。
- 把手：17px 长、`PM_SliderControlThickness` 高的**六边形**把手：`Button` 面填充、
  `Shadow` 六边轮廓 + 左侧/顶部 `Light` 高光 + 右侧/底部 `Dark` 阴影；
  中央画一组 **riffles**（每 4px 一组 Light/Dark 平行线，居中、上限 20px）。
- `PM_SliderControlThickness` 按 Qt 3 QWindowsStyle 公式动态计算：无刻度时
  等于控件高度；有刻度时 `6px` 起步再加 `(PM_SliderLength / 4)`（单侧）
  并按剩余空间分摊。
- 聚焦时在槽内画虚线焦点框。

### 滚动条（`CE_ScrollBar*`）

- 两端按钮：方形 bevel（`Mid` 面）+ 1px `Shadow` 外框 + 内缩 4px 的三横线箭头。
- 页区（Add/SubPage）：`Mid` 填充的凹陷槽，横条为上下明暗边、竖条为左右明暗边。
- 滑块：`Button` 面 bevel + 中央 riffles（水平条为竖线、垂直条为横线）+
  `Shadow` 外框；聚焦时画内缩 2px 虚线焦点框。
- 布局：两个箭头按钮并排置于轨道尾端（Qt 3 排布）；轨道过短时按钮宽/高取
  `min(长度/2, extent)`，两按钮均分尾端；`PM_ScrollBarSliderMin` = 25。

### 组合框（`CC_ComboBox`）

- 面板：与命令按钮同款方形 bevel 面板（`Button` 面、`Shadow` 外框、
  `Light` 左上、`Mid` 右下、角点切角）。
- 箭头按钮：固定 20px 宽（`SC_ComboBoxArrow`），与面板共用一套 bevel 阶梯线，
  左侧 `Mid` 分隔线，中央画**上下两组三横线箭头**：上箭头中心在按钮中心
  上方 3px（箭杆 7→5→3px 逐层收窄、尖端 1px 点），下箭头中心在下方 2px
  （箭杆 3→5→7px 展开、尖端 1px 点）。
- 不可编辑：面板内 `Window` 底 + 文字；聚焦时填充 `Highlight`、文字转
  `HighlightedText`，并在 `SE_ComboBoxFocusRect`（内缩 4px、右侧让 16px）画
  虚线焦点框。
- 可编辑：编辑区画 Qt 3 `qDrawShadePanel` 凹陷面板——2px 深、
  `Dark` 上/左 + `Light` 下/右的阶梯边框。

### 进度条（`CE_ProgressBarContents` / `PE_IndicatorProgressChunk`）

- 普通进度条：按 Qt 3 QCommonStyle 分块——`PM_ProgressBarChunkWidth` = 9px，
  每块为 `Highlight` 填充（上下各内缩 3px、右侧留 2px），块间露出槽底
  2px 背景缝，形成经典"方块格子"效果。
- busy（不确定，范围 0..0）：一条 4px 宽 `Highlight` 竖线在槽内来回扫动
  （Qt 3 CE_ProgressBarContents 行为），而非 Qt 6 基类的滚动方块动画；
  扫动相位由单调钟算出，样式通过 QTimer 事件过滤器跟踪可见的 busy 条并
  驱动重绘，切换回确定范围时停止。垂直进度条因
  Qt 3 仅有水平条，按其旋转画布后的语义表现为 4px 高横线沿垂直方向
  上下扫动。

### 箭头（`PE_IndicatorArrowUp/Down/Left/Right`、滚动条、菜单）

Qt 3 QWindowsStyle 的点阵列箭头：以区域中心为原点画 7 个点，
前 3 段水平/垂直线条为箭杆、第 7 点为箭头尖端；滚动条与菜单按钮箭头同款。
中心按 Qt 3 闭区间语义取 `x() + width()/2`（`QRect::center()` 在 Qt 6 用
`(width()-1)/2`，偶数宽区域会整体左移 1px）。

### 焦点框（`PE_FrameFocusRect`）

`Text` 色 1px 虚线矩形；按钮聚焦画在 `SE_PushButtonFocusRect`，滑块聚焦
画在把手内缩 2px 处。

### Tab 栏（`CE_TabBarTabShape` / `PE_FrameTabBarBase`）

复刻 Qt 3 QWindowsStyle 的 `CE_TabBarTab` 细线轮廓风格，替代 Qt 6 基类的
圆角渐变凸起面板：

- 每个 tab 用 `Light`/`Midlight`/`Dark`/`Shadow` 四色 1px 线勾边（左上高光、
  右下阴影），内部不填充（透出 tab bar 的 `Window` 底），选中与未选中 tab
  高度一致、无凸起位移。
- 底边（上置 tab）由每个 tab 自画两条 `Midlight`+`Light` 基线；选中 tab 用
  `Window` 色覆盖自身底边 2px 并涂掉左边缘下段，使选中 tab 与下方内容面板
  无缝融合。`PE_FrameTabBarBase` 置空，避免与基线叠加成双层边框。
- West/East 位置（Qt 3 无此形状）按 North 逻辑旋转 90° 绘制，基线随之落在
  外侧边缘，保持同一套扁平风格。


### 尺寸

| 度量 | 值 |
|------|-----|
| PM_ButtonDefaultIndicator | 3 |
| PM_ButtonShiftHorizontal / Vertical | 0 |
| PM_IndicatorWidth / Height | 15 / 13 |
| PM_ExclusiveIndicatorWidth / Height | 15 |
| PM_SliderLength | 17 |
| PM_SliderControlThickness | 动态（见滑块一节） |
| PM_ScrollBarSliderMin | 25 |
| PM_MaximumDragDistance | -1 |

### 悬停行为

polish 时对按钮、滑块、滚动条、组合框开启 `WA_Hover`，使悬停高亮生效；
unpolish 时恢复。

## 已知限制

- Tab 栏已按 Qt 3 QWindowsStyle 的细线轮廓重绘（见上）；Qt 3 没有 West/East
  位置的 tab，这两个方向用同一套逻辑旋转近似。
- 菜单栏沿用基类（Windows）外观，未做 Qt 3 式定制。
- 禁用状态下的箭头仍为单色（`ButtonText` 的 Disabled 变体），而 Qt 3
  会额外画 `Light` 偏移影。

## 实现说明

- 继承 `QProxyStyle`，base style 用 `QStyleFactory::create("windows")`；
  未覆盖的元素自动委托基类，保持经典外观的一致性。
- 全部自研绘制均为 QPainter 直线/点/折线（对齐 Qt 3 的逐点光栅化），
  无任何位图资源、不开抗锯齿。
- 构建：目录内 `CMakeLists.txt`（`qt_add_plugin`，Qt 6）与 `platinum.pro`
  （qmake，Qt 5/6）双轨；插件 key 为 `"platinum"`。
