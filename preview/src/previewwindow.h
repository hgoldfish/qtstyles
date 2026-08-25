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

#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QMainWindow>
#include <QPalette>
#include <QStringList>

class QAction;
class QComboBox;
class QDockWidget;
class QLabel;
class QListWidget;
class QProgressBar;
class QStyle;
class QToolBar;

// Create a style by name. The returned style is owned by the caller
// (normally QApplication::setStyle() takes ownership).
// Built-in qtstyles are instantiated directly; anything else is
// delegated to QStyleFactory so system styles also work.
QStyle *createStyle(const QString &name);

class PreviewWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PreviewWindow(const QString &initialStyle = QString(), int initialTab = 0,
                           QWidget *parent = nullptr);

private slots:
    void applyStyle(const QString &name);
    void showMessageBox();
    void showInputDialog();
    void showColorDialog();
    void showFontDialog();
    void showFileDialog();
    void showProgressDialog();
    void showAbout();
    void showAboutQt();

private:
    QWidget *createButtonsPage();
    QWidget *createInputsPage();
    QWidget *createListsPage();
    QWidget *createProgressPage();
    QWidget *createTabsPage();
    QWidget *createTextPage();
    QWidget *createFramesPage();
    QWidget *createMiscPage();

    void buildMenus();
    void buildToolBar();
    void buildCentral();
    void buildDocks();
    void buildStatusBar();
    void log(const QString &message);

    QStringList styles_;
    // 启动时的默认调色板，切换样式时以此为基线恢复，避免样式间调色板互相污染。
    QPalette initialPalette_;
    // 启动时选中的预览 tab 页索引。
    int initialTab_ = 0;
    // 「应用经典配色」勾选项：勾选后切换/应用样式时采用其 standardPalette()。
    QAction *applyClassicAction_ = nullptr;
    QComboBox *styleCombo_ = nullptr;
    QToolBar *mainToolBar_ = nullptr;
    QDockWidget *logDock_ = nullptr;
    QListWidget *logList_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QProgressBar *statusProgress_ = nullptr;
    QLabel *statusStyle_ = nullptr;
};

#endif // PREVIEWWINDOW_H
