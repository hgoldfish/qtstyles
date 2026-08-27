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

#ifndef QTSTYLES_PALETTE_H
#define QTSTYLES_PALETTE_H

#include <QtGui/qpalette.h>

namespace QtStyles {

// Qt 6.8+ 新增的 Accent 角色（高亮色别名）：经典调色板统一以 Highlight 作为
// accent，三个 ColorGroup 都显式给出，避免该角色缺省继承 app 调色板。
// Qt 6.8 之前的版本没有这个角色，条件编译掉即可。
inline void applyClassicAccent(QPalette *pal)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    pal->setBrush(QPalette::Accent, pal->brush(QPalette::Highlight));
    pal->setBrush(QPalette::Disabled, QPalette::Accent, pal->brush(QPalette::Highlight));
#else
    // Qt < 6.8 没有 Accent 角色，条件编译掉了函数体，显式引用参数避免
    // -Wunused-parameter。
    Q_UNUSED(pal);
#endif
}

// 把 Disabled 组里尚未被 style 自定义的其余角色补齐为 Active 组对应值，使
// 三个 ColorGroup 全部显式填满。各 style 的 standardPalette() 应在设好
// 自定义的 Disabled 覆盖后调用本函数收尾，保证返回的配色自足——QPalette
// 的默认构造会继承当前 application palette，缺省角色会在样式切换时串入
// 上一个样式的配色。
inline void completeClassicDisabled(QPalette *pal)
{
    pal->setBrush(QPalette::Disabled, QPalette::Window, pal->brush(QPalette::Window));
    pal->setBrush(QPalette::Disabled, QPalette::AlternateBase, pal->brush(QPalette::AlternateBase));
    pal->setBrush(QPalette::Disabled, QPalette::ToolTipBase, pal->brush(QPalette::ToolTipBase));
    pal->setBrush(QPalette::Disabled, QPalette::ToolTipText, pal->brush(QPalette::ToolTipText));
    pal->setBrush(QPalette::Disabled, QPalette::Button, pal->brush(QPalette::Button));
    pal->setBrush(QPalette::Disabled, QPalette::BrightText, pal->brush(QPalette::BrightText));
    pal->setBrush(QPalette::Disabled, QPalette::Light, pal->brush(QPalette::Light));
    pal->setBrush(QPalette::Disabled, QPalette::Midlight, pal->brush(QPalette::Midlight));
    pal->setBrush(QPalette::Disabled, QPalette::Mid, pal->brush(QPalette::Mid));
    pal->setBrush(QPalette::Disabled, QPalette::Dark, pal->brush(QPalette::Dark));
    pal->setBrush(QPalette::Disabled, QPalette::Shadow, pal->brush(QPalette::Shadow));
    pal->setBrush(QPalette::Disabled, QPalette::Highlight, pal->brush(QPalette::Highlight));
    pal->setBrush(QPalette::Disabled, QPalette::HighlightedText, pal->brush(QPalette::HighlightedText));
    pal->setBrush(QPalette::Disabled, QPalette::Link, pal->brush(QPalette::Link));
    pal->setBrush(QPalette::Disabled, QPalette::LinkVisited, pal->brush(QPalette::LinkVisited));
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    pal->setBrush(QPalette::Disabled, QPalette::Accent, pal->brush(QPalette::Accent));
#endif
}

// 在已设好各角色颜色的 palette 上派生经典 Disabled 组：文本/图标前景统一
// 降为 Dark 色、Base 退为 Window 色、PlaceholderText 一并变灰，并把其余
// Disabled 角色一并补齐（completeClassicDisabled），同时给出 Qt 6.8+ 的
// Accent 角色。各 style 的 standardPalette() 应显式填满全部角色后调用本
// 函数收尾，保证返回的配色自足——QPalette 的默认构造会继承当前 application
// palette，缺省角色会在样式切换时串入上一个样式的配色。
inline void applyClassicDisabled(QPalette *pal)
{
    const QColor dark = pal->color(QPalette::Dark);
    pal->setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    pal->setBrush(QPalette::Disabled, QPalette::Text, dark);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    pal->setBrush(QPalette::Disabled, QPalette::PlaceholderText, dark);
#endif
    pal->setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    pal->setBrush(QPalette::Disabled, QPalette::Base, pal->color(QPalette::Window));
    completeClassicDisabled(pal);
    applyClassicAccent(pal);
}

} // namespace QtStyles

#endif // QTSTYLES_PALETTE_H
