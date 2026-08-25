# Bluecurve（自研重写）

Bluecurve 是 Red Hat 8/9（2002-2003）的默认桌面主题。本模块以**观感规格**的方式还原它：
只提炼原始外观的颜色、几何与绘制顺序，代码全部自研（LGPL），不包含 Red Hat
原版 Qt 3 风格或任何社区移植版的代码。

## 观感规格

### 经典配色（`standardPalette()`，建议值）

Red Hat 8/9 时代的经典 Bluecurve 调色板由 `standardPalette()` 提供，是
**建议值**：Qt 不会自动采用，本样式也不会强制应用；应用可调用
`QApplication::setPalette(style->standardPalette())` 呈现经典观感。默认绘制
仍从当前 QPalette 自动推导（见下文色阶表）。

返回的调色板填满全部 `QPalette::ColorRole`（Disabled 组由
`lib/qtstyles_palette.h` 的共享辅助统一派生），因此完全自足、不继承调用时
的 app 调色板——运行时切换样式不会串入上一个样式的配色。

| 角色 | 值 | 说明 |
|------|-----|------|
| Window | `#e6e6e6` | 浅灰窗口底色 |
| Button | `#d9d9d9` | 浅米按钮面 |
| Base | `#ffffff` | 输入框/列表底（白） |
| Text / WindowText / ButtonText | `#000000` | |
| Light | `#ffffff` | bevel 高光 |
| Midlight | `#e2e2e2` | 悬停填充 |
| Mid | `#c0c0c0` | bevel 阴影 |
| Dark | `#a0a0a0` | |
| Shadow | `#808080` | 外边框 |
| Highlight | `#7590ae` | GNOME 2 蓝色高亮（Blue Medium） |
| HighlightedText | `#ffffff` | |

### 色板（由 QPalette 自动推导，不硬编码 RGB）

对按钮色做 HSL 明度/饱和度缩放生成 8 级 bevel 色阶：

| 色阶 | 因子 |
|------|------|
| shades[0] | 1.065 |
| shades[1] | 0.963 |
| shades[2] | 0.896 |
| shades[3] | 0.850 |
| shades[4] | 0.768 |
| shades[5] | 0.665 |
| shades[6] | 0.400 |
| shades[7] | 0.205 |

对高亮色生成 3 个渐变点色：spots = {highlight×1.62, ×1.05, ×0.72}。
按 `(button, highlight)` 颜色哈希缓存派生数据。

### 按钮（`PE_PanelButtonCommand/Bevel/Tool`）

- 外框：shades[6]（暗边框）。
- 双线高光：Raised 时左上为 `Qt::white`、右下为 shades[2]；Sunken/On 时反转。
- 填充（内缩 2px）：Sunken→Mid，MouseOver→Midlight，On→Mid，普通→Button；
  Flat 按钮不画边框与填充。
- Default 按钮在外圈再加一道黑框。

### 输入框（`PE_PanelLineEdit`）

凹槽（sunken well）：shades[5] 外框 + 内层左上 shades[1] / 右下 palette.light()，
填充 palette.base()（白色）。

### 高亮渐变（`drawGradientBox`）

从 `highlight×shade1` 到 `highlight×shade2` 逐列插值的渐变块，外圈用 spots
画 3D 边框（spots[2] 外框、spots[1] 右下、spots[0] 左上）。
用于：菜单选中项（0.9→1.2）、菜单栏按下项（0.9→1.2）、进度条内容（0.92→1.66）。

### 复选框 / 单选钮（13×13）

- 复选框：白色底、shades[5] 外框、内层左上线 shades[1] / 右下线 shades[2]；
  选中画对勾（前景 Text 色，圆头粗线——画在白底上需用黑色，不能白色）；
  三态画高亮短横线。
- 单选钮：白色圆 + shades[5] 外圈 + 左上高光弧 / 右下暗弧；选中时中心实心圆
  （highlight 色，半径约 1/3）。
- 鼠标悬停时整行背景填充 Midlight（`CE_CheckBox`/`CE_RadioButton`）。

### 滚动条

- 宽度 15。按钮：`drawLightBevel` + 内缩 3px 箭头（悬停填充 Midlight）。
- 槽：shades[3] 填充 + shades[5] 上下/左右边线。
- 滑块：`drawLightBevel` + 中央三组斜纹（shades[5] 上 / white 下）；
  宽/高不足 31 时不画斜纹。

### 滑块

- 槽：5px 高凹槽，shades[5] 框 + palette.mid() 填充 + shades[4] 内左上线。
- 把手：31px 圆角方块，shades[6] 外框 + 白色顶左高光 + 按钮色填充（悬停 Midlight）
  + 中央三组斜纹。

### 组合框

- Frame：`drawLightBevel`（悬停 Midlight）。
- 箭头区：9×8 下箭头 + 下方两条短横线（shades[3]）。
- 编辑区右侧分隔线：shades[4] 竖线 + light 竖线（可编辑时外扩 1px）。

### 菜单 / 菜单栏

- 菜单项：选中且启用画高亮渐变（0.9→1.2）；否则按钮色填充。
  分隔线：shades[2] 上 + light 下（左右内缩 6px）。子菜单箭头 8×9。
  加速键右对齐。禁用文字用 text 色 + 1px 浅色偏移（etch）。
- 菜单栏：项按下时画高亮渐变、文字用 HighlightedText；其余按钮色填充。
- 菜单栏/工具栏背景：按钮色填充 + 底部 shades[3] 边线。

### Tab

- 选中 tab：按钮色填充 + 高亮渐变边框，未选中 tab 按钮色填充，重叠区 1px。
- 垂直 tab（West/East）旋转绘制：先按 QCommonStyle 的 tabLayout 约定平移并
  旋转坐标系，再把文字矩形归零并交换宽高；若沿用带偏移的 tab->rect 画文字，
  文字会被平移到 tab 之外裁剪掉。

### 进度条

- 槽：shades[3] 填充 + shades[5] 外框。
- 内容：`drawGradientBox(0.92, 1.66)`；busy 模式为条宽 1/3（最小 25px）的渐变块，
  按 2400ms 一个来回时钟驱动滑动（Qt 6 的 indeterminate 不更新 `progress` 字段）。
- 文字：块内用 HighlightedText（白）、块外用 Text（黑），分别 clip 到/排除填充块，
  跨块边界时自动分段着色。垂直条按 QProgressBar 方向把文字旋转 90° 后垂直居中。

### 尺寸

| 度量 | 值 |
|------|-----|
| Indicator / ExclusiveIndicator | 13×13 |
| ScrollBarExtent | 15 |
| ScrollBarSliderMin | 31 |
| SliderLength | 31 |
| ButtonMargin | 10 |
| ButtonDefaultIndicator | 0 |
| DefaultFrameWidth | 1 |
| SplitterWidth / DockWidgetSeparatorExtent | 6 |
| TabBarTabHSpace / VSpace | 11 / 13 |
| TabBarTabOverlap | 1 |
| ProgressBarChunkWidth | 2 |
| MenuPanelWidth | 3 |

按钮最小 85×30（无图标时）；菜单项最小高 16；工具按钮最小 32×32；
组合框最小高 27；SpinBox 最小高 25；滑块最小 17。

### 悬停行为

polish 时对按钮、组合框、分割条设置 `WA_Hover`；对滚动条、滑块额外开启
鼠标跟踪，使悬停高亮生效。

## 实现说明

- 继承 `QCommonStyle`，未覆盖的元素委托基类。
- 自研绘制全部用 QPainter 矢量（箭头为多边形、对勾为路径、单选钮为椭圆弧），
  不依赖任何位图资源。
- 目标行数约 1100-1400，与仓库内 `winxp` 风格规模相当。
