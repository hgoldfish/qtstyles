# qtstyles 主题图鉴（-classic 系列）

本文档用 [preview](../preview/README.md) 程序展示 qtstyles 每个 `-classic` 主题的完整外观。
`-classic` 键会在样式安装时通过 `polish(QPalette&)` **强制**套用该主题的经典配色，
不受宿主主题影响，因此下面的截图即每个主题的真实经典观感。

每个主题展示 preview 程序的 8 个页面：

| 页面 | 包含的控件 |
| --- | --- |
| Buttons | QPushButton / QToolButton / QCheckBox / QRadioButton |
| Inputs | QLineEdit / QComboBox / QFontComboBox / QSpinBox / QDateTimeEdit / QDial / QSlider |
| Lists && Trees | QListWidget / QTreeWidget / QTableWidget / QListView / QColumnView |
| Progress && Sliders | QProgressBar / QSlider / QScrollBar / QSplitter |
| Tabs && Panels | QTabWidget / QToolBox / QStackedWidget |
| Text | QLabel / QTextEdit / QPlainTextEdit / QTextBrowser |
| Frames && Groups | QGroupBox / QFrame / QScrollArea |
| Misc | QCalendarWidget / QLCDNumber / QDialogButtonBox / QProgressBar |

## 如何更新截图

更新样式代码后，重新构建 preview 并重跑截图脚本即可：

```sh
# 从仓库根目录
cmake --build build-qt6 --target preview -j$(nproc)
python3 docs/screenshots.py --build
```

脚本会用 offscreen 平台（无需显示器）逐个运行 preview 程序，
把每个 `-classic` 主题 × 每个页面的截图写入 `docs/screenshots/<style>/<tab>.png`。
详见 [screenshots.py](screenshots.py)。

---

## bluecurve-classic

Red Hat 8/9（2002）的默认桌面主题，GNOME 2 时代的经典外观。
浅灰/浅米按钮面配上 1 像素高光描边，输入框为凹陷白底 well，
选中项与进度条用 GNOME 2「Blue Medium」（`#7590ae`）蓝色渐变，
复选/单选指示器为 13×13，滑块把手机为 31px 圆角方块并带斜纹。
本实现是**自研重写**（LGPL），基于 `QCommonStyle`，全部用 QPainter 矢量绘制。

经典配色：窗口 `#e6e6e6`，按钮 `#d9d9d9`，高亮 `#7590ae`。

<img src="screenshots/bluecurve-classic/buttons.png" alt="bluecurve-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/bluecurve-classic/inputs.png" alt="bluecurve-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/bluecurve-classic/lists-trees.png" alt="bluecurve-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/bluecurve-classic/progress-sliders.png" alt="bluecurve-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/bluecurve-classic/tabs-panels.png" alt="bluecurve-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/bluecurve-classic/text.png" alt="bluecurve-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/bluecurve-classic/frames-groups.png" alt="bluecurve-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/bluecurve-classic/misc.png" alt="bluecurve-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## dirtylooks-classic

对应 Qt 的 **Cleanlooks**（KDE 4 早期默认主题之一）。
暖米白底色（`#efebe7`）、蓝色高亮（`#628cb4`），按钮与输入框采用平滑渐变与细边线，
基于 `QProxyStyle`，未覆盖的控件委托 Windows 基础样式；缓存 pixmap 带设备像素比，
HiDPI 下保持清晰。

经典配色：背景 `#efebe7`，高亮 `#628cb4`。

<img src="screenshots/dirtylooks-classic/buttons.png" alt="dirtylooks-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/dirtylooks-classic/inputs.png" alt="dirtylooks-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/dirtylooks-classic/lists-trees.png" alt="dirtylooks-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/dirtylooks-classic/progress-sliders.png" alt="dirtylooks-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/dirtylooks-classic/tabs-panels.png" alt="dirtylooks-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/dirtylooks-classic/text.png" alt="dirtylooks-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/dirtylooks-classic/frames-groups.png" alt="dirtylooks-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/dirtylooks-classic/misc.png" alt="dirtylooks-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## highschool-classic

在 Motif 绘制基础上叠加 **SGI / Irix** 标志性元素：米灰配色
（窗口 `#c9beb1`、暖纸白 Base）与红色勾选——复选框画红色对勾、
菱形单选钮中心画红色实心圆点；其余控件保持经典 Motif 绘制与度量，
Motif 式反白选中（选中项反色）。继承 `OldschoolStyle`。

经典配色：米灰窗口 `#c9beb1`，红勾、菱形单选钮。

<img src="screenshots/highschool-classic/buttons.png" alt="highschool-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮（注意红色对勾与红点）*

<img src="screenshots/highschool-classic/inputs.png" alt="highschool-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/highschool-classic/lists-trees.png" alt="highschool-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/highschool-classic/progress-sliders.png" alt="highschool-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/highschool-classic/tabs-panels.png" alt="highschool-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/highschool-classic/text.png" alt="highschool-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/highschool-classic/frames-groups.png" alt="highschool-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/highschool-classic/misc.png" alt="highschool-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## keramik-classic

KDE 3 / TDE（2002 年左右）的**陶瓷**主题：平滑斜角渐变、圆角按钮（半径 3）、
细边框、凹陷的圆角输入 well，以及用高亮色突出的滑块/滚动条把手；
组合框箭头旁有 Keramik 标志性的「水波纹」，分组框为双线陶瓷框。
本实现是**自研重写**（LGPL），基于 `QProxyStyle`，全部矢量绘制，
并覆盖工具盒、停靠窗标题、工具提示、树分支、尺寸柄与 busy 进度条等现代控件。

经典配色：窗口 `#dfdfdf`，按钮 `#d8d8d8`，高亮 `#3d7ebb`（KDE 蓝）。

<img src="screenshots/keramik-classic/buttons.png" alt="keramik-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/keramik-classic/inputs.png" alt="keramik-classic — Inputs" width="820">
*Inputs：输入框、组合框（注意水波纹箭头）、微调框、拨盘与滑块*

<img src="screenshots/keramik-classic/lists-trees.png" alt="keramik-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/keramik-classic/progress-sliders.png" alt="keramik-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/keramik-classic/tabs-panels.png" alt="keramik-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/keramik-classic/text.png" alt="keramik-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/keramik-classic/frames-groups.png" alt="keramik-classic — Frames && Groups" width="820">
*Frames && Groups：分组框（双线陶瓷框）、各种框架形状与滚动区域*

<img src="screenshots/keramik-classic/misc.png" alt="keramik-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## newschool-classic

**CDE**（Common Desktop Environment）观感，继承 `OldschoolStyle` 的 Motif 度量：
斜边面板、反白选中保持 Motif 语义，配色换成 CDE 的淡紫灰背景
（`#b6b6cf`）与海军蓝高亮（`#000080`），并带 CDE 风格的标准图标。

经典配色：背景 `#b6b6cf`，高亮 `#000080`。

<img src="screenshots/newschool-classic/buttons.png" alt="newschool-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/newschool-classic/inputs.png" alt="newschool-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/newschool-classic/lists-trees.png" alt="newschool-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/newschool-classic/progress-sliders.png" alt="newschool-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/newschool-classic/tabs-panels.png" alt="newschool-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/newschool-classic/text.png" alt="newschool-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/newschool-classic/frames-groups.png" alt="newschool-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/newschool-classic/misc.png" alt="newschool-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## oldschool-classic

经典的 **Motif** 观感，源自 Qt `QMotifStyle`：斜边面板、默认按钮外圈指示、
三态复选框、反白选中（选中项反色）、Motif 点阵箭头与滚动条、粗菜单项度量。
纯 Motif 绘制，不含 SGI 配色与红色勾选（那是 `highschool`）。

经典配色：经典 Motif 米灰面板 + 反白选中。

<img src="screenshots/oldschool-classic/buttons.png" alt="oldschool-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/oldschool-classic/inputs.png" alt="oldschool-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/oldschool-classic/lists-trees.png" alt="oldschool-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/oldschool-classic/progress-sliders.png" alt="oldschool-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/oldschool-classic/tabs-panels.png" alt="oldschool-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/oldschool-classic/text.png" alt="oldschool-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/oldschool-classic/frames-groups.png" alt="oldschool-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/oldschool-classic/misc.png" alt="oldschool-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## phase-classic

**Phase** 风格（David Johnson 的原 Qt 4 / KDE 样式，MIT 许可）：平滑圆角按钮、
蓝灰渐变与玻璃质感。基于 `QProxyStyle`，布局与未重画的控件委托
**Windows** 基础样式（`QStyleFactory::create("Windows")`），与 Qt 4 原版一致。
自带 busy 进度条动画。

经典配色：窗口 `#eeeeee`，按钮 `#dddde3`，高亮 `#6090c0`。

<img src="screenshots/phase-classic/buttons.png" alt="phase-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/phase-classic/inputs.png" alt="phase-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/phase-classic/lists-trees.png" alt="phase-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/phase-classic/progress-sliders.png" alt="phase-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/phase-classic/tabs-panels.png" alt="phase-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/phase-classic/text.png" alt="phase-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/phase-classic/frames-groups.png" alt="phase-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/phase-classic/misc.png" alt="phase-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## plastic-classic

对应 Qt 的 **Plastique**：光滑的「塑料」质感按钮——渐变填充、圆角、
亮边高光；输入框与列表采用柔和的凹陷 well。基于 `QProxyStyle`，
缓存 pixmap 带设备像素比，HiDPI 下保持清晰。

经典配色：经典 Plastique 蓝灰调。

<img src="screenshots/plastic-classic/buttons.png" alt="plastic-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/plastic-classic/inputs.png" alt="plastic-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/plastic-classic/lists-trees.png" alt="plastic-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/plastic-classic/progress-sliders.png" alt="plastic-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/plastic-classic/tabs-panels.png" alt="plastic-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/plastic-classic/text.png" alt="plastic-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/plastic-classic/frames-groups.png" alt="plastic-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/plastic-classic/misc.png" alt="plastic-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## platinum-classic

经典 Macintosh（Mac OS 8/9）**Platinum** 外观：暖米灰调
（窗口 `#d4d0c6`）、方形带角点切角的命令按钮、15×13 复选 / 15×15 单选指示器、
7px 深槽滑块与**强调色**六边形把手（riffles）、强调色滚动条滑块与实心黑箭头，
进度条为 Qt 3 式 9px「方块格子」，停靠窗标题带 racing stripes、grow box、
disclosure triangle 与 primary group box；列表表头为扁平单像素边框与实心排序三角，
禁用箭头带 Light 蚀刻影。本实现是**自研重写**（LGPL），
基于 `QProxyStyle`（windows 基础样式），绘制全部对齐 Qt 3 的逐点光栅化、不开抗锯齿。

经典配色：窗口 `#d4d0c6`（暖米灰），高亮 `#00007b`（Mac 深海军蓝）。

<img src="screenshots/platinum-classic/buttons.png" alt="platinum-classic — Buttons" width="820">
*Buttons：切角命令按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/platinum-classic/inputs.png" alt="platinum-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/platinum-classic/lists-trees.png" alt="platinum-classic — Lists && Trees" width="820">
*Lists && Trees：扁平表头、列表、树、表格与多列视图*

<img src="screenshots/platinum-classic/progress-sliders.png" alt="platinum-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条、六边形滑块把手、滚动条与分割器*

<img src="screenshots/platinum-classic/tabs-panels.png" alt="platinum-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页（细线轮廓）、工具盒与堆叠页*

<img src="screenshots/platinum-classic/text.png" alt="platinum-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/platinum-classic/frames-groups.png" alt="platinum-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/platinum-classic/misc.png" alt="platinum-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## winxp-classic

默认 **Windows XP Luna（Blue）** 属性页外观：米色面板 `#ece9d8`、高亮
`#316ac5`，安装时通过 `polish(QPalette&)` **强制**套用该配色。
对照真机属性对话框：选中标签顶部橙色细线、圆角米色按钮（默认钮深蓝描边）、
分组框蓝标题、白底菜单（无 Office 左侧彩条）。工具栏 / dock 标题 / 工具盒
用 Luna Rebar 米色（`#f1f3ef`→`#ece9d8`），不再用 `winxp-blue` 的 Office
糖果蓝条。控件绘制复用 Luna Blue 的箭头/滚动条/输入框色，主客户区为平涂米色。

<img src="screenshots/winxp-classic/buttons.png" alt="winxp-classic — Buttons" width="820">
*Buttons：按钮、工具栏按钮、复选框与单选钮*

<img src="screenshots/winxp-classic/inputs.png" alt="winxp-classic — Inputs" width="820">
*Inputs：输入框、组合框、微调框、拨盘与滑块*

<img src="screenshots/winxp-classic/lists-trees.png" alt="winxp-classic — Lists && Trees" width="820">
*Lists && Trees：列表、树、表格与多列视图*

<img src="screenshots/winxp-classic/progress-sliders.png" alt="winxp-classic — Progress && Sliders" width="820">
*Progress && Sliders：进度条（含 busy 动画）、滑块、滚动条与分割器*

<img src="screenshots/winxp-classic/tabs-panels.png" alt="winxp-classic — Tabs && Panels" width="820">
*Tabs && Panels：标签页、工具盒与堆叠页*

<img src="screenshots/winxp-classic/text.png" alt="winxp-classic — Text" width="820">
*Text：富文本标签与编辑器*

<img src="screenshots/winxp-classic/frames-groups.png" alt="winxp-classic — Frames && Groups" width="820">
*Frames && Groups：分组框、各种框架形状与滚动区域*

<img src="screenshots/winxp-classic/misc.png" alt="winxp-classic — Misc" width="820">
*Misc：日历、LCD 数字与标准对话框按钮*

---

## 其它变体

仓库还提供每个主题的无后缀键（如 `bluecurve`、`keramik`），它们**不强制**经典配色，
外观跟随宿主主题调色板；`winxp-blue` / `winxp-silver` / `winxp-olive` 则是
固定 Windows XP Luna 配色的变体。本文档只收录 `-classic` 系列。
