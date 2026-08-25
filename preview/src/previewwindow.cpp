/****************************************************************************
**
** Copyright (C) 2026 Qize Huang <hgoldfish@gmail.com>
**
** This file is part of qtstyles, a collection of retro Qt widget styles.
**
** This library is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library.  If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "previewwindow.h"

#include <QtWidgets>
#include <QApplication>
#include <QStyleFactory>
#include <QThread>

#include "bluecurvestyle.h"
#include "dirtylooksstyle.h"
#include "highschoolstyle.h"
#include "keramikstyle.h"
#include "oldschoolstyle.h"
#include "newschoolstyle.h"
#include "platinumstyle.h"
#include "plasticstyle.h"
#include "phasestyle.h"
#include "winxpstyle.h"

// 单一来源：本仓库内置样式的名字。构造函数的样式列表与 createStyle()
// 的分派都以此为准（if 链的顺序与名字索引一一对应）。
static const char *const kBuiltinStyleNames[] = {
    "dirtylooks", "oldschool", "newschool", "highschool", "plastic", "phase",
    "winxp", "winxp-blue", "winxp-silver", "winxp-olive",
    "bluecurve", "keramik", "platinum"
};

static bool isBuiltinStyleName(const QString &name)
{
    for (const char *builtin : kBuiltinStyleNames) {
        if (name.compare(QLatin1String(builtin), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

// winxp-blue / winxp-silver / winxp-olive 的 Luna 配色由 polish(QPalette&)
// 在样式安装时强制应用，不受「应用经典配色」开关影响；否则用
// standardPalette()（Win2000 建议色）覆盖会冲掉 Luna。
static bool isLunaVariant(const QString &name)
{
    return name.compare(QLatin1String("winxp-blue"), Qt::CaseInsensitive) == 0
        || name.compare(QLatin1String("winxp-silver"), Qt::CaseInsensitive) == 0
        || name.compare(QLatin1String("winxp-olive"), Qt::CaseInsensitive) == 0;
}

QStyle *createStyle(const QString &name)
{
    if (name.compare(QLatin1String(kBuiltinStyleNames[0]), Qt::CaseInsensitive) == 0)
        return new DirtylooksStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[1]), Qt::CaseInsensitive) == 0)
        return new OldschoolStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[2]), Qt::CaseInsensitive) == 0)
        return new NewschoolStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[3]), Qt::CaseInsensitive) == 0)
        return new HighschoolStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[4]), Qt::CaseInsensitive) == 0)
        return new PlasticStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[5]), Qt::CaseInsensitive) == 0)
        return new PhaseStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[6]), Qt::CaseInsensitive) == 0)
        return new WinXPStyle(WinXPStyle::Classic);
    if (name.compare(QLatin1String(kBuiltinStyleNames[7]), Qt::CaseInsensitive) == 0)
        return new WinXPStyle(WinXPStyle::Blue);
    if (name.compare(QLatin1String(kBuiltinStyleNames[8]), Qt::CaseInsensitive) == 0)
        return new WinXPStyle(WinXPStyle::Silver);
    if (name.compare(QLatin1String(kBuiltinStyleNames[9]), Qt::CaseInsensitive) == 0)
        return new WinXPStyle(WinXPStyle::Olive);
    if (name.compare(QLatin1String(kBuiltinStyleNames[10]), Qt::CaseInsensitive) == 0)
        return new BluecurveStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[11]), Qt::CaseInsensitive) == 0)
        return new KeramikStyle;
    if (name.compare(QLatin1String(kBuiltinStyleNames[12]), Qt::CaseInsensitive) == 0)
        return new PlatinumStyle;
    return QStyleFactory::create(name);
}

static QWidget *scrollable(QWidget *content)
{
    QScrollArea *area = new QScrollArea;
    area->setWidget(content);
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    return area;
}

static QLabel *makePageLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setFrameShape(QFrame::StyledPanel);
    return label;
}

PreviewWindow::PreviewWindow(const QString &initialStyle, int initialTab, QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("qtstyles — Widget Preview"));
    setMinimumSize(960, 680);
    resize(1280, 860);

    initialTab_ = initialTab;

    // 记住启动时的默认调色板：切换样式时以此作为恢复基线。
    initialPalette_ = QApplication::palette();

    // 可用样式：本项目内置的几种 + 系统自带样式
    const QStringList systemKeys = QStyleFactory::keys();
    for (const char *name : kBuiltinStyleNames)
        styles_ << QLatin1String(name);
    for (const QString &key : systemKeys) {
        if (!isBuiltinStyleName(key))
            styles_ << key;
    }

    buildToolBar();
    buildDocks();
    buildMenus();
    buildCentral();
    buildStatusBar();

    int index = 0;
    for (int i = 0; i < styles_.size(); ++i) {
        if (styles_.at(i).compare(initialStyle, Qt::CaseInsensitive) == 0) {
            index = i;
            break;
        }
    }
    styleCombo_->setCurrentIndex(index);
    applyStyle(styleCombo_->currentText());
}

void PreviewWindow::applyStyle(const QString &name)
{
    QStyle *style = createStyle(name);
    if (!style)
        return;

    // QApplication::setStyle() 不会自动采用 QStyle::standardPalette()
    //（平台主题的调色板优先），内置样式各自定义的配色必须显式应用。
    // 先恢复启动时的默认调色板，否则基于 QProxyStyle 的样式（如
    // dirtylooks、plastic、phase）会从上一个样式的调色板出发产生串扰。
    qApp->setPalette(initialPalette_);
    qApp->setStyle(style);
    // 勾选「应用经典配色」时才套用 standardPalette()，否则保持系统配色。
    // winxp 的三种 Luna 变体除外：其配色已在 setStyle() 的 polish(QPalette&)
    // 中强制应用，再用 standardPalette() 覆盖会冲掉 Luna。
    if (applyClassicAction_ && applyClassicAction_->isChecked() && !isLunaVariant(name))
        qApp->setPalette(style->standardPalette());

    if (statusStyle_)
        statusStyle_->setText(tr("Style: %1").arg(name));
    if (statusLabel_)
        statusLabel_->setText(tr("Switched to “%1”").arg(name));
    log(tr("Style changed to %1").arg(name));
}

// ---------------------------------------------------------------------------
// 菜单栏与工具栏
// ---------------------------------------------------------------------------

void PreviewWindow::buildMenus()
{
    // 文件
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newAct = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-new")), tr("&New"));
    newAct->setShortcut(QKeySequence::New);
    QAction *openAct = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), tr("&Open..."));
    openAct->setShortcut(QKeySequence::Open);
    QAction *saveAct = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-save")), tr("&Save"));
    saveAct->setShortcut(QKeySequence::Save);
    QAction *saveAsAct = fileMenu->addAction(tr("Save &As..."));
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    fileMenu->addSeparator();

    // 带子菜单的演示
    QMenu *recentMenu = fileMenu->addMenu(tr("&Recent Files"));
    recentMenu->addAction(tr("recent_file_1.cpp"));
    recentMenu->addAction(tr("recent_file_2.cpp"));
    recentMenu->addSeparator();
    recentMenu->addAction(tr("Clear List"));
    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"));
    exitAct->setShortcut(QKeySequence::Quit);

    connect(newAct, &QAction::triggered, this, [this] { log(tr("File → New triggered")); });
    connect(openAct, &QAction::triggered, this, [this] { log(tr("File → Open… triggered")); });
    connect(saveAct, &QAction::triggered, this, [this] { log(tr("File → Save triggered")); });
    connect(saveAsAct, &QAction::triggered, this, [this] { log(tr("File → Save As… triggered")); });
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // 编辑
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *undoAct = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-undo")), tr("&Undo"));
    undoAct->setShortcut(QKeySequence::Undo);
    QAction *redoAct = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-redo")), tr("&Redo"));
    redoAct->setShortcut(QKeySequence::Redo);
    editMenu->addSeparator();
    QAction *cutAct = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-cut")), tr("Cu&t"));
    cutAct->setShortcut(QKeySequence::Cut);
    QAction *copyAct = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("&Copy"));
    copyAct->setShortcut(QKeySequence::Copy);
    QAction *pasteAct = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-paste")), tr("&Paste"));
    pasteAct->setShortcut(QKeySequence::Paste);
    editMenu->addSeparator();
    QAction *findAct = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), tr("&Find..."));
    findAct->setShortcut(QKeySequence::Find);

    // 视图（可勾选的项）
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *toggleToolBarAct = viewMenu->addAction(tr("&Toolbar"));
    toggleToolBarAct->setCheckable(true);
    toggleToolBarAct->setChecked(true);
    QAction *toggleDockAct = viewMenu->addAction(tr("&Log Dock"));
    toggleDockAct->setCheckable(true);
    toggleDockAct->setChecked(true);
    QAction *toggleStatusAct = viewMenu->addAction(tr("&Status Bar"));
    toggleStatusAct->setCheckable(true);
    toggleStatusAct->setChecked(true);
    QAction *toggleFullScreenAct = viewMenu->addAction(tr("&Full Screen"));
    toggleFullScreenAct->setShortcut(QKeySequence::FullScreen);
    toggleFullScreenAct->setCheckable(true);

    connect(toggleToolBarAct, &QAction::toggled, mainToolBar_, &QToolBar::setVisible);
    connect(toggleDockAct, &QAction::toggled, logDock_, &QDockWidget::setVisible);
    connect(toggleStatusAct, &QAction::toggled, this, [this](bool on) {
        statusBar()->setVisible(on);
    });
    connect(toggleFullScreenAct, &QAction::toggled, this, [this](bool on) {
        if (on)
            showFullScreen();
        else
            showNormal();
    });

    // 工具（单选动作组演示）
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    QActionGroup *schemeGroup = new QActionGroup(this);
    QAction *autoScheme = toolsMenu->addAction(tr("Auto Scheme"));
    QAction *darkScheme = toolsMenu->addAction(tr("Dark Scheme"));
    QAction *lightScheme = toolsMenu->addAction(tr("Light Scheme"));
    for (QAction *act : { autoScheme, darkScheme, lightScheme }) {
        act->setCheckable(true);
        schemeGroup->addAction(act);
    }
    autoScheme->setChecked(true);
    toolsMenu->addSeparator();
    // 勾选后采用当前样式的经典配色（standardPalette()），切换样式时也会
    // 随之应用；不勾选则始终使用系统默认配色。
    applyClassicAction_ = toolsMenu->addAction(tr("Apply Classic Palette"));
    applyClassicAction_->setCheckable(true);
    applyClassicAction_->setChecked(false);
    connect(applyClassicAction_, &QAction::toggled, this, [this](bool) {
        // 勾选/取消都重新走 applyStyle()：setStyle() 重新触发 polish(QPalette&)
        //（winxp Luna 变体恢复其强制配色），再按勾选状态决定是否套用
        // standardPalette()。
        applyStyle(styleCombo_->currentText());
    });
    toolsMenu->addSeparator();
    QAction *settingsAct = toolsMenu->addAction(tr("&Settings..."));
    connect(settingsAct, &QAction::triggered, this, [this] { log(tr("Tools → Settings… triggered")); });

    // 帮助
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"));
    QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"));
    connect(aboutAct, &QAction::triggered, this, &PreviewWindow::showAbout);
    connect(aboutQtAct, &QAction::triggered, this, &PreviewWindow::showAboutQt);
}

void PreviewWindow::buildToolBar()
{
    mainToolBar_ = addToolBar(tr("Main Toolbar"));
    mainToolBar_->setObjectName(QStringLiteral("mainToolBar"));
    mainToolBar_->setMovable(true);

    mainToolBar_->addAction(QIcon::fromTheme(QStringLiteral("document-new")), tr("New"),
                            this, [this] { log(tr("Toolbar New clicked")); });
    mainToolBar_->addAction(QIcon::fromTheme(QStringLiteral("document-open")), tr("Open"),
                            this, [this] { log(tr("Toolbar Open clicked")); });
    mainToolBar_->addAction(QIcon::fromTheme(QStringLiteral("document-save")), tr("Save"),
                            this, [this] { log(tr("Toolbar Save clicked")); });
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(QIcon::fromTheme(QStringLiteral("edit-cut")), tr("Cut"),
                            this, [this] { log(tr("Toolbar Cut clicked")); });
    mainToolBar_->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy"),
                            this, [this] { log(tr("Toolbar Copy clicked")); });
    mainToolBar_->addAction(QIcon::fromTheme(QStringLiteral("edit-paste")), tr("Paste"),
                            this, [this] { log(tr("Toolbar Paste clicked")); });

    QToolButton *toolMenuButton = new QToolButton(mainToolBar_);
    toolMenuButton->setText(tr("More"));
    toolMenuButton->setPopupMode(QToolButton::InstantPopup);
    QMenu *moreMenu = new QMenu(toolMenuButton);
    moreMenu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Refresh"),
                        this, [this] { log(tr("More → Refresh clicked")); });
    moreMenu->addAction(QIcon::fromTheme(QStringLiteral("view-zoom-in")), tr("Zoom In"),
                        this, [this] { log(tr("More → Zoom In clicked")); });
    moreMenu->addAction(QIcon::fromTheme(QStringLiteral("view-zoom-out")), tr("Zoom Out"),
                        this, [this] { log(tr("More → Zoom Out clicked")); });
    toolMenuButton->setMenu(moreMenu);
    mainToolBar_->addWidget(toolMenuButton);

    mainToolBar_->addSeparator();
    QLabel *styleLabel = new QLabel(tr("Style: "), mainToolBar_);
    styleCombo_ = new QComboBox(mainToolBar_);
    styleCombo_->setToolTip(tr("Switch the widget style"));
    for (const QString &name : styles_)
        styleCombo_->addItem(name);
    connect(styleCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) { applyStyle(styles_.value(index)); });

    QWidget *spacer = new QWidget(mainToolBar_);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mainToolBar_->addWidget(spacer);
    mainToolBar_->addWidget(styleLabel);
    mainToolBar_->addWidget(styleCombo_);
}

// ---------------------------------------------------------------------------
// 中央区域
// ---------------------------------------------------------------------------

void PreviewWindow::buildCentral()
{
    QTabWidget *tabs = new QTabWidget;
    tabs->setObjectName(QStringLiteral("mainTabs"));
    tabs->addTab(scrollable(createButtonsPage()), tr("Buttons"));
    tabs->addTab(scrollable(createInputsPage()), tr("Inputs"));
    tabs->addTab(scrollable(createListsPage()), tr("Lists && Trees"));
    tabs->addTab(scrollable(createProgressPage()), tr("Progress && Sliders"));
    tabs->addTab(scrollable(createTabsPage()), tr("Tabs && Panels"));
    tabs->addTab(scrollable(createTextPage()), tr("Text"));
    tabs->addTab(scrollable(createFramesPage()), tr("Frames && Groups"));
    tabs->addTab(scrollable(createMiscPage()), tr("Misc"));
    tabs->setCurrentIndex(qBound(0, initialTab_, tabs->count() - 1));
    setCentralWidget(tabs);
}

// 1) 按钮
QWidget *PreviewWindow::createButtonsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QGroupBox *pushGroup = new QGroupBox(tr("QPushButton"));
    QGridLayout *pg = new QGridLayout(pushGroup);
    pg->setSpacing(10);

    QPushButton *normalBtn = new QPushButton(tr("Normal"));
    QPushButton *defaultBtn = new QPushButton(tr("Default"));
    defaultBtn->setDefault(true);
    QPushButton *flatBtn = new QPushButton(tr("Flat"));
    flatBtn->setFlat(true);
    QPushButton *disabledBtn = new QPushButton(tr("Disabled"));
    disabledBtn->setEnabled(false);
    QPushButton *checkableBtn = new QPushButton(tr("Checkable (on)"));
    checkableBtn->setCheckable(true);
    checkableBtn->setChecked(true);
    QPushButton *iconBtn = new QPushButton(tr("With Icon"));
    iconBtn->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok")));

    QPushButton *menuBtn = new QPushButton(tr("With Menu"));
    QMenu *btnMenu = new QMenu(menuBtn);
    btnMenu->addAction(tr("Action &1"));
    btnMenu->addAction(tr("Action &2"));
    btnMenu->addSeparator();
    QMenu *subMenu = btnMenu->addMenu(tr("Sub Menu"));
    subMenu->addAction(tr("Sub Action 1"));
    subMenu->addAction(tr("Sub Action 2"));
    btnMenu->addAction(tr("Action &3"));
    menuBtn->setMenu(btnMenu);

    int c = 0;
    pg->addWidget(normalBtn, 0, c++);
    pg->addWidget(defaultBtn, 0, c++);
    pg->addWidget(flatBtn, 0, c++);
    pg->addWidget(disabledBtn, 0, c++);
    c = 0;
    pg->addWidget(checkableBtn, 1, c++);
    pg->addWidget(iconBtn, 1, c++);
    pg->addWidget(menuBtn, 1, c++);

    QGroupBox *toolGroup = new QGroupBox(tr("QToolButton"));
    QGridLayout *tg = new QGridLayout(toolGroup);
    tg->setSpacing(10);
    QToolButton *tbNormal = new QToolButton;
    tbNormal->setText(tr("Tool Button"));
    QToolButton *tbIcon = new QToolButton;
    tbIcon->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    tbIcon->setToolTip(tr("Icon only, auto-raised"));
    tbIcon->setAutoRaise(true);
    QToolButton *tbCheck = new QToolButton;
    tbCheck->setText(tr("Checkable"));
    tbCheck->setCheckable(true);
    tbCheck->setChecked(true);
    QToolButton *tbMenu = new QToolButton;
    tbMenu->setText(tr("Instant Popup"));
    QMenu *tbMenuObj = new QMenu(tbMenu);
    tbMenuObj->addAction(tr("Tool Menu 1"));
    tbMenuObj->addAction(tr("Tool Menu 2"));
    tbMenu->setMenu(tbMenuObj);
    tbMenu->setPopupMode(QToolButton::InstantPopup);
    QToolButton *tbArrow = new QToolButton;
    tbArrow->setArrowType(Qt::DownArrow);
    tbArrow->setText(tr("Arrow"));
    tbArrow->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    c = 0;
    tg->addWidget(tbNormal, 0, c++);
    tg->addWidget(tbIcon, 0, c++);
    tg->addWidget(tbCheck, 0, c++);
    tg->addWidget(tbMenu, 0, c++);
    tg->addWidget(tbArrow, 0, c++);

    QGroupBox *toggleGroup = new QGroupBox(tr("QCheckBox && QRadioButton"));
    QHBoxLayout *toggleLayout = new QHBoxLayout(toggleGroup);
    QVBoxLayout *checkLayout = new QVBoxLayout;
    QCheckBox *c1 = new QCheckBox(tr("Checked"));
    c1->setChecked(true);
    QCheckBox *c2 = new QCheckBox(tr("Unchecked"));
    QCheckBox *c3 = new QCheckBox(tr("Tri-state"));
    c3->setTristate(true);
    c3->setCheckState(Qt::PartiallyChecked);
    QCheckBox *c4 = new QCheckBox(tr("Disabled"));
    c4->setEnabled(false);
    checkLayout->addWidget(c1);
    checkLayout->addWidget(c2);
    checkLayout->addWidget(c3);
    checkLayout->addWidget(c4);

    QVBoxLayout *radioLayout = new QVBoxLayout;
    QRadioButton *r1 = new QRadioButton(tr("Option 1"));
    r1->setChecked(true);
    QRadioButton *r2 = new QRadioButton(tr("Option 2"));
    QRadioButton *r3 = new QRadioButton(tr("Option 3"));
    QRadioButton *r4 = new QRadioButton(tr("Disabled Option"));
    r4->setEnabled(false);
    radioLayout->addWidget(r1);
    radioLayout->addWidget(r2);
    radioLayout->addWidget(r3);
    radioLayout->addWidget(r4);
    toggleLayout->addLayout(checkLayout);
    toggleLayout->addSpacing(20);
    toggleLayout->addLayout(radioLayout);
    toggleLayout->addStretch();

    outer->addWidget(pushGroup);
    outer->addWidget(toolGroup);
    outer->addWidget(toggleGroup);
    outer->addStretch();

    for (QPushButton *btn : { normalBtn, defaultBtn, flatBtn, disabledBtn, checkableBtn, iconBtn, menuBtn }) {
        connect(btn, &QPushButton::clicked, this, [this, btn] { log(tr("Clicked “%1”").arg(btn->text())); });
    }
    return page;
}

// 2) 输入控件
QWidget *PreviewWindow::createInputsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QGroupBox *lineGroup = new QGroupBox(tr("QLineEdit"));
    QGridLayout *lg = new QGridLayout(lineGroup);
    QLineEdit *le1 = new QLineEdit(tr("Normal text"));
    QLineEdit *le2 = new QLineEdit;
    le2->setPlaceholderText(tr("Placeholder…"));
    QLineEdit *le3 = new QLineEdit(tr("secret"));
    le3->setEchoMode(QLineEdit::Password);
    QLineEdit *le4 = new QLineEdit(tr("Read-only"));
    le4->setReadOnly(true);
    QLineEdit *le5 = new QLineEdit(tr("Disabled"));
    le5->setEnabled(false);
    lg->addWidget(le1, 0, 0);
    lg->addWidget(le2, 0, 1);
    lg->addWidget(le3, 1, 0);
    lg->addWidget(le4, 1, 1);
    lg->addWidget(le5, 2, 0, 1, 2);

    QGroupBox *comboGroup = new QGroupBox(tr("QComboBox && QFontComboBox"));
    QGridLayout *cg = new QGridLayout(comboGroup);
    QComboBox *combo1 = new QComboBox;
    combo1->addItems(QStringList() << tr("Item One") << tr("Item Two") << tr("Item Three") << tr("Item Four"));
    QComboBox *combo2 = new QComboBox;
    combo2->setEditable(true);
    combo2->addItems(QStringList() << tr("Editable Item 1") << tr("Editable Item 2") << tr("Editable Item 3"));
    combo2->setCurrentText(tr("Type something…"));
    QFontComboBox *fontCombo = new QFontComboBox;
    cg->addWidget(new QLabel(tr("Non-editable")), 0, 0);
    cg->addWidget(new QLabel(tr("Editable")), 1, 0);
    cg->addWidget(new QLabel(tr("Font")), 2, 0);
    cg->addWidget(combo1, 0, 1);
    cg->addWidget(combo2, 1, 1);
    cg->addWidget(fontCombo, 2, 1);
    cg->setColumnStretch(1, 1);

    QGroupBox *spinGroup = new QGroupBox(tr("QSpinBox && QDoubleSpinBox && QDateTimeEdit"));
    QGridLayout *sg = new QGridLayout(spinGroup);
    QSpinBox *spin = new QSpinBox;
    spin->setRange(0, 100);
    spin->setValue(42);
    QDoubleSpinBox *dspin = new QDoubleSpinBox;
    dspin->setRange(0.0, 100.0);
    dspin->setValue(13.37);
    dspin->setDecimals(2);
    QDateTimeEdit *dtEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    dtEdit->setCalendarPopup(true);
    QDateEdit *dateEdit = new QDateEdit(QDate::currentDate());
    QTimeEdit *timeEdit = new QTimeEdit(QTime::currentTime());
    sg->addWidget(new QLabel(tr("SpinBox")), 0, 0);
    sg->addWidget(new QLabel(tr("DoubleSpinBox")), 1, 0);
    sg->addWidget(new QLabel(tr("DateTimeEdit")), 2, 0);
    sg->addWidget(new QLabel(tr("DateEdit")), 3, 0);
    sg->addWidget(new QLabel(tr("TimeEdit")), 4, 0);
    sg->addWidget(spin, 0, 1);
    sg->addWidget(dspin, 1, 1);
    sg->addWidget(dtEdit, 2, 1);
    sg->addWidget(dateEdit, 3, 1);
    sg->addWidget(timeEdit, 4, 1);
    sg->setColumnStretch(1, 1);

    QGroupBox *dialGroup = new QGroupBox(tr("QDial && QSlider"));
    QHBoxLayout *dlg = new QHBoxLayout(dialGroup);
    QDial *dial = new QDial;
    dial->setRange(0, 100);
    dial->setValue(60);
    dial->setNotchesVisible(true);
    QSlider *hSlider = new QSlider(Qt::Horizontal);
    hSlider->setRange(0, 100);
    hSlider->setValue(35);
    QSlider *vSlider = new QSlider(Qt::Vertical);
    vSlider->setRange(0, 100);
    vSlider->setValue(80);
    vSlider->setInvertedAppearance(true);
    dlg->addWidget(dial);
    dlg->addWidget(hSlider, 1);
    dlg->addWidget(vSlider);

    outer->addWidget(lineGroup);
    outer->addWidget(comboGroup);
    outer->addWidget(spinGroup);
    outer->addWidget(dialGroup);
    outer->addStretch();
    return page;
}

// 3) 列表 / 树 / 表格
QWidget *PreviewWindow::createListsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QGroupBox *listGroup = new QGroupBox(tr("QListWidget"));
    QHBoxLayout *listLayout = new QHBoxLayout(listGroup);

    QListWidget *list = new QListWidget;
    for (int i = 1; i <= 8; ++i)
        list->addItem(tr("List item %1").arg(i));
    list->setCurrentRow(2);

    QListWidget *iconList = new QListWidget;
    iconList->setViewMode(QListView::IconMode);
    iconList->setMovement(QListView::Static);
    iconList->setResizeMode(QListView::Adjust);
    for (int i = 1; i <= 6; ++i) {
        QListWidgetItem *item = new QListWidgetItem(
            QIcon::fromTheme(QStringLiteral("dialog-information")),
            tr("Icon %1").arg(i));
        iconList->addItem(item);
    }
    listLayout->addWidget(list, 1);
    listLayout->addWidget(iconList, 1);

    QGroupBox *treeGroup = new QGroupBox(tr("QTreeWidget && QTreeView"));
    QHBoxLayout *treeLayout = new QHBoxLayout(treeGroup);
    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderLabels(QStringList() << tr("Name") << tr("Value"));
    QTreeWidgetItem *root = new QTreeWidgetItem(tree, QStringList() << tr("Project"));
    for (int i = 1; i <= 3; ++i) {
        QTreeWidgetItem *child = new QTreeWidgetItem(root,
            QStringList() << tr("Item %1").arg(i) << QString::number(i * 10));
        child->setCheckState(0, (i == 2) ? Qt::Checked : Qt::Unchecked);
        new QTreeWidgetItem(child, QStringList() << tr("Sub %1.1").arg(i) << QString::number(i));
    }
    root->setExpanded(true);
    tree->setCurrentItem(root->child(0));

    QTreeView *plainTreeView = new QTreeView;
    QStandardItemModel *treeModel = new QStandardItemModel(plainTreeView);
    treeModel->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Data"));
    for (int i = 1; i <= 4; ++i) {
        QStandardItem *item = new QStandardItem(QStringLiteral("View item %1").arg(i));
        item->appendRow({ new QStandardItem(QStringLiteral("child")), new QStandardItem(QString::number(i)) });
        treeModel->appendRow(item);
    }
    plainTreeView->setModel(treeModel);
    plainTreeView->expandAll();

    treeLayout->addWidget(tree, 1);
    treeLayout->addWidget(plainTreeView, 1);

    QGroupBox *tableGroup = new QGroupBox(tr("QTableWidget && QListView"));
    QHBoxLayout *tableLayout = new QHBoxLayout(tableGroup);
    QTableWidget *table = new QTableWidget(4, 3);
    table->setHorizontalHeaderLabels(QStringList() << tr("Column 1") << tr("Column 2") << tr("Column 3"));
    table->setVerticalHeaderLabels(QStringList() << tr("Row 1") << tr("Row 2") << tr("Row 3") << tr("Row 4"));
    for (int r = 0; r < 4; ++r) {
        for (int col = 0; col < 3; ++col)
            table->setItem(r, col, new QTableWidgetItem(QStringLiteral("%1,%2").arg(r + 1).arg(col + 1)));
    }
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setShowGrid(true);
    table->setSortingEnabled(true);
    table->resizeColumnsToContents();

    QListView *listView = new QListView;
    QStandardItemModel *viewModel = new QStandardItemModel(listView);
    for (int i = 1; i <= 5; ++i) {
        QStandardItem *item = new QStandardItem(QIcon::fromTheme(QStringLiteral("folder")),
                                                tr("Folder %1").arg(i));
        viewModel->appendRow(item);
    }
    listView->setModel(viewModel);
    listView->setViewMode(QListView::ListMode);

    tableLayout->addWidget(table, 2);
    tableLayout->addWidget(listView, 1);

    QGroupBox *columnGroup = new QGroupBox(tr("QColumnView"));
    QVBoxLayout *colLayout = new QVBoxLayout(columnGroup);
    QColumnView *columnView = new QColumnView;
    QStandardItemModel *colModel = new QStandardItemModel(columnView);
    QStandardItem *top = new QStandardItem(tr("Root"));
    for (int i = 1; i <= 3; ++i) {
        QStandardItem *branch = new QStandardItem(tr("Branch %1").arg(i));
        branch->appendRow(new QStandardItem(tr("Leaf %1.1").arg(i)));
        branch->appendRow(new QStandardItem(tr("Leaf %1.2").arg(i)));
        top->appendRow(branch);
    }
    colModel->appendRow(top);
    columnView->setModel(colModel);
    columnView->setMinimumHeight(140);
    colLayout->addWidget(columnView);

    outer->addWidget(listGroup);
    outer->addWidget(treeGroup);
    outer->addWidget(tableGroup);
    outer->addWidget(columnGroup);
    return page;
}

// 4) 进度条 / 滑块 / 滚动条 / 分割器
QWidget *PreviewWindow::createProgressPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QGroupBox *progressGroup = new QGroupBox(tr("QProgressBar"));
    QGridLayout *pg = new QGridLayout(progressGroup);
    QProgressBar *hProgress = new QProgressBar;
    hProgress->setRange(0, 100);
    hProgress->setValue(64);
    hProgress->setFormat(tr("%p% — %v of %m"));
    QProgressBar *hProgressText = new QProgressBar;
    hProgressText->setRange(0, 100);
    hProgressText->setValue(30);
    QProgressBar *busyProgress = new QProgressBar;
    busyProgress->setRange(0, 0); // 忙碌状态
    busyProgress->setTextVisible(false);
    QProgressBar *vProgress = new QProgressBar;
    vProgress->setRange(0, 100);
    vProgress->setValue(80);
    vProgress->setOrientation(Qt::Vertical);
    vProgress->setFixedHeight(140);
    QProgressBar *vProgressBusy = new QProgressBar;
    vProgressBusy->setRange(0, 0);
    vProgressBusy->setOrientation(Qt::Vertical);
    vProgressBusy->setTextVisible(false);
    vProgressBusy->setFixedHeight(140);
    pg->addWidget(hProgress, 0, 0, 1, 3);
    pg->addWidget(hProgressText, 1, 0, 1, 3);
    pg->addWidget(new QLabel(tr("Busy (indeterminate)")), 2, 0, 1, 2);
    pg->addWidget(busyProgress, 3, 0, 1, 2);
    pg->addWidget(new QLabel(tr("Vertical")), 2, 2);
    pg->addWidget(new QLabel(tr("Vertical busy")), 2, 3);
    pg->addWidget(vProgress, 3, 2);
    pg->addWidget(vProgressBusy, 3, 3);
    pg->setColumnStretch(0, 1);
    pg->setRowMinimumHeight(3, 140);

    QGroupBox *sliderGroup = new QGroupBox(tr("QSlider && QScrollBar"));
    QGridLayout *sg = new QGridLayout(sliderGroup);
    QSlider *hSlider = new QSlider(Qt::Horizontal);
    hSlider->setRange(0, 100);
    hSlider->setValue(55);
    QSlider *hTicks = new QSlider(Qt::Horizontal);
    hTicks->setRange(0, 100);
    hTicks->setValue(25);
    hTicks->setTickPosition(QSlider::TicksBothSides);
    hTicks->setTickInterval(10);
    QSlider *vSlider = new QSlider(Qt::Vertical);
    vSlider->setRange(0, 100);
    vSlider->setValue(70);
    vSlider->setTickPosition(QSlider::TicksRight);
    vSlider->setFixedHeight(140);
    QScrollBar *hBar = new QScrollBar(Qt::Horizontal);
    hBar->setRange(0, 100);
    hBar->setValue(40);
    QScrollBar *vBar = new QScrollBar(Qt::Vertical);
    vBar->setRange(0, 100);
    vBar->setValue(60);
    vBar->setFixedHeight(140);
    sg->addWidget(new QLabel(tr("Horizontal slider")), 0, 0, 1, 2);
    sg->addWidget(hSlider, 1, 0, 1, 2);
    sg->addWidget(new QLabel(tr("With tick marks")), 2, 0, 1, 2);
    sg->addWidget(hTicks, 3, 0, 1, 2);
    sg->addWidget(new QLabel(tr("Vertical slider")), 4, 0);
    sg->addWidget(new QLabel(tr("Vertical scrollbar")), 4, 1);
    sg->addWidget(vSlider, 5, 0);
    sg->addWidget(vBar, 5, 1);
    sg->addWidget(new QLabel(tr("Horizontal scrollbar")), 6, 0, 1, 2);
    sg->addWidget(hBar, 7, 0, 1, 2);
    sg->setColumnStretch(0, 1);
    sg->setRowMinimumHeight(5, 140);

    QGroupBox *splitGroup = new QGroupBox(tr("QSplitter"));
    QVBoxLayout *splitLayout = new QVBoxLayout(splitGroup);
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    QLabel *leftPanel = new QLabel(tr("Left panel"), splitter);
    leftPanel->setAlignment(Qt::AlignCenter);
    leftPanel->setMinimumWidth(120);
    leftPanel->setStyleSheet(QStringLiteral("background: palette(base);"));
    QLabel *rightPanel = new QLabel(tr("Right panel"), splitter);
    rightPanel->setAlignment(Qt::AlignCenter);
    rightPanel->setMinimumWidth(120);
    rightPanel->setStyleSheet(QStringLiteral("background: palette(alternate-base);"));
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setSizes({ 220, 220 });
    splitLayout->addWidget(splitter);
    splitLayout->addWidget(new QLabel(tr("Drag the handle to resize the two panes.")));

    outer->addWidget(progressGroup);
    outer->addWidget(sliderGroup);
    outer->addWidget(splitGroup);
    outer->addStretch();
    return page;
}

// 5) 标签页 / 工具盒 / 堆叠
QWidget *PreviewWindow::createTabsPage()
{
    QWidget *page = new QWidget;
    QHBoxLayout *outer = new QHBoxLayout(page);

    QTabWidget *tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::North);
    tabs->addTab(makePageLabel(tr("First tab content.\nA few widgets to show the tab look.")), tr("Tab 1"));
    QCheckBox *tabCheck = new QCheckBox(tr("A checkbox inside a tab"));
    QPushButton *tabButton = new QPushButton(tr("A button inside a tab"));
    QWidget *tab2 = new QWidget;
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->addWidget(tabCheck);
    tab2Layout->addWidget(tabButton);
    tab2Layout->addStretch();
    tabs->addTab(tab2, tr("Tab 2"));
    QTextEdit *tab3Edit = new QTextEdit(tr("Editable text inside a tab."));
    tabs->addTab(tab3Edit, tr("Tab 3"));
    tabs->addTab(makePageLabel(tr("Another simple tab.")), tr("Tab 4"));

    QTabWidget *westTabs = new QTabWidget;
    westTabs->setTabPosition(QTabWidget::West);
    westTabs->addTab(makePageLabel(tr("West position")), tr("West"));
    westTabs->addTab(makePageLabel(tr("Second")), tr("Second"));
    westTabs->addTab(makePageLabel(tr("Third")), tr("Third"));

    QTabWidget *southTabs = new QTabWidget;
    southTabs->setTabPosition(QTabWidget::South);
    southTabs->addTab(makePageLabel(tr("South position")), tr("South"));
    southTabs->addTab(makePageLabel(tr("Another")), tr("Another"));

    QToolBox *toolBox = new QToolBox;
    QLabel *tb1 = new QLabel(tr("ToolBox page 1"));
    tb1->setAlignment(Qt::AlignCenter);
    QLabel *tb2 = new QLabel(tr("ToolBox page 2"));
    tb2->setAlignment(Qt::AlignCenter);
    QLabel *tb3 = new QLabel(tr("ToolBox page 3"));
    tb3->setAlignment(Qt::AlignCenter);
    toolBox->addItem(tb1, tr("Page &One"));
    toolBox->addItem(tb2, tr("Page &Two"));
    toolBox->addItem(tb3, tr("Page &Three"));
    toolBox->setCurrentIndex(1);

    QWidget *stackHost = new QWidget;
    QVBoxLayout *stackLayout = new QVBoxLayout(stackHost);
    QStackedWidget *stack = new QStackedWidget;
    stack->addWidget(makePageLabel(tr("Stacked page 1")));
    stack->addWidget(makePageLabel(tr("Stacked page 2")));
    stack->addWidget(makePageLabel(tr("Stacked page 3")));
    stack->setCurrentIndex(1);
    QComboBox *stackSwitch = new QComboBox;
    stackSwitch->addItems(QStringList() << tr("Page 1") << tr("Page 2") << tr("Page 3"));
    stackSwitch->setCurrentIndex(1);
    connect(stackSwitch, QOverload<int>::of(&QComboBox::currentIndexChanged),
            stack, &QStackedWidget::setCurrentIndex);
    stackLayout->addWidget(stackSwitch);
    stackLayout->addWidget(stack);

    QWidget *right = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(toolBox, 1);
    rightLayout->addWidget(stackHost);

    QWidget *left = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->addWidget(tabs, 2);
    leftLayout->addWidget(westTabs, 1);
    leftLayout->addWidget(southTabs, 1);

    outer->addWidget(left, 3);
    outer->addWidget(right, 2);
    return page;
}

// 6) 文本
QWidget *PreviewWindow::createTextPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QLabel *label = new QLabel(
        tr("<b>QLabel</b> supports <i>rich text</i>: <font color=\"#c00000\">colored</font>, "
           "<a href=\"https://www.qt.io\">links</a>, and "
           "<span style=\"background-color:#ffff00\">highlighted</span> spans."));
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
        log(tr("Link activated: %1").arg(link));
    });

    QTextEdit *textEdit = new QTextEdit;
    textEdit->setHtml(
        tr("<h2>QTextEdit</h2>"
           "<p>Rich text editing widget with <b>bold</b>, <i>italic</i>, "
           "<u>underline</u> and <span style=\"color:#0a7a0a;\">colored</span> text.</p>"
           "<ul><li>List item one</li><li>List item two</li></ul>"
           "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
           "<tr><th>Col 1</th><th>Col 2</th></tr>"
           "<tr><td>a</td><td>b</td></tr>"
           "</table>"));

    QPlainTextEdit *plainEdit = new QPlainTextEdit;
    plainEdit->setPlainText(
        QStringLiteral("// QPlainTextEdit\n"
                       "static const char *snippet[] = {\n"
                       "    \"line one\",\n"
                       "    \"line two\",\n"
                       "    \"line three\",\n"
                       "};\n"
                       "int main() { return 0; }\n"));
    plainEdit->setMaximumBlockCount(200);

    QTextBrowser *browser = new QTextBrowser;
    browser->setHtml(tr("<h3>QTextBrowser</h3><p>Read-only HTML view.</p>"
                        "<p style=\"background-color:#eeeeee\">Boxed paragraph</p>"
                        "<p>A <em>final</em> paragraph with a <a href=\"https://doc.qt.io\">link</a>.</p>"));

    outer->addWidget(label);
    outer->addWidget(textEdit, 1);
    outer->addWidget(plainEdit, 1);
    outer->addWidget(browser, 1);
    return page;
}

// 7) 分组 / 框架
QWidget *PreviewWindow::createFramesPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QGroupBox *plainGroup = new QGroupBox(tr("QGroupBox (plain)"));
    QFormLayout *form = new QFormLayout(plainGroup);
    form->addRow(tr("Name:"), new QLineEdit(tr("Example")));
    form->addRow(tr("Description:"), new QLineEdit);
    form->addRow(tr("Enabled:"), new QCheckBox(tr("Yes")));

    QGroupBox *checkGroup = new QGroupBox(tr("QGroupBox (checkable)"));
    checkGroup->setCheckable(true);
    checkGroup->setChecked(true);
    QVBoxLayout *checkLayout = new QVBoxLayout(checkGroup);
    checkLayout->addWidget(new QLabel(tr("This group can be enabled / disabled.")));
    QSlider *innerSlider = new QSlider(Qt::Horizontal);
    innerSlider->setRange(0, 100);
    innerSlider->setValue(50);
    checkLayout->addWidget(innerSlider);

    QGroupBox *frameGroup = new QGroupBox(tr("QFrame shapes"));
    QGridLayout *fg = new QGridLayout(frameGroup);
    QFrame *hLine = new QFrame;
    hLine->setFrameShape(QFrame::HLine);
    hLine->setFrameShadow(QFrame::Sunken);
    QFrame *vLine = new QFrame;
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Sunken);
    QFrame *box = new QFrame;
    box->setFrameShape(QFrame::Box);
    box->setFrameShadow(QFrame::Raised);
    box->setMinimumSize(100, 60);
    QFrame *panel = new QFrame;
    panel->setFrameShape(QFrame::Panel);
    panel->setFrameShadow(QFrame::Raised);
    panel->setMinimumSize(100, 60);
    QFrame *styled = new QFrame;
    styled->setFrameShape(QFrame::StyledPanel);
    styled->setFrameShadow(QFrame::Raised);
    styled->setMinimumSize(100, 60);
    QFrame *win = new QFrame;
    win->setFrameShape(QFrame::WinPanel);
    win->setFrameShadow(QFrame::Sunken);
    win->setMinimumSize(100, 60);
    fg->addWidget(new QLabel(tr("HLine")), 0, 0);
    fg->addWidget(hLine, 0, 1);
    fg->addWidget(new QLabel(tr("VLine")), 0, 2);
    fg->addWidget(vLine, 0, 3);
    fg->addWidget(new QLabel(tr("Box")), 1, 0);
    fg->addWidget(box, 1, 1);
    fg->addWidget(new QLabel(tr("Panel")), 1, 2);
    fg->addWidget(panel, 1, 3);
    fg->addWidget(new QLabel(tr("StyledPanel")), 2, 0);
    fg->addWidget(styled, 2, 1);
    fg->addWidget(new QLabel(tr("WinPanel")), 2, 2);
    fg->addWidget(win, 2, 3);

    QGroupBox *scrollGroup = new QGroupBox(tr("QScrollArea"));
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollGroup);
    QScrollArea *scrollArea = new QScrollArea;
    QWidget *bigContent = new QWidget;
    QVBoxLayout *bigLayout = new QVBoxLayout(bigContent);
    bigLayout->addWidget(new QLabel(tr("Scroll me — there is more below.")));
    for (int i = 1; i <= 12; ++i)
        bigLayout->addWidget(new QPushButton(tr("Content button %1").arg(i)));
    bigLayout->addWidget(new QLabel(tr("The end.")));
    scrollArea->setWidget(bigContent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFixedHeight(160);
    scrollLayout->addWidget(scrollArea);

    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->addWidget(plainGroup, 1);
    topRow->addWidget(checkGroup, 1);

    outer->addLayout(topRow);
    outer->addWidget(frameGroup);
    outer->addWidget(scrollGroup);
    outer->addStretch();
    return page;
}

// 8) 其他
QWidget *PreviewWindow::createMiscPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QGroupBox *calendarGroup = new QGroupBox(tr("QCalendarWidget"));
    QHBoxLayout *calLayout = new QHBoxLayout(calendarGroup);
    QCalendarWidget *calendar = new QCalendarWidget;
    calendar->setSelectedDate(QDate::currentDate());
    calendar->setGridVisible(true);
    QLCDNumber *lcd = new QLCDNumber(7);
    lcd->display(3.14159);
    lcd->setSegmentStyle(QLCDNumber::Flat);
    QVBoxLayout *lcdLayout = new QVBoxLayout;
    lcdLayout->addWidget(new QLabel(tr("QLCDNumber")));
    lcdLayout->addWidget(lcd);
    calLayout->addWidget(calendar, 2);
    calLayout->addLayout(lcdLayout, 1);

    QGroupBox *dialogGroup = new QGroupBox(tr("Standard dialogs"));
    QGridLayout *dg = new QGridLayout(dialogGroup);
    QPushButton *msgBoxBtn = new QPushButton(tr("QMessageBox"));
    QPushButton *inputBtn = new QPushButton(tr("QInputDialog"));
    QPushButton *colorBtn = new QPushButton(tr("QColorDialog"));
    QPushButton *fontBtn = new QPushButton(tr("QFontDialog"));
    QPushButton *fileBtn = new QPushButton(tr("QFileDialog"));
    QPushButton *progressDlgBtn = new QPushButton(tr("QProgressDialog"));
    dg->addWidget(msgBoxBtn, 0, 0);
    dg->addWidget(inputBtn, 0, 1);
    dg->addWidget(colorBtn, 0, 2);
    dg->addWidget(fontBtn, 1, 0);
    dg->addWidget(fileBtn, 1, 1);
    dg->addWidget(progressDlgBtn, 1, 2);
    connect(msgBoxBtn, &QPushButton::clicked, this, &PreviewWindow::showMessageBox);
    connect(inputBtn, &QPushButton::clicked, this, &PreviewWindow::showInputDialog);
    connect(colorBtn, &QPushButton::clicked, this, &PreviewWindow::showColorDialog);
    connect(fontBtn, &QPushButton::clicked, this, &PreviewWindow::showFontDialog);
    connect(fileBtn, &QPushButton::clicked, this, &PreviewWindow::showFileDialog);
    connect(progressDlgBtn, &QPushButton::clicked, this, &PreviewWindow::showProgressDialog);

    QGroupBox *buttonBoxGroup = new QGroupBox(tr("QDialogButtonBox && QToolTip"));
    QHBoxLayout *bbLayout = new QHBoxLayout(buttonBoxGroup);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel
                                                       | QDialogButtonBox::Apply | QDialogButtonBox::Help);
    QToolButton *tipButton = new QToolButton;
    tipButton->setText(tr("Hover me"));
    tipButton->setToolTip(tr("This is a QToolTip. It is shown when you hover over this button."));
    bbLayout->addWidget(buttonBox, 1);
    bbLayout->addWidget(tipButton);

    QGroupBox *progressGroup = new QGroupBox(tr("QProgressBar in a window"));
    QVBoxLayout *pl = new QVBoxLayout(progressGroup);
    QProgressBar *busy = new QProgressBar;
    busy->setRange(0, 0);
    busy->setTextVisible(false);
    QProgressBar *withText = new QProgressBar;
    withText->setRange(0, 100);
    withText->setValue(72);
    withText->setFormat(tr("%p%"));
    pl->addWidget(busy);
    pl->addWidget(withText);

    outer->addWidget(calendarGroup);
    outer->addWidget(dialogGroup);
    outer->addWidget(buttonBoxGroup);
    outer->addWidget(progressGroup);
    outer->addStretch();
    return page;
}

// ---------------------------------------------------------------------------
// Dock 与状态栏
// ---------------------------------------------------------------------------

void PreviewWindow::buildDocks()
{
    logDock_ = new QDockWidget(tr("Log"), this);
    logDock_->setObjectName(QStringLiteral("logDock"));
    logList_ = new QListWidget(logDock_);
    logList_->addItem(tr("Ready."));
    logDock_->setWidget(logList_);
    addDockWidget(Qt::RightDockWidgetArea, logDock_);
    logDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                          | QDockWidget::DockWidgetClosable);
}

void PreviewWindow::buildStatusBar()
{
    statusLabel_ = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(statusLabel_);

    statusProgress_ = new QProgressBar(this);
    statusProgress_->setRange(0, 100);
    statusProgress_->setValue(37);
    statusProgress_->setMaximumWidth(120);
    statusBar()->addPermanentWidget(statusProgress_);

    statusStyle_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusStyle_);
}

void PreviewWindow::log(const QString &message)
{
    if (!logList_)
        return;
    logList_->addItem(message);
    logList_->scrollToBottom();
    if (statusLabel_)
        statusLabel_->setText(message);
}

// ---------------------------------------------------------------------------
// 标准对话框演示
// ---------------------------------------------------------------------------

void PreviewWindow::showMessageBox()
{
    QMessageBox::information(this, tr("QMessageBox"),
        tr("This is an information box.\nStandard buttons use the current style."));
}

void PreviewWindow::showInputDialog()
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("QInputDialog"),
                                               tr("Enter some text:"), QLineEdit::Normal,
                                               QStringLiteral("hello"), &ok);
    if (ok)
        log(tr("Input dialog returned: %1").arg(text));
}

void PreviewWindow::showColorDialog()
{
    const QColor color = QColorDialog::getColor(Qt::green, this, tr("QColorDialog"));
    if (color.isValid())
        log(tr("Color dialog returned: %1").arg(color.name()));
}

void PreviewWindow::showFontDialog()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, QFont(), this, tr("QFontDialog"));
    if (ok)
        log(tr("Font dialog returned: %1").arg(font.family()));
}

void PreviewWindow::showFileDialog()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("QFileDialog"),
                                                      QString(), QStringLiteral("All files (*)"));
    if (!file.isEmpty())
        log(tr("File dialog returned: %1").arg(file));
}

void PreviewWindow::showProgressDialog()
{
    QProgressDialog dialog(tr("Working…"), tr("Cancel"), 0, 100, this);
    dialog.setWindowTitle(tr("QProgressDialog"));
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumDuration(0);
    for (int i = 0; i <= 100; ++i) {
        dialog.setValue(i);
        QCoreApplication::processEvents();
        if (dialog.wasCanceled())
            break;
        QThread::msleep(8);
    }
}

void PreviewWindow::showAbout()
{
    QMessageBox::about(this, tr("About qtstyles Preview"),
        tr("<b>qtstyles Preview</b><br/>"
           "A demo program that previews the widget styles shipped with qtstyles:<br/>"
           "<ul>"
           "<li><b>dirtylooks</b> (cleanlooks)</li>"
           "<li><b>oldschool</b> (motif)</li>"
           "<li><b>newschool</b> (cde)</li>"
           "<li><b>highschool</b> (motif + SGI red accents)</li>"
           "<li><b>plastic</b> (plastique)</li>"
           "<li><b>phase</b></li>"
           "<li><b>bluecurve</b></li>"
           "<li><b>keramik</b></li>"
           "<li><b>platinum</b></li>"
           "</ul>"
           "Switch styles from the toolbar drop-down."));
}

void PreviewWindow::showAboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
