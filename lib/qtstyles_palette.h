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

// 在已设好各角色颜色的 palette 上派生经典 Disabled 组：文本/图标前景统一
// 降为 Dark 色、Base 退为 Window 色、PlaceholderText 一并变灰。各 style 的
// standardPalette() 应显式填满全部角色后调用本函数收尾，保证返回的配色
// 自足——QPalette 的默认构造会继承当前 application palette，缺省角色会在
// 样式切换时串入上一个样式的配色。
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
}

} // namespace QtStyles

#endif // QTSTYLES_PALETTE_H
