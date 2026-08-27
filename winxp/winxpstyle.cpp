/****************************************************************************
* WinXP Qt style (Office 2003 look, port of WindowsModernStyle)
* Copyright (C) 2008-2009 Michał Męciński
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*   3. Neither the name of the copyright holder nor the names of the
*      contributors may be used to endorse or promote products derived from
*      this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/

#include "winxpstyle.h"
#include "qtstyles_palette.h"
#include "qstylehelper_p.h"

#include <QStyleOption>
#include <QPainter>
#include <QPainterPath>
#include <QMainWindow>
#include <QAbstractButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QSlider>
#include <QScrollBar>
#include <QLineEdit>
#include <QComboBox>
#include <QAbstractSpinBox>
#include <QDateTimeEdit>
#include <QHeaderView>
#include <QProgressBar>
#include <QTimer>
#include <QEvent>
#include <QToolBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QLayout>
#include <QLinearGradient>
#include <QFontMetricsF>
#include <QtMath>
#include <QCommonStyle>
#include <QStyleFactory>
#include <QVariantAnimation>

// alpha is the WEIGHT OF src (not dest): 1.0 -> pure src, 0.0 -> pure dest.
// This is the opposite convention of blendColor() below, where t is the
// weight of dest. Keep the two straight or the 0.15/0.85 class of bug returns.
static QColor blendColors( const QColor& src, const QColor& dest, double srcWeight )
{
    double red = srcWeight * src.red() + ( 1.0 - srcWeight ) * dest.red();
    double green = srcWeight * src.green() + ( 1.0 - srcWeight ) * dest.green();
    double blue = srcWeight * src.blue() + ( 1.0 - srcWeight ) * dest.blue();
    return QColor( (int)( red + 0.5 ), (int)( green + 0.5 ), (int)( blue + 0.5 ) );
}

static QColor blendRoles( const QPalette& palette, QPalette::ColorRole src, QPalette::ColorRole dest, double srcWeight )
{
    return blendColors( palette.color( src ), palette.color( dest ), srcWeight );
}

static QColor blendColor( const QColor& src, const QColor& dest, double t )
{
    return QColor( (int)( src.red() + ( dest.red() - src.red() ) * t + 0.5 ),
        (int)( src.green() + ( dest.green() - src.green() ) * t + 0.5 ),
        (int)( src.blue() + ( dest.blue() - src.blue() ) * t + 0.5 ),
        (int)( src.alpha() + ( dest.alpha() - src.alpha() ) * t + 0.5 ) );
}

// XP "Luna" push-button look, reproduced with QPainter. The states mirror the
// five frames of the original luna.msstyles "button.bmp" (Normal, Hot,
// Pressed, Disabled, Defaulted); the hue values are derived from the theme's
// BorderColorHint/FillColorHint/AccentColorHint (0,60,116 / 243,243,239 /
// 250,196,88) and the classic Luna gradient ramps.
namespace {

enum XPButtonState
{
    XPBS_Normal,
    XPBS_Hot,
    XPBS_Pressed,
    XPBS_Disabled,
    XPBS_Default
};

struct XPButtonColors
{
    QColor border;
    QColor fillTop;
    QColor fillMid;
    QColor fillBottom;
    QColor glow;
};

static QVariantList xpColorsToList( const XPButtonColors& colors )
{
    QVariantList list;
    list << colors.border << colors.fillTop << colors.fillMid << colors.fillBottom << colors.glow;
    return list;
}

static XPButtonColors xpColorsFromList( const QVariantList& list, const XPButtonColors& fallback )
{
    if ( list.size() != 5 )
        return fallback;
    XPButtonColors colors;
    colors.border = list[ 0 ].value<QColor>();
    colors.fillTop = list[ 1 ].value<QColor>();
    colors.fillMid = list[ 2 ].value<QColor>();
    colors.fillBottom = list[ 3 ].value<QColor>();
    colors.glow = list[ 4 ].value<QColor>();
    return colors;
}

static XPButtonColors interpolateColors( const XPButtonColors& from, const XPButtonColors& to, double t )
{
    XPButtonColors colors;
    colors.border = blendColor( from.border, to.border, t );
    colors.fillTop = blendColor( from.fillTop, to.fillTop, t );
    colors.fillMid = blendColor( from.fillMid, to.fillMid, t );
    colors.fillBottom = blendColor( from.fillBottom, to.fillBottom, t );
    // blendColor() would interpolate an invalid color as black, so a default
    // ring that appears/disappears must not be faded through black; pick the
    // valid side directly instead.
    colors.glow = ( from.glow.isValid() && to.glow.isValid() )
        ? blendColor( from.glow, to.glow, t )
        : ( to.glow.isValid() ? to.glow : from.glow );
    return colors;
}

// State-transition animation for buttons. The whole XP "hover/press glow"
// effect is reproduced here as a color ramp animated by QVariantAnimation (a
// public API); no Windows theme engine is involved. The animation is stored on
// the widget through dynamic properties so a single style instance can track
// any number of buttons.
class XPButtonAnimation : public QVariantAnimation
{
public:
    explicit XPButtonAnimation( QObject* parent )
        : QVariantAnimation( parent )
    {
    }

    void updateCurrentValue( const QVariant& ) override
    {
        if ( QWidget* widget = qobject_cast<QWidget*>( parent() ) )
            widget->update();
    }
};

static void drawXPButtonShape( QPainter* painter, const QRect& rect, const XPButtonColors& colors )
{
    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    // Luna push buttons are rounded rectangles (about 2px corners), unlike
    // the sharp rects of the classic/base style.
    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
    gradient.setColorAt( 0.0, colors.fillTop );
    gradient.setColorAt( 0.55, colors.fillMid );
    gradient.setColorAt( 1.0, colors.fillBottom );
    painter->setPen( Qt::NoPen );
    painter->setBrush( gradient );
    painter->drawRoundedRect( rect, 2, 2 );

    painter->setPen( colors.border );
    painter->setBrush( Qt::NoBrush );
    painter->drawRoundedRect( rect.adjusted( 0, 0, -1, -1 ), 2, 2 );

    if ( colors.glow.isValid() ) {
        // XP default-button orange ring around the face.
        painter->setPen( colors.glow );
        painter->drawRoundedRect( rect.adjusted( -1, -1, 0, 0 ), 3, 3 );
    }
    painter->restore();
}

// (Re)starts the state-transition animation towards @p target. @p startColors
// is the ramp's starting point: the previously drawn state's colors on first
// launch, or the interpolated current frame when a running transition is
// re-targeted. Both call sites of drawXPButton share it.
static void startXPTransition( QWidget* widget, XPButtonAnimation* anim,
    XPButtonState target, const XPButtonColors& startColors )
{
    anim->setDuration( 200 );
    anim->setStartValue( 0.0 );
    anim->setEndValue( 1.0 );
    widget->setProperty( "_q_xp_btn_state", int( target ) );
    widget->setProperty( "_q_xp_btn_from", xpColorsToList( startColors ) );
    anim->start();
}

} // namespace

// The ring color of a "defaulted" button: Luna uses the deep button border
// (Blue #003C74), not the orange hot accent. Classic falls back to Highlight.
static QColor xpDefaultGlow( WinXPStyle::Mode mode, const QPalette& palette )
{
    switch ( mode ) {
        case WinXPStyle::Olive:
            return QColor( 0x05, 0x62, 0x06 );
        case WinXPStyle::Silver:
            return QColor( 0x6E, 0x6E, 0x6E );
        case WinXPStyle::Classic:
            return palette.color( QPalette::Highlight );
        default: // Blue
            return QColor( 0x00, 0x3C, 0x74 );
    }
}

// @p defaulted keeps the default-button ring on the Hot/Pressed states too,
// matching Luna where the ring survives the hover/press face transition.
static XPButtonColors xpButtonColors( WinXPStyle::Mode mode, const QPalette& palette,
    XPButtonState state, bool defaulted = false )
{
    switch ( state ) {
        case XPBS_Hot: {
            // Luna AccentColorHint rollover borders: Blue and Silver share the
            // classic orange 250,196,88, Olive (HomeStead) uses its own dark
            // orange 227,145,79, and Classic keeps the generic ramp border.
            QColor border;
            switch ( mode ) {
                case WinXPStyle::Olive:
                    border = QColor( 0xE3, 0x91, 0x4F );
                    break;
                case WinXPStyle::Classic:
                    border = QColor( 0xE9, 0x9C, 0x35 );
                    break;
                default: // Blue, Silver
                    border = QColor( 0xFA, 0xC4, 0x58 );
                    break;
            }
            return { border, QColor( 0xFF, 0xFD, 0xF4 ),
                QColor( 0xFF, 0xE8, 0xB8 ), QColor( 0xF7, 0xB2, 0x5E ),
                defaulted ? xpDefaultGlow( mode, palette ) : QColor() };
        }

        case XPBS_Pressed:
            return { QColor( 0xC8, 0x76, 0x0F ), QColor( 0xFC, 0xD3, 0x9B ),
                QColor( 0xFD, 0xBB, 0x6B ), QColor( 0xF2, 0x97, 0x3A ),
                defaulted ? xpDefaultGlow( mode, palette ) : QColor() };

        case XPBS_Disabled:
            switch ( mode ) {
                case WinXPStyle::Silver:
                    return { QColor( 0xC9, 0xC9, 0xC2 ), QColor( 0xF8, 0xF8, 0xF6 ),
                        QColor( 0xF0, 0xF0, 0xEC ), QColor( 0xE0, 0xE0, 0xDA ),
                        QColor() };
                case WinXPStyle::Olive:
                    return { QColor( 0xC6, 0xCD, 0xB4 ), QColor( 0xF6, 0xF7, 0xF0 ),
                        QColor( 0xEE, 0xF0, 0xE3 ), QColor( 0xDD, 0xE2, 0xCC ),
                        QColor() };
                case WinXPStyle::Classic:
                    return { palette.color( QPalette::Mid ), palette.color( QPalette::Button ),
                        palette.color( QPalette::Button ), palette.color( QPalette::Midlight ),
                        QColor() };
                default: // Blue
                    return { QColor( 0xC9, 0xC7, 0xB8 ), QColor( 0xF8, 0xF8, 0xF4 ),
                        QColor( 0xF0, 0xF0, 0xEA ), QColor( 0xE0, 0xDF, 0xD4 ),
                        QColor() };
            }

        case XPBS_Default:
        case XPBS_Normal: {
            // Only the default button carries the orange "defaulted" ring; the
            // other states keep glow invalid so drawXPButtonShape skips it.
            XPButtonColors colors;
            switch ( mode ) {
                case WinXPStyle::Silver:
                    // Metallic's exact button border is not available from the
                    // reference sources; use a neutral dark grey close to the
                    // theme's other border tones (HomeStead 5,98,6 / Luna 0,60,116
                    // are the corresponding deep edges for the other schemes).
                    colors = { QColor( 0x6E, 0x6E, 0x6E ), QColor( 0xFF, 0xFF, 0xFF ),
                        QColor( 0xE9, 0xE9, 0xE5 ), QColor( 0xC9, 0xC9, 0xC2 ),
                        QColor() };
                    break;
                case WinXPStyle::Olive:
                    colors = { QColor( 0x05, 0x62, 0x06 ), QColor( 0xFF, 0xFF, 0xFF ),
                        QColor( 0xE8, 0xED, 0xD8 ), QColor( 0xC3, 0xCD, 0x9F ),
                        QColor() };
                    break;
                case WinXPStyle::Classic: {
                    const QColor button = palette.color( QPalette::Button );
                    const QColor light = palette.color( QPalette::Light );
                    const QColor dark = palette.color( QPalette::Dark );
                    colors = { dark, light, blendColors( button, light, 0.5 ), button,
                        QColor() };
                    break;
                }
                default: // Blue — Luna FillColorHint 243,243,239 / Border 0,60,116
                    colors = { QColor( 0x00, 0x3C, 0x74 ), QColor( 0xFF, 0xFF, 0xFF ),
                        QColor( 0xF3, 0xF3, 0xEF ), QColor( 0xE5, 0xE4, 0xDA ),
                        QColor() };
                    break;
            }
            if ( state == XPBS_Default )
                colors.glow = xpDefaultGlow( mode, palette );
            return colors;
        }
    }
    Q_UNREACHABLE();
    return XPButtonColors();
}

WinXPStyle::WinXPStyle( Mode mode, bool forceClassicPalette )
    : QProxyStyle( QStyleFactory::create( QLatin1String("Windows") ) ),
      m_mode( mode ),
      m_forceClassicPalette( forceClassicPalette ),
      m_progressTimer( new QTimer( this ) )
{
    // The busy (indeterminate) progress bar is animated by a QTimer. A plain
    // startTimer() on the style would deliver timer events through
    // QProxyStyle::event(), which on Qt 5 forwards every event to the base
    // style, so timerEvent() would never fire; a QTimer's timeout signal
    // bypasses QStyle::event() entirely (same trick as the phase style).
    connect( m_progressTimer, &QTimer::timeout, this, &WinXPStyle::animateProgressBars );
}

WinXPStyle::~WinXPStyle()
{
}

/*!
    Returns the classic Windows XP Luna Blue Control Panel Colors as the
    suggested palette for the style.  This is a suggestion only -- Qt does
    not adopt it automatically.  The fixed Luna schemes (Blue/Silver/Olive)
    still only override selection tones in polish unless forceClassicPalette
    is set; the "winxp-classic" key forces this palette in full.
*/
QPalette WinXPStyle::standardPalette() const
{
    // Control Panel\Colors for the default Luna (Blue) theme.
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0xec, 0xe9, 0xd8));
    palette.setColor(QPalette::WindowText, QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::AlternateBase, QColor(0xec, 0xe9, 0xd8));
    palette.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xe1));
    palette.setColor(QPalette::ToolTipText, QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Text, QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Button, QColor(0xec, 0xe9, 0xd8));
    palette.setColor(QPalette::ButtonText, QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::Light, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::Midlight, QColor(0xf1, 0xef, 0xe2));
    palette.setColor(QPalette::Mid, QColor(0xac, 0xa8, 0x99));
    palette.setColor(QPalette::Dark, QColor(0x71, 0x6f, 0x64));
    palette.setColor(QPalette::Shadow, QColor(0x71, 0x6f, 0x64));
    palette.setColor(QPalette::Highlight, QColor(0x31, 0x6a, 0xc5));
    palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::PlaceholderText, QColor(0xac, 0xa8, 0x99));
    palette.setColor(QPalette::Link, QColor(0x00, 0x00, 0xff));
    palette.setColor(QPalette::LinkVisited, QColor(0x80, 0x00, 0x80));

    QtStyles::applyClassicDisabled(&palette);
    return palette;
}

void WinXPStyle::polish( QPalette& palette )
{
    QProxyStyle::polish( palette );

    // winxp-classic: install the full Luna Blue Control Panel Colors first so
    // Classic derivation (if ever used) and widget fills see the beige face.
    if ( m_forceClassicPalette )
        palette = standardPalette();

    // The original style probed the Windows theme engine (uxtheme.dll) to
    // pick the active Luna/Aero scheme. Blue, Silver and Olive reproduce the
    // fixed Windows XP "Luna" palettes; Classic derives the colors from the
    // palette passed in, so it follows the active QPalette.
    switch ( m_mode ) {
        case Blue:
            m_colorBackgroundBegin = QColor( 158, 190, 245 );
            m_colorBackgroundEnd = QColor( 196, 218, 250 );
            m_colorMenuBorder = QColor( 0, 45, 150 );
            m_colorMenuBackground = QColor( 246, 246, 246 );
            m_colorMenuTitleBegin = QColor( 227, 239, 255 );
            m_colorMenuTitleEnd = QColor( 123, 164, 224 );
            m_colorBarBegin = QColor( 227, 239, 255 );
            m_colorBarMiddle = QColor( 203, 225, 252 );
            m_colorBarEnd = QColor( 123, 164, 224 );
            m_colorHandle = QColor( 39, 65, 118 );
            m_colorHandleLight = QColor( 255, 255, 255 );
            m_colorSeparator = QColor( 106, 140, 203 );
            m_colorSeparatorLight = QColor( 241, 249, 255 );
            m_colorItemBorder = QColor( 0, 0, 128 );
            m_colorItemBackgroundBegin = QColor( 255, 238, 190 );
            m_colorItemBackgroundMiddle = QColor( 255, 225, 172 );
            m_colorItemBackgroundEnd = QColor( 255, 200, 125 );
            m_colorItemCheckedBegin = QColor( 255, 200, 125 );
            m_colorItemCheckedMiddle = QColor( 255, 180, 100 );
            m_colorItemCheckedEnd = QColor( 255, 150, 70 );
            m_colorItemSunkenBegin = QColor( 254, 128, 62 );
            m_colorItemSunkenMiddle = QColor( 255, 177, 109 );
            m_colorItemSunkenEnd = QColor( 255, 223, 154 );
            m_colorBorder = QColor( 68, 86, 134 );
            m_colorBorderLight = QColor( 106, 140, 203 );
            m_colors.fieldBorder = QColor( 0x7F, 0x9D, 0xB9 );
            m_colors.fieldBorderHot = QColor( 0x5E, 0x9B, 0xD1 );
            m_colors.fieldBackground = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.fieldBackgroundDisabled = QColor( 0xEB, 0xEB, 0xE4 );
            m_colors.fieldTextDisabled = QColor( 0x99, 0x99, 0x99 );
            m_colors.indicatorBorder = QColor( 0x1D, 0x52, 0x81 );
            m_colors.indicatorHotBorder = QColor( 0xFA, 0xC4, 0x58 );
            m_colors.indicatorCheck = QColor( 0x21, 0xA1, 0x21 );
            m_colors.indicatorDot = QColor( 0x0F, 0x1C, 0x43 );
            m_colors.arrowFaceBegin = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.arrowFaceEnd = QColor( 0xC8, 0xD6, 0xFB );
            m_colors.arrowBorder = QColor( 0x7A, 0x9E, 0xDB );
            m_colors.arrowGlyph = QColor( 0x00, 0x54, 0xE3 );
            m_colors.arrowGlyphDisabled = QColor( 0xA7, 0xA7, 0xA7 );
            m_colors.track = QColor( 0xF0, 0xF0, 0xF0 );
            m_colors.trackBorder = QColor( 0xB4, 0xB4, 0xB4 );
            m_colors.thumbFaceBegin = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.thumbFaceEnd = QColor( 0xBF, 0xCF, 0xEE );
            m_colors.thumbBorder = QColor( 0x31, 0x55, 0xA4 );
            // Gripper from the ReactOS Luna bitmaps (ScrollThumbGripperVertical):
            // normal #54839E, hot #6492AC, pressed #4D7791, disabled #374B53.
            m_colors.thumbGripper = QColor( 0x54, 0x83, 0x9E );
            m_colors.progressBorder = QColor( 0x68, 0x68, 0x68 );
            m_colors.progressGroove = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.progressChunk = QColor( 0x6C, 0xB8, 0x52 );
            m_colors.headerBegin = QColor( 0xFA, 0xF8, 0xF3 );
            m_colors.headerEnd = QColor( 0xCC, 0xDD, 0xF5 );
            m_colors.headerBorder = QColor( 0x9C, 0xB4, 0xD8 );
            m_colors.groupBoxBorder = QColor( 0xD0, 0xD0, 0xBF );
            m_colors.groupBoxBorderLight = QColor( 0xE0, 0xE0, 0xE0 );
            break;

        case Silver:
            m_colorBackgroundBegin = QColor( 215, 215, 229 );
            m_colorBackgroundEnd = QColor( 243, 243, 247 );
            m_colorMenuBorder = QColor( 124, 124, 148 );
            m_colorMenuBackground = QColor( 253, 250, 255 );
            m_colorMenuTitleBegin = QColor( 232, 233, 242 );
            m_colorMenuTitleEnd = QColor( 172, 170, 194 );
            m_colorBarBegin = QColor( 252, 252, 252 );
            m_colorBarMiddle = QColor( 232, 233, 242 );
            m_colorBarEnd = QColor( 172, 170, 194 );
            m_colorHandle = QColor( 84, 84, 117 );
            m_colorHandleLight = QColor( 255, 255, 255 );
            m_colorSeparator = QColor( 110, 109, 143 );
            m_colorSeparatorLight = QColor( 255, 255, 255 );
            m_colorItemBorder = QColor( 75, 75, 111 );
            m_colorItemBackgroundBegin = QColor( 232, 234, 243 );
            m_colorItemBackgroundMiddle = QColor( 221, 225, 239 );
            m_colorItemBackgroundEnd = QColor( 197, 204, 230 );
            m_colorItemCheckedBegin = QColor( 214, 218, 238 );
            m_colorItemCheckedMiddle = QColor( 202, 208, 232 );
            m_colorItemCheckedEnd = QColor( 187, 194, 224 );
            m_colorItemSunkenBegin = QColor( 178, 186, 220 );
            m_colorItemSunkenMiddle = QColor( 196, 202, 230 );
            m_colorItemSunkenEnd = QColor( 220, 224, 240 );
            m_colorBorder = QColor( 110, 109, 143 );
            m_colorBorderLight = QColor( 145, 144, 173 );
            m_colors.fieldBorder = QColor( 0xB8, 0xB8, 0xC0 );
            m_colors.fieldBorderHot = QColor( 0xA0, 0xA0, 0xB0 );
            m_colors.fieldBackground = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.fieldBackgroundDisabled = QColor( 0xEB, 0xEB, 0xE8 );
            m_colors.fieldTextDisabled = QColor( 0x99, 0x99, 0x99 );
            m_colors.indicatorBorder = QColor( 0x84, 0x84, 0x9C );
            m_colors.indicatorHotBorder = QColor( 0xF4, 0xC0, 0x5C );
            m_colors.indicatorCheck = QColor( 0x5C, 0xA0, 0x5C );
            m_colors.indicatorDot = QColor( 0x3A, 0x3A, 0x4E );
            m_colors.arrowFaceBegin = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.arrowFaceEnd = QColor( 0xD4, 0xD4, 0xDE );
            m_colors.arrowBorder = QColor( 0x99, 0x99, 0xAB );
            m_colors.arrowGlyph = QColor( 0x6E, 0x6E, 0x8A );
            m_colors.arrowGlyphDisabled = QColor( 0xB8, 0xB8, 0xC0 );
            m_colors.track = QColor( 0xEF, 0xEF, 0xF3 );
            m_colors.trackBorder = QColor( 0xC0, 0xC0, 0xCC );
            m_colors.thumbFaceBegin = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.thumbFaceEnd = QColor( 0xCF, 0xCF, 0xD9 );
            m_colors.thumbBorder = QColor( 0x6E, 0x6E, 0x8A );
            m_colors.thumbGripper = QColor( 0x8C, 0x8C, 0xA1 );
            m_colors.progressBorder = QColor( 0x68, 0x68, 0x68 );
            m_colors.progressGroove = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.progressChunk = QColor( 0x83, 0xBE, 0x62 );
            m_colors.headerBegin = QColor( 0xFA, 0xF8, 0xF3 );
            m_colors.headerEnd = QColor( 0xD8, 0xD8, 0xE2 );
            m_colors.headerBorder = QColor( 0xAD, 0xAD, 0xBE );
            m_colors.groupBoxBorder = QColor( 0xD0, 0xD0, 0xBF );
            m_colors.groupBoxBorderLight = QColor( 0xE8, 0xE8, 0xEE );
            break;

        case Olive:
            m_colorBackgroundBegin = QColor( 217, 217, 167 );
            m_colorBackgroundEnd = QColor( 242, 241, 228 );
            m_colorMenuBorder = QColor( 117, 141, 94 );
            m_colorMenuBackground = QColor( 244, 244, 238 );
            m_colorMenuTitleBegin = QColor( 237, 240, 214 );
            m_colorMenuTitleEnd = QColor( 181, 196, 143 );
            m_colorBarBegin = QColor( 255, 255, 237 );
            m_colorBarMiddle = QColor( 206, 220, 167 );
            m_colorBarEnd = QColor( 181, 196, 143 );
            m_colorHandle = QColor( 81, 94, 51 );
            m_colorHandleLight = QColor( 255, 255, 255 );
            m_colorSeparator = QColor( 96, 128, 88 );
            m_colorSeparatorLight = QColor( 244, 247, 222 );
            m_colorItemBorder = QColor( 63, 93, 56 );
            m_colorItemBackgroundBegin = QColor( 255, 238, 190 );
            m_colorItemBackgroundMiddle = QColor( 255, 225, 172 );
            m_colorItemBackgroundEnd = QColor( 255, 200, 125 );
            m_colorItemCheckedBegin = QColor( 255, 200, 125 );
            m_colorItemCheckedMiddle = QColor( 255, 180, 100 );
            m_colorItemCheckedEnd = QColor( 255, 150, 70 );
            m_colorItemSunkenBegin = QColor( 254, 128, 62 );
            m_colorItemSunkenMiddle = QColor( 255, 177, 109 );
            m_colorItemSunkenEnd = QColor( 255, 223, 154 );
            m_colorBorder = QColor( 96, 128, 88 );
            m_colorBorderLight = QColor( 151, 166, 123 );
            m_colors.fieldBorder = QColor( 0x8E, 0x9A, 0x5C );
            m_colors.fieldBorderHot = QColor( 0x7E, 0x8C, 0x4E );
            m_colors.fieldBackground = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.fieldBackgroundDisabled = QColor( 0xEC, 0xED, 0xDC );
            m_colors.fieldTextDisabled = QColor( 0x99, 0x99, 0x99 );
            m_colors.indicatorBorder = QColor( 0x7F, 0x8C, 0x4F );
            m_colors.indicatorHotBorder = QColor( 0xF4, 0xC0, 0x5C );
            m_colors.indicatorCheck = QColor( 0x5C, 0xA0, 0x5C );
            m_colors.indicatorDot = QColor( 0x2F, 0x36, 0x1E );
            m_colors.arrowFaceBegin = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.arrowFaceEnd = QColor( 0xDB, 0xE2, 0xBE );
            m_colors.arrowBorder = QColor( 0x99, 0xA3, 0x70 );
            m_colors.arrowGlyph = QColor( 0x5C, 0x66, 0x3A );
            m_colors.arrowGlyphDisabled = QColor( 0xB8, 0xBE, 0x9C );
            m_colors.track = QColor( 0xF2, 0xF2, 0xE4 );
            m_colors.trackBorder = QColor( 0xC2, 0xC8, 0x9E );
            m_colors.thumbFaceBegin = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.thumbFaceEnd = QColor( 0xD0, 0xD8, 0xA8 );
            m_colors.thumbBorder = QColor( 0x5B, 0x66, 0x44 );
            m_colors.thumbGripper = QColor( 0x9A, 0xA0, 0x6E );
            m_colors.progressBorder = QColor( 0x68, 0x68, 0x68 );
            m_colors.progressGroove = QColor( 0xFF, 0xFF, 0xFF );
            m_colors.progressChunk = QColor( 0x8E, 0xB8, 0x4C );
            m_colors.headerBegin = QColor( 0xFA, 0xF8, 0xF3 );
            m_colors.headerEnd = QColor( 0xDB, 0xE2, 0xBE );
            m_colors.headerBorder = QColor( 0xB0, 0xBA, 0x84 );
            m_colors.groupBoxBorder = QColor( 0xD0, 0xD0, 0xBF );
            m_colors.groupBoxBorderLight = QColor( 0xE4, 0xE8, 0xCE );
            break;

        case Classic:
            m_colorBackgroundBegin = palette.color( QPalette::Button );
            m_colorBackgroundEnd = blendRoles( palette, QPalette::Button, QPalette::Base, 0.205 );
            m_colorMenuBorder = blendRoles( palette, QPalette::Text, QPalette::Dark, 0.2 );
            m_colorMenuBackground = blendRoles( palette, QPalette::Button, QPalette::Base, 0.143 );
            m_colorMenuTitleBegin = blendRoles( palette, QPalette::Button, QPalette::Base, 0.2 );
            m_colorMenuTitleEnd = blendRoles( palette, QPalette::Button, QPalette::Base, 0.5 );
            m_colorBarBegin = blendRoles( palette, QPalette::Button, QPalette::Base, 0.2 );
            m_colorBarMiddle = blendRoles( palette, QPalette::Button, QPalette::Base, 0.5 );
            m_colorBarEnd = palette.color( QPalette::Button );
            m_colorHandle = blendRoles( palette, QPalette::Dark, QPalette::Base, 0.75 );
            m_colorHandleLight = palette.color( QPalette::Light );
            m_colorSeparator = blendRoles( palette, QPalette::Dark, QPalette::Base, 0.7 );
            m_colorSeparatorLight = palette.color( QPalette::Light );
            m_colorItemBorder = palette.color( QPalette::Highlight );
            m_colorItemBackgroundBegin = blendRoles( palette, QPalette::Highlight, QPalette::Base, 0.3 );
            m_colorItemBackgroundMiddle = m_colorItemBackgroundEnd = m_colorItemBackgroundBegin;
            m_colorItemCheckedBegin = blendRoles( palette, QPalette::Highlight, QPalette::Base, 0.15 );
            m_colorItemCheckedMiddle = m_colorItemCheckedEnd = m_colorItemCheckedBegin;
            m_colorItemSunkenBegin = blendRoles( palette, QPalette::Highlight, QPalette::Base, 0.45 );
            m_colorItemSunkenMiddle = m_colorItemSunkenEnd = m_colorItemSunkenBegin;
            m_colorBorder = palette.color( QPalette::Dark );
            m_colorBorderLight = blendRoles( palette, QPalette::Dark, QPalette::Base, 0.8 );
            m_colors.fieldBorder = blendRoles( palette, QPalette::Dark, QPalette::Base, 0.7 );
            m_colors.fieldBorderHot = palette.color( QPalette::Highlight );
            m_colors.fieldBackground = palette.color( QPalette::Base );
            m_colors.fieldBackgroundDisabled = blendRoles( palette, QPalette::Button, QPalette::Base, 0.5 );
            m_colors.fieldTextDisabled = palette.color( QPalette::Disabled, QPalette::Text );
            m_colors.indicatorBorder = palette.color( QPalette::Dark );
            m_colors.indicatorHotBorder = palette.color( QPalette::Highlight );
            m_colors.indicatorCheck = palette.color( QPalette::Highlight );
            m_colors.indicatorDot = palette.color( QPalette::Text );
            m_colors.arrowFaceBegin = palette.color( QPalette::Light );
            m_colors.arrowFaceEnd = blendRoles( palette, QPalette::Button, QPalette::Light, 0.6 );
            m_colors.arrowBorder = palette.color( QPalette::Dark );
            m_colors.arrowGlyph = palette.color( QPalette::Text );
            m_colors.arrowGlyphDisabled = palette.color( QPalette::Disabled, QPalette::Text );
            m_colors.track = palette.color( QPalette::Button );
            m_colors.trackBorder = palette.color( QPalette::Mid );
            m_colors.thumbFaceBegin = palette.color( QPalette::Light );
            m_colors.thumbFaceEnd = blendRoles( palette, QPalette::Button, QPalette::Light, 0.6 );
            m_colors.thumbBorder = palette.color( QPalette::Dark );
            m_colors.thumbGripper = blendRoles( palette, QPalette::Dark, QPalette::Base, 0.5 );
            m_colors.progressBorder = palette.color( QPalette::Dark );
            m_colors.progressGroove = palette.color( QPalette::Base );
            m_colors.progressChunk = palette.color( QPalette::Highlight );
            m_colors.headerBegin = palette.color( QPalette::Button );
            m_colors.headerEnd = blendRoles( palette, QPalette::Button, QPalette::Light, 0.5 );
            m_colors.headerBorder = palette.color( QPalette::Mid );
            m_colors.groupBoxBorder = palette.color( QPalette::Dark );
            m_colors.groupBoxBorderLight = palette.color( QPalette::Midlight );
            break;
    }

    // Dialog-faithful chrome for winxp-classic: property sheets use a flat
    // beige client, white menus, solid Luna-blue selection (no Office candy
    // stripe), and Luna rebar colours for toolbars / dock titles / toolboxes —
    // not the Office 2003 blue candy bars that Mode::Blue assigns above.
    // Rebar FillColorHint from ReactOS Luna: 241,243,239 (not a QPalette role).
    if ( m_forceClassicPalette ) {
        const QColor face = palette.color( QPalette::Window );
        const QColor rebar( 0xf1, 0xf3, 0xef );
        const QColor sel = palette.color( QPalette::Highlight );
        const QColor dark = palette.color( QPalette::Dark );
        m_colorBackgroundBegin = m_colorBackgroundEnd = face;
        m_colorBarBegin = Qt::white;
        m_colorBarMiddle = rebar;
        m_colorBarEnd = face;
        m_colorBorder = dark;
        m_colorBorderLight = QColor( 0xd0, 0xd0, 0xbf );
        m_colorHandle = dark;
        m_colorHandleLight = Qt::white;
        m_colorSeparator = palette.color( QPalette::Mid );
        m_colorSeparatorLight = Qt::white;
        m_colorMenuBackground = Qt::white;
        m_colorMenuBorder = dark;
        m_colorItemBorder = sel;
        m_colorItemBackgroundBegin = m_colorItemBackgroundMiddle = m_colorItemBackgroundEnd = sel;
        m_colorItemCheckedBegin = m_colorItemCheckedMiddle = m_colorItemCheckedEnd = sel;
        m_colorItemSunkenBegin = m_colorItemSunkenMiddle = m_colorItemSunkenEnd = QColor( 0x21, 0x5d, 0xc6 );
        // standardPalette() already has the full Luna set; keep disabled tones.
        palette.setColor( QPalette::Disabled, QPalette::Text, m_colors.fieldTextDisabled );
        palette.setColor( QPalette::Disabled, QPalette::WindowText, m_colors.fieldTextDisabled );
        palette.setColor( QPalette::Disabled, QPalette::ButtonText, m_colors.fieldTextDisabled );
        return;
    }

    // Item views draw their selection with QPalette::Highlight; give each
    // Luna scheme its own selection tone instead of the default application
    // blue. The Luna schemes also get XP's disabled text tone (TextColor
    // 153,153,153) for disabled controls. Classic leaves the incoming
    // palette alone entirely.
    switch ( m_mode ) {
        case Blue:
            palette.setColor( QPalette::Highlight, QColor( 0x31, 0x6A, 0xC5 ) );
            break;
        case Silver:
            palette.setColor( QPalette::Highlight, QColor( 0x8E, 0x92, 0xB8 ) );
            break;
        case Olive:
            palette.setColor( QPalette::Highlight, QColor( 0x84, 0x9B, 0x4C ) );
            break;
        case Classic:
        default:
            return;
    }
    palette.setColor( QPalette::HighlightedText, Qt::white );
    palette.setColor( QPalette::Disabled, QPalette::Text, m_colors.fieldTextDisabled );
    palette.setColor( QPalette::Disabled, QPalette::WindowText, m_colors.fieldTextDisabled );
    palette.setColor( QPalette::Disabled, QPalette::ButtonText, m_colors.fieldTextDisabled );
}

// A rounded tab widget (inside any window) is drawn with the XP look for all
// four tab positions. The base style draws tabs by calling its own
// drawControl/drawPrimitive internally, so CE_TabBarTab/CE_TabBarTabShape/
// PE_FrameTabWidget/... must be intercepted as a whole (see
// drawControl/drawPrimitive).
static const QTabWidget* isStyledTabWidget( const QWidget* widget )
{
    const QTabWidget* tabWidget = qobject_cast<const QTabWidget*>( widget );
    if ( tabWidget && tabWidget->tabShape() == QTabWidget::Rounded )
        return tabWidget;
    return nullptr;
}

static const QTabBar* isStyledTabBar( const QWidget* widget )
{
    const QTabBar* tabBar = qobject_cast<const QTabBar*>( widget );
    if ( tabBar && ( tabBar->shape() == QTabBar::RoundedNorth || tabBar->shape() == QTabBar::RoundedSouth
            || tabBar->shape() == QTabBar::RoundedWest || tabBar->shape() == QTabBar::RoundedEast ) ) {
        return tabBar;
    }
    return nullptr;
}

static bool isToolBoxButton( const QWidget* widget )
{
    const QAbstractButton* button = qobject_cast<const QAbstractButton*>( widget );
    if ( button && qobject_cast<const QToolBox*>( button->parentWidget() ) )
        return true;
    return false;
}

static bool isToolBoxPanel( const QWidget* widget )
{
    const QWidget* viewport = widget->parentWidget();
    if ( viewport ) {
        const QScrollArea* scrollArea = qobject_cast<const QScrollArea*>( viewport->parentWidget() );
        if ( scrollArea && qobject_cast<const QToolBox*>( scrollArea->parentWidget() ) )
            return true;
    }
    return false;
}

// Common controls whose XP hover feedback (orange borders/faces) needs hover
// events; shared by polish()/unpolish().
static bool needsXPHover( const QWidget* widget )
{
    return qobject_cast<const QCheckBox*>( widget ) || qobject_cast<const QRadioButton*>( widget )
        || qobject_cast<const QSlider*>( widget ) || qobject_cast<const QScrollBar*>( widget )
        || qobject_cast<const QLineEdit*>( widget ) || qobject_cast<const QComboBox*>( widget )
        || qobject_cast<const QAbstractSpinBox*>( widget ) || qobject_cast<const QHeaderView*>( widget );
}

void WinXPStyle::polish( QWidget* widget )
{
    if ( qobject_cast<QMainWindow*>( widget ) )
        widget->setAttribute( Qt::WA_StyledBackground );

    if ( qobject_cast<QToolBox*>( widget ) ) {
        widget->setAttribute( Qt::WA_StyledBackground );
        widget->layout()->setContentsMargins( 2, 2, 2, 2 );
    }

    if ( isToolBoxButton( widget ) ) {
        widget->setAttribute( Qt::WA_Hover );
        QSizePolicy policy = widget->sizePolicy();
        policy.setControlType( QSizePolicy::PushButton );
        widget->setSizePolicy( policy );
    }

    if ( isToolBoxPanel( widget ) )
        widget->setAttribute( Qt::WA_StyledBackground );

    if ( isStyledTabBar( widget ) )
        widget->setAttribute( Qt::WA_Hover );

    // XP hover feedback (orange borders/faces) needs hover events on the
    // common controls that are now drawn by the style itself.
    if ( needsXPHover( widget ) )
        widget->setAttribute( Qt::WA_Hover );

    if ( QProgressBar* bar = qobject_cast<QProgressBar*>( widget ) ) {
        widget->installEventFilter( this );
        // Already-visible bars will not get a Show event, so register them
        // right away; invisible ones are picked up when they are shown.
        // Only the (0, 0) range is busy for the drawing code, so track that
        // exact case instead of any min == max range.
        if ( bar->isVisible() && bar->minimum() == 0 && bar->maximum() == 0 )
            addProgressBar( bar );
    }

    QProxyStyle::polish( widget );
}

void WinXPStyle::unpolish( QWidget* widget )
{
    if ( XPButtonAnimation* anim = static_cast<XPButtonAnimation*>(
            widget->property( "_q_xp_btn_anim" ).value<void*>() ) ) {
        anim->stop();
        delete anim;
        widget->setProperty( "_q_xp_btn_anim", QVariant() );
        widget->setProperty( "_q_xp_btn_from", QVariantList() );
    }

    if ( qobject_cast<QMainWindow*>( widget ) )
        widget->setAttribute( Qt::WA_StyledBackground, false );

    if ( qobject_cast<QToolBox*>( widget ) )
        widget->setAttribute( Qt::WA_StyledBackground, false );

    if ( isToolBoxButton( widget ) )
        widget->setAttribute( Qt::WA_Hover, false );

    if ( isToolBoxPanel( widget ) )
        widget->setAttribute( Qt::WA_StyledBackground, false );

    if ( isStyledTabBar( widget ) )
        widget->setAttribute( Qt::WA_Hover, false );

    if ( needsXPHover( widget ) )
        widget->setAttribute( Qt::WA_Hover, false );

    if ( qobject_cast<QProgressBar*>( widget ) ) {
        widget->removeEventFilter( this );
        removeProgressBar( static_cast<QProgressBar*>( widget ) );
    }

    QProxyStyle::unpolish( widget );
}

int WinXPStyle::pixelMetric( PixelMetric metric, const QStyleOption* option, const QWidget* widget ) const
{
    switch ( metric ) {
        case PM_MenuBarPanelWidth:
            return 0;
        case PM_MenuBarVMargin:
        case PM_MenuBarHMargin:
            return 2;
        case PM_MenuPanelWidth:
            return 1;
        case PM_MenuHMargin:
            return 0;
        case PM_MenuVMargin:
            return 1;

        case PM_ToolBarFrameWidth:
            return 2;
        case PM_ToolBarItemMargin:
        case PM_ToolBarItemSpacing:
            return 0;
        case PM_ToolBarIconSize:
            return qRound( QStyleHelper::dpiScaled( 16, option ) );

        case PM_MenuButtonIndicator:
            return qRound( QStyleHelper::dpiScaled( 12, option ) );

        case PM_ScrollBarExtent:
            return qRound( QStyleHelper::dpiScaled( 17, option ) );
        case PM_SliderThickness:
            return qRound( QStyleHelper::dpiScaled( 22, option ) );
        case PM_SplitterWidth:
            return qRound( QStyleHelper::dpiScaled( 5, option ) );
        case PM_ComboBoxFrameWidth:
        case PM_SpinBoxFrameWidth:
            return 1;

        case PM_ButtonShiftVertical:
        case PM_ButtonShiftHorizontal:
            if ( widget && qobject_cast<QToolBar*>( widget->parentWidget() ) )
                return 0;
            break;

        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
            // Luna draws the radio and the checkbox in the same 13x13 cell.
            // The base style sizes the radio at 12x12 (a pixel smaller and
            // lower than the checkbox) and scales the checkbox by DPI, so a
            // fixed 13 keeps both indicators identical on every screen, in
            // line with the rest of this style's fixed-pixel metrics.
            return qRound( QStyleHelper::dpiScaled( 13, option ) );

        case PM_DockWidgetSeparatorExtent:
            return qRound( QStyleHelper::dpiScaled( 4, option ) );
        case PM_DockWidgetTitleBarButtonMargin:
            return qRound( QStyleHelper::dpiScaled( 4, option ) );
        case PM_DockWidgetTitleMargin:
            return qRound( QStyleHelper::dpiScaled( 3, option ) );

        case PM_LayoutVerticalSpacing:
            if ( qobject_cast<const QToolBox*>( widget ) )
                return -1;
            break;

        case PM_TabBarBaseOverlap:
            if ( isStyledTabWidget( widget ) || isStyledTabBar( widget ) )
                return 0;
            break;
        case PM_TabBarTabShiftVertical:
            if ( const QTabBar* tabBar = isStyledTabBar( widget ) )
                return ( tabBar->shape() == QTabBar::RoundedSouth ) ? -2 : 2;
            break;
        case PM_TabBarTabHSpace:
            // The Windows base value (24) pads the label of a vertical tab
            // further than the narrow strip needs; 12 keeps the tab width
            // compact on both orientations.
            return qRound( QStyleHelper::dpiScaled( 12, option ) );

        case PM_HeaderMarkSize:
            // Padding box around the fixed 7×7 header chevron. Mark may grow
            // with DPI; the glyph stays 7×7 (see PE_IndicatorHeaderArrow).
            return qRound( QStyleHelper::dpiScaled( 9, option ) );

        default:
            break;
    }

    return QProxyStyle::pixelMetric( metric, option, widget );
}

QSize WinXPStyle::sizeFromContents( ContentsType type, const QStyleOption* option,
    const QSize& contentsSize, const QWidget* widget ) const
{
    switch ( type ) {
        case CT_MenuBar:
            return contentsSize - QSize( 0, 1 );

        case CT_Menu:
            return contentsSize;

        case CT_MenuBarItem:
            return contentsSize + QSize( 16, 6 );

        case CT_MenuItem:
            if ( const QStyleOptionMenuItem* menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) ) {
                if ( menuItem->menuItemType == QStyleOptionMenuItem::Separator )
                    return QSize( 10, 3 );
                int space = 32 + 16;
                if ( menuItem->text.contains( '\t' ) )
                    space += 12;
                return QSize( contentsSize.width() + space, 22 );
            }
            break;

        case CT_TabBarTab: {
            QSize sz = QProxyStyle::sizeFromContents( type, option, contentsSize, widget );
            // QTabBar::tabSizeHint already transposes the content size for
            // vertical tabs (width = text height, height = text width) and
            // QTabBar::layoutTabs consumes the returned size as-is, so the
            // shape comes out as a tall narrow strip. A further transpose here
            // would flip it back into a wide flat tab.
            return sz;
        }

        case CT_SpinBox: {
            QSize sz = QProxyStyle::sizeFromContents( type, option, contentsSize, widget );
            // A calendar-popup QDateTimeEdit is drawn as a combo box; size it
            // like one so it lines up flush with neighbouring input widgets
            // instead of coming out a couple of pixels shorter. QComboBox::sizeHint
            // bases its content height on ceil(fontMetricsF().height())+2, while
            // QDateTimeEdit drives it from its internal editor, which is one
            // pixel shorter.
            const QDateTimeEdit* dte = qobject_cast<const QDateTimeEdit*>( widget );
            if ( dte && dte->calendarPopup() ) {
                const int contentH = qMax( qCeil( QFontMetricsF( dte->fontMetrics() ).height() ), 14 ) + 2;
                QStyleOptionComboBox copt;
                copt.initFrom( dte );
                if ( const QStyleOptionSpinBox* spin = qstyleoption_cast<const QStyleOptionSpinBox*>( option ) )
                    copt.frame = spin->frame;
                const QSize comboSize = QProxyStyle::sizeFromContents( CT_ComboBox, &copt,
                    QSize( contentsSize.width(), contentH ), dte );
                sz.setHeight( comboSize.height() );
            }
            return sz;
        }

        default:
            break;
    }

    return QProxyStyle::sizeFromContents( type, option, contentsSize, widget );
}

QRect WinXPStyle::subElementRect( SubElement element, const QStyleOption* option, const QWidget* widget ) const
{
    QRect rect = QProxyStyle::subElementRect( element, option, widget );

    switch ( element ) {
        case SE_DockWidgetCloseButton:
        case SE_DockWidgetFloatButton:
            rect.translate( -2, 0 );
            break;

        case SE_TabWidgetTabContents:
            if ( isStyledTabWidget( widget ) )
                rect = QCommonStyle::subElementRect( SE_TabWidgetTabPane, option, widget );
            break;

        case SE_TabWidgetTabBar:
            if ( isStyledTabWidget( widget ) )
                rect = QCommonStyle::subElementRect( SE_TabWidgetTabBar, option, widget );
            break;

        case SE_ProgressBarGroove:
        case SE_ProgressBarLabel:
            // The base Windows style keys off QStyleOptionProgressBar::bottomToTop
            // for direction, which QProgressBar no longer sets on Qt 6, so it
            // hands back broken rects (negative width on vertical bars, a narrow
            // right-hand strip for the label). The whole bar is drawn by this
            // style, so both span the full option rect.
            rect = option->rect;
            break;

        case SE_ProgressBarContents:
            rect = option->rect.adjusted( 2, 2, -2, -2 );
            break;

        case SE_HeaderArrow: {
            // QCommonStyle sizes this as half the section height — far too
            // large for our 7×7 Luna chevron, and on tight columns the glyph
            // then sits on top of the label. Pin a mark-sized square on the
            // trailing edge, vertically centered.
            if ( const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>( option ) ) {
                if ( header->sortIndicator == QStyleOptionHeader::None ) {
                    rect = QRect();
                    break;
                }
            }
            const int margin = pixelMetric( PM_HeaderMargin, option, widget );
            const int mark = pixelMetric( PM_HeaderMarkSize, option, widget );
            QRect markRect( 0, 0, mark, mark );
            const bool horizontal = option->state & State_Horizontal;
            if ( horizontal ) {
                markRect.moveCenter( QPoint(
                    option->rect.right() - margin - mark / 2,
                    option->rect.center().y() ) );
            } else {
                markRect.moveCenter( QPoint(
                    option->rect.center().x(),
                    option->rect.bottom() - margin - mark / 2 ) );
            }
            rect = visualRect( option->direction, option->rect, markRect );
            break;
        }

        case SE_HeaderLabel: {
            // Pair with SE_HeaderArrow: reserve mark+margin instead of the
            // CommonStyle half-height carve-out.
            const int margin = pixelMetric( PM_HeaderMargin, option, widget );
            rect = option->rect.adjusted( margin, margin, -margin, -margin );
            if ( const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>( option ) ) {
                if ( header->sortIndicator != QStyleOptionHeader::None ) {
                    const int mark = pixelMetric( PM_HeaderMarkSize, option, widget );
                    if ( option->state & State_Horizontal )
                        rect.setWidth( qMax( 0, rect.width() - mark - margin ) );
                    else
                        rect.setHeight( qMax( 0, rect.height() - mark - margin ) );
                }
            }
            rect = visualRect( option->direction, option->rect, rect );
            break;
        }

        default:
            break;
    }

    return rect;
}

int WinXPStyle::layoutSpacing( QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
    Qt::Orientation orientation, const QStyleOption* option, const QWidget* widget ) const
{
    Q_UNUSED( orientation );
    Q_UNUSED( option );

    if ( qobject_cast<const QToolBox*>( widget ) ) {
        if ( control1 == QSizePolicy::PushButton && control2 == QSizePolicy::DefaultType )
            return 0;
        else if ( control1 == QSizePolicy::DefaultType && control2 == QSizePolicy::PushButton )
            return 3;
        else
            return 2;
    }

    return 6;
}

void WinXPStyle::drawXPButton( QPainter* painter, const QStyleOption* option ) const
{
    const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option );
    if ( !btn || option->rect.width() < 4 || option->rect.height() < 4 )
        return;

    const QRect rect = option->rect;

    const bool defaulted = btn->features & QStyleOptionButton::DefaultButton;
    XPButtonState target = XPBS_Normal;
    if ( !( option->state & QStyle::State_Enabled ) )
        target = XPBS_Disabled;
    else if ( option->state & ( QStyle::State_Sunken | QStyle::State_On ) )
        target = XPBS_Pressed;
    else if ( option->state & QStyle::State_MouseOver )
        target = XPBS_Hot;
    else if ( defaulted )
        target = XPBS_Default;

    XPButtonColors to = xpButtonColors( m_mode, option->palette, target, defaulted );
    XPButtonColors colors = to;

    if ( QWidget* widget = qobject_cast<QWidget*>( option->styleObject ) ) {
        // First draw: seed the recorded state with the current target so a
        // later state change has a correct animation starting point. A missing
        // _q_xp_btn_state property marks "never drawn".
        if ( !widget->property( "_q_xp_btn_state" ).isValid() ) {
            widget->setProperty( "_q_xp_btn_state", int( target ) );
            widget->setProperty( "_q_xp_btn_from", xpColorsToList( to ) );
        }
        int current = widget->property( "_q_xp_btn_state" ).toInt();
        XPButtonAnimation* anim = static_cast<XPButtonAnimation*>(
            widget->property( "_q_xp_btn_anim" ).value<void*>() );

        if ( anim && anim->state() == QAbstractAnimation::Running ) {
            // Mid-transition: show the interpolated colors, re-target if the
            // state changed while the previous transition was running.
            const double progress = anim->currentValue().toReal();
            const XPButtonColors from = xpColorsFromList(
                widget->property( "_q_xp_btn_from" ).toList(), to );
            const XPButtonColors oldTo = xpButtonColors( m_mode, option->palette, (XPButtonState)current, defaulted );
            colors = interpolateColors( from, oldTo, progress );
            if ( current != int( target ) ) {
                // start() restarts a running animation from zero.
                startXPTransition( widget, anim, target, colors );
            }
        } else if ( current != int( target ) ) {
            // Begin a transition from the previously drawn state.
            const XPButtonColors from = xpButtonColors( m_mode, option->palette, (XPButtonState)current, defaulted );
            if ( !anim ) {
                anim = new XPButtonAnimation( widget );
                widget->setProperty( "_q_xp_btn_anim", QVariant::fromValue<void*>( anim ) );
                QObject::connect( anim, &QAbstractAnimation::finished, widget, [ widget, anim ]() {
                    widget->setProperty( "_q_xp_btn_anim", QVariant() );
                    widget->setProperty( "_q_xp_btn_from", QVariantList() );
                    anim->deleteLater();
                } );
            }
            startXPTransition( widget, anim, target, from );
            colors = from;
        }
    }

    // The painter clips to the widget's rect, so the default-button ring is
    // drawn on the inside edge: the face is inset by one pixel to leave room.
    QRect faceRect = rect;
    if ( colors.glow.isValid() )
        faceRect.adjust( 1, 1, -1, -1 );

    drawXPButtonShape( painter, faceRect, colors );
}

// Draws a small filled glyph arrow (used on combo/scroll/spin arrow buttons).
// @p center is the button center; the glyph is a fixed size there regardless
// of button dimensions.
//
// @p glyphWidth/@p glyphHeight match the real Luna bitmap each control uses:
//   ComboButtonGlyph 9x7 (combobox drop-down), ScrollArrowGlyphs 9x9
//   (scrollbar), SpinGlyphs 7x7 (spinbox). Each glyph is a fixed bitmap
//   centered in its button; it never scales with the button.
//
// The triangle is drawn pixel-by-pixel (fillRect, no antialiasing) following
// the exact row widths of the ReactOS Lunar bitmaps (see comments in the
// width tables below), so every size is crisp and identical to the original
// pixmaps instead of an antialiased polygon that blurs the edges.
static void drawXPGlyphArrow( QPainter* painter, const QPoint& center, int glyphWidth, int glyphHeight,
    Qt::ArrowType direction, const QColor& color )
{
    painter->save();
    painter->setPen( Qt::NoPen );
    painter->setBrush( color );

    const int left = center.x() - glyphWidth / 2;
    const int top = center.y() - glyphHeight / 2;

    // Row widths (top to bottom) of each control's source bitmap:
    //   SpinGlyphs 7x7:            { 1, 3, 5 }   (tip at top for UpArrow)
    //   ComboButtonGlyph 9x7:      { 1, 3, 5, 7 }
    //   ScrollArrowGlyphs 9x9:     { 1, 3, 5, 7, 9 }
    // Vertical arrows use these directly (reversed for DownArrow); horizontal
    // arrows transpose them to columns.
    const int* widths = nullptr;
    int count = 0;
    static const int spinW[3]   = { 1, 3, 5 };
    static const int comboW[4]  = { 1, 3, 5, 7 };
    static const int scrollW[5] = { 1, 3, 5, 7, 9 };
    if ( glyphWidth == 7 && glyphHeight == 7 )      { widths = spinW;   count = 3; }
    else if ( glyphWidth == 9 && glyphHeight == 7 ) { widths = comboW;  count = 4; }
    else if ( glyphWidth == 9 && glyphHeight == 9 ) { widths = scrollW; count = 5; }

    const bool vertical = direction == Qt::UpArrow || direction == Qt::DownArrow;
    const bool reversed = direction == Qt::DownArrow || direction == Qt::RightArrow;

    if ( !widths ) {
        // Unknown size: fall back to a plain isoceles triangle filling the
        // glyph box, centered.
        const int steps = ( ( vertical ? glyphHeight : glyphWidth ) + 1 ) / 2;
        for ( int i = 0; i < steps; ++i ) {
            const int half = reversed ? ( steps - 1 - i ) : i;
            const int size = half * 2 + 1;
            if ( vertical ) {
                const int y = top + i;
                painter->drawRect( center.x() - size / 2, y, size, 1 );
            } else {
                const int x = left + i;
                painter->drawRect( x, center.y() - size / 2, 1, size );
            }
        }
    } else {
        for ( int i = 0; i < count; ++i ) {
            const int size = reversed ? widths[ count - 1 - i ] : widths[ i ];
            if ( vertical ) {
                // Vertical centering within the glyph box: leave equal empty
                // rows above and below the triangle.
                const int y = top + ( glyphHeight - count ) / 2 + i;
                painter->drawRect( center.x() - size / 2, y, size, 1 );
            } else {
                const int x = left + ( glyphWidth - count ) / 2 + i;
                painter->drawRect( x, center.y() - size / 2, 1, size );
            }
        }
    }
    painter->restore();
}

void WinXPStyle::drawXPEditField( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hot = option->state & ( QStyle::State_HasFocus | QStyle::State_MouseOver );

    QColor border;
    if ( !enabled )
        border = m_colors.fieldBackgroundDisabled;
    else if ( hot )
        border = m_colors.fieldBorderHot;
    else
        border = m_colors.fieldBorder;

    painter->fillRect( rect, enabled ? m_colors.fieldBackground : m_colors.fieldBackgroundDisabled );
    painter->setPen( border );
    painter->setBrush( Qt::NoBrush );
    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
}

void WinXPStyle::drawXPIndicator( QPainter* painter, const QStyleOption* option, bool radio, bool checked ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 3 || rect.height() < 3 )
        return;

    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hot = option->state & QStyle::State_MouseOver;
    const bool tri = option->state & QStyle::State_NoChange;

    QColor border;
    if ( !enabled )
        border = blendColors( m_colors.indicatorBorder, m_colors.fieldBackgroundDisabled, 0.6 );
    else if ( hot )
        border = m_colors.indicatorHotBorder;
    else
        border = m_colors.indicatorBorder;

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing, radio );

    if ( radio ) {
        painter->setBrush( enabled ? m_colors.fieldBackground : m_colors.fieldBackgroundDisabled );
        painter->setPen( QPen( border, 1.4 ) );
        // The cell is an odd 13x13, so an even 12x12 ellipse on integer
        // coordinates sits half a pixel up-left of centre and the ring-dot gap
        // comes out wider on one side. Centering the ellipse on the half-pixel
        // boundary keeps the ring symmetric (same idiom as the checkbox frame).
        painter->drawEllipse( QRectF( rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ) );
        if ( checked ) {
            // A disabled checked radio keeps the dot, grayed out (Luna does too).
            QRect dot = rect.adjusted( rect.width() / 4, rect.height() / 4,
                -rect.width() / 4, -rect.height() / 4 );
            painter->setPen( Qt::NoPen );
            painter->setBrush( enabled ? m_colors.indicatorDot
                                       : blendColors( m_colors.indicatorDot, m_colors.fieldBackgroundDisabled, 0.6 ) );
            painter->drawEllipse( dot );
        }
    } else {
        painter->setRenderHint( QPainter::Antialiasing, false );
        painter->setPen( Qt::NoPen );
        painter->setBrush( enabled ? m_colors.fieldBackground : m_colors.fieldBackgroundDisabled );
        painter->drawRect( rect );
        painter->setPen( QPen( border, 1.0 ) );
        painter->setBrush( Qt::NoBrush );
        // A full-pixel pen drawn on integer coordinates overflows the frame
        // rect on the right/bottom while leaving the left/top edge visible,
        // so the four borders came out uneven. Centering the line on the
        // half-pixel boundary gives a clean 1px border on every side.
        painter->drawRect( QRectF( rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ) );

        if ( checked || tri ) {
            painter->setRenderHint( QPainter::Antialiasing, true );
            const QColor checkColor = enabled ? m_colors.indicatorCheck
                                              : blendColors( m_colors.indicatorCheck, m_colors.fieldBackgroundDisabled, 0.6 );
            painter->setPen( QPen( checkColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            if ( tri ) {
                painter->drawLine( QPointF( rect.left() + 3.5, rect.center().y() ),
                    QPointF( rect.right() - 3.5, rect.center().y() ) );
            } else {
                QPainterPath path;
                path.moveTo( rect.left() + 2.5, rect.top() + rect.height() / 2 - 0.5 );
                path.lineTo( rect.center().x() - 0.5, rect.bottom() - 2.5 );
                path.lineTo( rect.right() - 2, rect.top() + 2.5 );
                painter->drawPath( path );
            }
        }
    }
    painter->restore();
}

void WinXPStyle::drawXPButtonFace( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 4 || rect.height() < 4 )
        return;

    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hot = option->state & QStyle::State_MouseOver;
    const bool sunken = option->state & ( QStyle::State_Sunken | QStyle::State_On );

    QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
    if ( sunken ) {
        gradient.setColorAt( 0.0, m_colors.arrowFaceEnd );
        gradient.setColorAt( 1.0, m_colors.arrowFaceBegin );
    } else if ( hot && enabled ) {
        gradient.setColorAt( 0.0, blendColors( m_colors.arrowFaceBegin, m_colors.indicatorHotBorder, 0.25 ) );
        gradient.setColorAt( 1.0, blendColors( m_colors.arrowFaceEnd, m_colors.indicatorHotBorder, 0.25 ) );
    } else {
        gradient.setColorAt( 0.0, m_colors.arrowFaceBegin );
        gradient.setColorAt( 1.0, m_colors.arrowFaceEnd );
    }
    painter->fillRect( rect, gradient );

    painter->setPen( enabled ? m_colors.arrowBorder
                             : blendColors( m_colors.arrowBorder, m_colors.fieldBackgroundDisabled, 0.5 ) );
    painter->setBrush( Qt::NoBrush );
    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
}

void WinXPStyle::drawXPArrowButton( QPainter* painter, const QStyleOption* option,
    Qt::ArrowType arrow, int glyphWidth, int glyphHeight ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 4 || rect.height() < 4 )
        return;

    drawXPButtonFace( painter, option );

    // The glyph is a fixed size centered in the button (see drawXPGlyphArrow);
    // the button rect only contributes its center. The disabled state draws
    // the classic 1px highlight shadow below/right of the dimmed glyph.
    const QPoint center = rect.center();
    if ( option->state & QStyle::State_Enabled ) {
        drawXPGlyphArrow( painter, center, glyphWidth, glyphHeight, arrow, m_colors.arrowGlyph );
    } else {
        drawXPGlyphArrow( painter, center + QPoint( 1, 1 ), glyphWidth, glyphHeight, arrow, QColor( 0xFF, 0xFF, 0xFF ) );
        drawXPGlyphArrow( painter, center, glyphWidth, glyphHeight, arrow, m_colors.arrowGlyphDisabled );
    }
}

void WinXPStyle::drawXPScrollThumb( QPainter* painter, const QRect& rect, const QStyleOption* option ) const
{
    if ( rect.width() < 4 || rect.height() < 4 )
        return;

    const bool horizontal = option->state & QStyle::State_Horizontal;
    const bool enabled = option->state & QStyle::State_Enabled;
    const bool sunken = option->state & QStyle::State_Sunken;
    const bool hot = option->state & QStyle::State_MouseOver;

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    // Fill the whole slider rect with the track color so the rounded
    // corners blend with the groove instead of exposing the parent
    // background as a halo that makes the border read too thick.
    painter->fillRect( rect, m_colors.track );

    QLinearGradient gradient;
    if ( horizontal )
        gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
    else
        gradient = QLinearGradient( rect.topLeft(), rect.topRight() );

    QColor begin = m_colors.thumbFaceBegin;
    QColor end = m_colors.thumbFaceEnd;
    if ( !enabled ) {
        begin = blendColors( m_colors.thumbFaceBegin, m_colors.fieldBackgroundDisabled, 0.5 );
        end = blendColors( m_colors.thumbFaceEnd, m_colors.fieldBackgroundDisabled, 0.5 );
    } else if ( sunken ) {
        begin = m_colors.thumbFaceEnd;
        end = m_colors.thumbFaceBegin;
    } else if ( hot ) {
        begin = blendColors( m_colors.thumbFaceBegin, m_colors.indicatorHotBorder, 0.2 );
        end = blendColors( m_colors.thumbFaceEnd, m_colors.indicatorHotBorder, 0.2 );
    }
    gradient.setColorAt( 0.0, begin );
    gradient.setColorAt( 1.0, end );

    painter->setPen( QPen( enabled ? m_colors.thumbBorder
                                   : blendColors( m_colors.thumbBorder, m_colors.fieldBackgroundDisabled, 0.6 ), 1.0 ) );
    painter->setBrush( gradient );
    // QPainter clamps the corner radius to half the short side, so a fixed
    // radius turns a stubby thumb into an ellipse. XP keeps short thumbs
    // rectangular-ish; scale the radius down with the short side.
    const int shortSide = qMin( rect.width(), rect.height() );
    const int radius = qMin( 3, shortSide / 4 );
    // Center the 1px pen on the pixel boundary (half a pixel inside the
    // integer rect) so the border renders as a full-opacity ring at the
    // thumb's outer edge rather than a soft 50% line straddling it.
    painter->drawRoundedRect( QRectF( rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ), radius, radius );

    // SBP_GRIPPERVERT/HORZ: the two short dashes that mark the thumb as
    // draggable. They lie along the thumb's long axis, stacked in the middle:
    // a vertical thumb gets two horizontal dashes above each other, a
    // horizontal thumb two vertical dashes side by side. Only draw them when
    // the thumb is long enough along that axis to read; a stubby thumb just
    // gets the border. fillRect keeps the dashes crisp (an antialiased 1px
    // line over the busy gradient barely reads at this size).
    if ( ( horizontal && rect.width() >= 12 ) || ( !horizontal && rect.height() >= 12 ) ) {
        // The gripper tracks the thumb face: hot brightens, pressed darkens,
        // disabled fades. The blend weights reproduce the ReactOS gripper
        // state colors (hot #6492AC, pressed #4D7791) from the normal #54839E.
        QColor grip = m_colors.thumbGripper;
        if ( !enabled )
            grip = blendColors( m_colors.thumbGripper, m_colors.fieldBackgroundDisabled, 0.5 );
        else if ( sunken )
            grip = blendColors( m_colors.thumbGripper, QColor( 0x00, 0x00, 0x00 ), 0.85 );
        else if ( hot )
            grip = blendColors( m_colors.thumbGripper, QColor( 0xFF, 0xFF, 0xFF ), 0.9 );
        painter->setPen( Qt::NoPen );
        painter->setBrush( grip );
        const int cx = rect.center().x();
        const int cy = rect.center().y();
        if ( horizontal ) {
            // Two vertical dashes, side by side across the middle.
            painter->drawRect( cx - 4, cy - 2, 1, 5 );
            painter->drawRect( cx + 3, cy - 2, 1, 5 );
        } else {
            // Two horizontal dashes, stacked across the middle.
            painter->drawRect( cx - 2, cy - 4, 5, 1 );
            painter->drawRect( cx - 2, cy + 3, 5, 1 );
        }
    }

    painter->restore();
}

void WinXPStyle::drawXPSliderGroove( QPainter* painter, const QStyleOption* option ) const
{
    QRect rect = option->rect;
    const bool horizontal = option->state & QStyle::State_Horizontal;

    if ( horizontal )
        rect = QRect( rect.left(), rect.center().y() - 2, rect.width(), 5 );
    else
        rect = QRect( rect.center().x() - 2, rect.top(), 5, rect.height() );

    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    painter->fillRect( rect, m_colors.track );
    painter->setPen( m_colors.trackBorder );
    painter->setBrush( Qt::NoBrush );
    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
}

void WinXPStyle::drawXPSliderThumb( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 3 || rect.height() < 3 )
        return;

    const bool horizontal = option->state & QStyle::State_Horizontal;
    const bool enabled = option->state & QStyle::State_Enabled;
    const bool sunken = option->state & QStyle::State_Sunken;

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    QLinearGradient gradient;
    if ( horizontal )
        gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
    else
        gradient = QLinearGradient( rect.topLeft(), rect.topRight() );

    QColor begin = m_colors.thumbFaceBegin;
    QColor end = m_colors.thumbFaceEnd;
    if ( sunken && enabled ) {
        begin = m_colors.thumbFaceEnd;
        end = m_colors.thumbFaceBegin;
    }
    if ( !enabled ) {
        begin = blendColors( m_colors.thumbFaceBegin, m_colors.fieldBackgroundDisabled, 0.5 );
        end = blendColors( m_colors.thumbFaceEnd, m_colors.fieldBackgroundDisabled, 0.5 );
    }
    gradient.setColorAt( 0.0, begin );
    gradient.setColorAt( 1.0, end );

    painter->setPen( QPen( enabled ? m_colors.thumbBorder
                                   : blendColors( m_colors.thumbBorder, m_colors.fieldBackgroundDisabled, 0.6 ), 1.0 ) );
    painter->setBrush( gradient );
    // Center the 1px pen on the pixel boundary so the border is a solid
    // ring at the thumb's outer edge (same as the scrollbar thumb).
    painter->drawRoundedRect( QRectF( rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 2, 2 );

    painter->restore();
}

void WinXPStyle::drawXPProgressGroove( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    painter->fillRect( rect, m_colors.progressGroove );
    painter->setPen( m_colors.progressBorder );
    painter->setBrush( Qt::NoBrush );
    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
}

void WinXPStyle::drawXPProgressChunk( QPainter* painter, const QRect& rect, const QStyleOption* option ) const
{
    Q_UNUSED( option );
    if ( rect.width() < 1 || rect.height() < 1 )
        return;

    // Luna chunk gradient: light top edge, solid middle, slightly darker
    // bottom. srcWeight 0.85 means 15% black - enough depth without going
    // muddy; the earlier 0.15 (85% black) was the mistake to avoid.
    QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
    gradient.setColorAt( 0.0, blendColors( m_colors.progressChunk, QColor( 0xFF, 0xFF, 0xFF ), 0.55 ) );
    gradient.setColorAt( 0.45, m_colors.progressChunk );
    gradient.setColorAt( 1.0, blendColors( m_colors.progressChunk, QColor( 0x00, 0x00, 0x00 ), 0.85 ) );
    painter->fillRect( rect, gradient );
}

void WinXPStyle::drawXPHeaderSection( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    const bool sunken = option->state & QStyle::State_Sunken;

    QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
    if ( sunken ) {
        gradient.setColorAt( 0.0, m_colors.headerEnd );
        gradient.setColorAt( 1.0, m_colors.headerBegin );
    } else {
        gradient.setColorAt( 0.0, m_colors.headerBegin );
        gradient.setColorAt( 1.0, m_colors.headerEnd );
    }
    painter->fillRect( rect, gradient );

    painter->setPen( m_colors.headerBorder );
    painter->setBrush( Qt::NoBrush );
    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );

    painter->setPen( m_colors.groupBoxBorderLight );
    painter->drawLine( rect.left() + 1, rect.bottom() - 1, rect.right() - 1, rect.bottom() - 1 );
}

void WinXPStyle::drawXPGroupBox( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    painter->save();
    painter->setPen( m_colors.groupBoxBorder );
    painter->setBrush( Qt::NoBrush );
    // Property-sheet Luna uses slightly rounded frames; Classic stays sharp.
    if ( m_mode != Classic ) {
        painter->setRenderHint( QPainter::Antialiasing, true );
        painter->drawRoundedRect( QRectF( rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 3.0, 3.0 );
    } else {
        painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
    }
    painter->restore();
}

void WinXPStyle::drawXPSplitterHandle( QPainter* painter, const QStyleOption* option ) const
{
    const QRect rect = option->rect;
    if ( rect.width() < 2 || rect.height() < 2 )
        return;

    // State_Horizontal is set for handles of a Qt::Horizontal splitter, i.e.
    // a vertical bar separating left/right panes; for the other orientation
    // the gradient and the groove run at 90 degrees.
    const bool verticalBar = option->state & QStyle::State_Horizontal;

    QLinearGradient gradient;
    if ( verticalBar )
        gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
    else
        gradient = QLinearGradient( rect.topLeft(), rect.topRight() );
    gradient.setColorAt( 0.0, m_colorBarBegin );
    gradient.setColorAt( 1.0, m_colorBarEnd );
    painter->fillRect( rect, gradient );

    painter->setPen( m_colorBorderLight );
    painter->setBrush( Qt::NoBrush );
    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );

    // XP splitter handles carry a short recessed groove in the middle.
    if ( verticalBar ) {
        painter->setPen( m_colorSeparatorLight );
        painter->drawLine( rect.left() + 1, rect.center().y(), rect.right() - 1, rect.center().y() );
        painter->setPen( m_colorSeparator );
        painter->drawLine( rect.left() + 1, rect.center().y() + 1, rect.right() - 1, rect.center().y() + 1 );
    } else {
        painter->setPen( m_colorSeparatorLight );
        painter->drawLine( rect.center().x(), rect.top() + 1, rect.center().x(), rect.bottom() - 1 );
        painter->setPen( m_colorSeparator );
        painter->drawLine( rect.center().x() + 1, rect.top() + 1, rect.center().x() + 1, rect.bottom() - 1 );
    }
}

void WinXPStyle::drawPrimitive( PrimitiveElement element, const QStyleOption* option,
    QPainter* painter, const QWidget* widget ) const
{
    // PE_WindowGradient is a private extension outside QStyle::PrimitiveElement,
    // so it cannot be used as a case label in the switch below (-Wswitch).
    if ( element == static_cast<PrimitiveElement>( PE_WindowGradient ) ) {
        // polish() already flattens begin/end to the same beige for
        // winxp-classic, so a two-stop gradient is a solid fill there.
        QLinearGradient gradient( option->rect.topLeft(), option->rect.topRight() );
        gradient.setColorAt( 0.0, m_colorBackgroundBegin );
        gradient.setColorAt( 0.6, m_colorBackgroundEnd );
        painter->fillRect( option->rect, gradient );
        return;
    }

    switch ( element ) {
        case PE_Widget:
            if ( qobject_cast<const QMainWindow*>( widget ) ) {
                QRect rect = option->rect;
                if ( QStatusBar* statusBar = widget->findChild<QStatusBar*>() ) {
                    rect.adjust( 0, 0, 0, -statusBar->height() );
                    painter->setPen( option->palette.light().color() );
                    painter->drawLine( rect.bottomLeft() + QPoint( 0, 1 ),
                        rect.bottomRight() + QPoint( 0, 1 ) );
                }
                QLinearGradient gradient( option->rect.topLeft(), option->rect.topRight() );
                gradient.setColorAt( 0.0, m_colorBackgroundBegin );
                gradient.setColorAt( 0.6, m_colorBackgroundEnd );
                painter->fillRect( rect, gradient );
                return;
            }

            if ( qobject_cast<const QToolBox*>( widget ) ) {
                QLinearGradient gradient( option->rect.topLeft(), option->rect.topRight() );
                gradient.setColorAt( 0.4, m_colorBackgroundBegin );
                gradient.setColorAt( 1.0, m_colorBackgroundEnd );
                painter->fillRect( option->rect, gradient );
                return;
            }

            if ( isToolBoxPanel( widget ) ) {
                QLinearGradient gradient( option->rect.topLeft(), option->rect.topRight() );
                gradient.setColorAt( 0.4, m_colorBarMiddle );
                gradient.setColorAt( 1.0, m_colorBarBegin );
                painter->fillRect( option->rect, gradient );
                return;
            }
            break;

        case PE_PanelMenuBar:
            return;

        case PE_FrameMenu:
            painter->setPen( m_colorMenuBorder );
            painter->setBrush( Qt::NoBrush );
            painter->drawRect( option->rect.adjusted( 0, 0, -1, -1 ) );

            if ( const QMenu* menu = qobject_cast<const QMenu*>( widget ) ) {
                if ( const QMenuBar* menuBar = qobject_cast<const QMenuBar*>( menu->parent() ) ) {
                    QRect rect = menuBar->actionGeometry( menu->menuAction() );
                    if ( !rect.isEmpty() ) {
                        painter->setPen( m_colorMenuBackground );
                        painter->drawLine( 1, 0, rect.width() - 2, 0 );
                    }
                }
            }

            if ( const QToolBar* toolBar = qobject_cast<const QToolBar*>( widget ) ) {
                QRect rect = option->rect.adjusted( 1, 1, -1, -1 );
                QLinearGradient gradient;
                if ( toolBar->orientation() == Qt::Vertical )
                    gradient = QLinearGradient( rect.topLeft(), rect.topRight() );
                else
                    gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
                gradient.setColorAt( 0.0, m_colorBarBegin );
                gradient.setColorAt( 0.4, m_colorBarMiddle );
                gradient.setColorAt( 0.6, m_colorBarMiddle );
                gradient.setColorAt( 1.0, m_colorBarEnd );
                painter->fillRect( rect, gradient );
            }
            return;

        case PE_IndicatorToolBarHandle:
            if ( option->state & State_Horizontal ) {
                for ( int i = option->rect.height() / 5; i <= 4 * ( option->rect.height() / 5 ); i += 5 ) {
                    int x = option->rect.left() + 3;
                    int y = option->rect.top() + i + 1;
                    painter->fillRect( x + 1, y, 2, 2, m_colorHandleLight );
                    painter->fillRect( x, y - 1, 2, 2, m_colorHandle );
                }
            } else {
                for ( int i = option->rect.width() / 5; i <= 4 * ( option->rect.width() / 5 ); i += 5 ) {
                    int x = option->rect.left() + i + 1;
                    int y = option->rect.top() + 3;
                    painter->fillRect( x, y + 1, 2, 2, m_colorHandleLight );
                    painter->fillRect( x - 1, y, 2, 2, m_colorHandle );
                }
            }
            return;

        case PE_IndicatorToolBarSeparator:
            painter->setPen( m_colorSeparator );
            if ( option->state & State_Horizontal )
                painter->drawLine( ( option->rect.left() + option->rect.right() - 1 ) / 2, option->rect.top() + 2,
                    ( option->rect.left() + option->rect.right() - 1 ) / 2, option->rect.bottom() - 2 );
            else
                painter->drawLine( option->rect.left() + 2, ( option->rect.top() + option->rect.bottom() - 1 ) / 2,
                    option->rect.right() - 2, ( option->rect.top() + option->rect.bottom() - 1 ) / 2 );
            painter->setPen( m_colorSeparatorLight );
            if ( option->state & State_Horizontal )
                painter->drawLine( ( option->rect.left() + option->rect.right() + 1 ) / 2, option->rect.top() + 2,
                    ( option->rect.left() + option->rect.right() + 1 ) / 2, option->rect.bottom() - 2 );
            else
                painter->drawLine( option->rect.left() + 2, ( option->rect.top() + option->rect.bottom() + 1 ) / 2,
                    option->rect.right() - 2, ( option->rect.top() + option->rect.bottom() + 1 ) / 2 );
            return;

        case PE_IndicatorButtonDropDown: {
            const QToolBar* toolBar = widget
                ? qobject_cast<const QToolBar*>( widget->parentWidget() ) : nullptr;
            if ( toolBar ) {
                QRect rect = option->rect.adjusted( -1, 0, -1, -1 );
                bool selected = option->state & State_MouseOver && option->state & State_Enabled;
                bool sunken = option->state & State_Sunken;
                if ( selected || sunken ) {
                    painter->setPen( m_colorItemBorder );
                    if ( toolBar->orientation() == Qt::Vertical ) {
                        if ( sunken )
                            painter->setBrush( m_colorItemSunkenEnd );
                        else
                            painter->setBrush( m_colorItemBackgroundEnd );
                    } else {
                        QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
                        if ( sunken ) {
                            gradient.setColorAt( 0.0, m_colorItemSunkenBegin );
                            gradient.setColorAt( 0.5, m_colorItemSunkenMiddle );
                            gradient.setColorAt( 1.0, m_colorItemSunkenEnd );
                        } else {
                            gradient.setColorAt( 0.0, m_colorItemBackgroundBegin );
                            gradient.setColorAt( 0.5, m_colorItemBackgroundMiddle );
                            gradient.setColorAt( 1.0, m_colorItemBackgroundEnd );
                        }
                        painter->setBrush( gradient );
                    }
                    painter->drawRect( rect );
                }
                QStyleOption optionArrow = *option;
                optionArrow.rect.adjust( 2, 2, -2, -2 );
                drawPrimitive( PE_IndicatorArrowDown, &optionArrow, painter, widget );
            }
            return;
        }

        case PE_IndicatorDockWidgetResizeHandle:
            return;

        case PE_IndicatorHeaderArrow:
            // Header sort chevron. SE_HeaderArrow already sizes the rect;
            // use the 7×7 spin glyph so it stays crisp like other Luna arrows.
            //
            // QCommonStyle draws SortUp tip-down and SortDown tip-up; match
            // that visual, not the enum name. (QHeaderView's Ascending→enum
            // mapping flipped across Qt versions; the enum→geometry map did not.)
            if ( const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>( option ) ) {
                if ( header->sortIndicator == QStyleOptionHeader::None )
                    return;
                const bool tipDown = header->sortIndicator == QStyleOptionHeader::SortUp;
                const QColor color = ( option->state & State_Enabled )
                    ? option->palette.color( QPalette::ButtonText )
                    : option->palette.color( QPalette::Disabled, QPalette::ButtonText );
                // Must stay exactly 7×7: drawXPGlyphArrow treats 9×9 as the
                // scroll-arrow bitmap. dpiScaled(7) rounding into 9 would
                // silently swap in the wrong chevron.
                drawXPGlyphArrow( painter, option->rect.center(), 7, 7,
                    tipDown ? Qt::DownArrow : Qt::UpArrow, color );
                return;
            }
            break;

        case PE_PanelButtonTool:
            if ( widget && widget->inherits( "QDockWidgetTitleButton" ) ) {
                if ( option->state & ( QStyle::State_MouseOver | QStyle::State_Sunken ) ) {
                    painter->setPen( m_colorItemBorder );
                    painter->setBrush( ( option->state & QStyle::State_Sunken ) ? m_colorItemSunkenMiddle : m_colorItemBackgroundMiddle );
                    painter->drawRect( option->rect.adjusted( 0, 0, -1, -1 ) );
                }
                return;
            }
            break;

        case PE_PanelButtonCommand:
            drawXPButton( painter, option );
            return;

        case PE_PanelLineEdit:
        case PE_FrameLineEdit:
            // A calendar-popup QDateTimeEdit draws one fused field border from
            // its CC_ComboBox branch; skip the embedded QLineEdit's nested frame
            // so the two fuse instead of drawing a double border.
            if ( widget && qobject_cast<const QDateTimeEdit*>( widget->parentWidget() ) )
                return;
            drawXPEditField( painter, option );
            return;

        case PE_IndicatorCheckBox:
            drawXPIndicator( painter, option, false, option->state & QStyle::State_On );
            return;

        case PE_IndicatorRadioButton:
            drawXPIndicator( painter, option, true, option->state & QStyle::State_On );
            return;

        case PE_IndicatorProgressChunk:
            drawXPProgressChunk( painter, option->rect, option );
            return;

        case PE_FrameDefaultButton:
            // The XP default-button ring is drawn by drawXPButton() itself, so
            // the base style's generic frame (palette shadow) is suppressed.
            return;

        case PE_FrameTabWidget:
            if ( isStyledTabWidget( widget ) ) {
                painter->fillRect( option->rect, option->palette.window() );
                if ( m_forceClassicPalette ) {
                    painter->setPen( QColor( 0x91, 0x9b, 0x9c ) );
                    painter->setBrush( Qt::NoBrush );
                    painter->drawRect( option->rect.adjusted( 0, 0, -1, -1 ) );
                }
                return;
            }
            break;

        case PE_FrameTabBarBase:
            if ( isStyledTabBar( widget ) )
                return;
            break;

        case PE_FrameGroupBox:
            drawXPGroupBox( painter, option );
            return;

        default:
            break;
    }

    QProxyStyle::drawPrimitive( element, option, painter, widget );
}

void WinXPStyle::drawControl( ControlElement element, const QStyleOption* option,
    QPainter* painter, const QWidget* widget ) const
{
    switch ( element ) {
        case CE_PushButton:
            // Push-button pipeline (bevel + label + focus) reproduced from
            // QCommonStyle. The pieces call our own drawControl()/drawPrimitive()
            // overrides directly (not proxy()), so the XP button look stays under
            // our control and does not depend on how the base style chains the
            // sub-elements.
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) ) {
                drawControl( CE_PushButtonBevel, btn, painter, widget );
                QStyleOptionButton subopt = *btn;
                subopt.rect = subElementRect( SE_PushButtonContents, btn, widget );
                drawControl( CE_PushButtonLabel, &subopt, painter, widget );
                if ( btn->state & QStyle::State_HasFocus ) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=( *btn );
                    fropt.rect = subElementRect( SE_PushButtonFocusRect, btn, widget );
                    drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
                }
            }
            return;

        case CE_PushButtonBevel:
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) ) {
                QRect br = btn->rect;
                if ( btn->features & QStyleOptionButton::DefaultButton )
                    drawPrimitive( PE_FrameDefaultButton, option, painter, widget );
                const bool hot = btn->state & QStyle::State_MouseOver && btn->state & QStyle::State_Enabled;
                // Flat push buttons still get the XP hot border on hover.
                if ( !( btn->features & ( QStyleOptionButton::Flat | QStyleOptionButton::CommandLinkButton ) )
                    || btn->state & ( QStyle::State_Sunken | QStyle::State_On )
                    || ( btn->features & QStyleOptionButton::Flat && hot )
                    || ( btn->features & QStyleOptionButton::CommandLinkButton && hot ) ) {
                    QStyleOptionButton tmpBtn = *btn;
                    tmpBtn.rect = br;
                    drawPrimitive( PE_PanelButtonCommand, &tmpBtn, painter, widget );
                }
                if ( btn->features & QStyleOptionButton::HasMenu ) {
                    int mbi = pixelMetric( PM_MenuButtonIndicator, btn, widget );
                    QRect ir = btn->rect;
                    QStyleOptionButton newBtn = *btn;
                    newBtn.rect = QRect( ir.right() - mbi - 2, ir.height() / 2 - mbi / 2 + 3, mbi - 6, mbi - 6 );
                    newBtn.rect = visualRect( btn->direction, br, newBtn.rect );
                    drawPrimitive( PE_IndicatorArrowDown, &newBtn, painter, widget );
                }
            }
            return;

        case CE_PushButtonLabel:
            if ( const QStyleOptionButton* button = qstyleoption_cast<const QStyleOptionButton*>( option ) ) {
                QRect textRect = button->rect;
                int tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
                if ( !styleHint( SH_UnderlineShortcut, button, widget ) )
                    tf |= Qt::TextHideMnemonic;

                if ( button->features & QStyleOptionButton::HasMenu ) {
                    int indicatorSize = pixelMetric( PM_MenuButtonIndicator, button, widget );
                    if ( button->direction == Qt::LeftToRight )
                        textRect = textRect.adjusted( 0, 0, -indicatorSize, 0 );
                    else
                        textRect = textRect.adjusted( indicatorSize, 0, 0, 0 );
                }

                if ( !button->icon.isNull() ) {
                    // Center both icon and text
                    QIcon::Mode mode = button->state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled;
                    if ( mode == QIcon::Normal && button->state & QStyle::State_HasFocus )
                        mode = QIcon::Active;
                    QIcon::State state = QIcon::Off;
                    if ( button->state & QStyle::State_On )
                        state = QIcon::On;

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
                    QPixmap pixmap = button->icon.pixmap( button->iconSize,
                        QStyleHelper::getDpr(painter), mode, state );
#else
                    QPixmap pixmap = button->icon.pixmap( button->iconSize, mode, state );
#endif
                    int pixmapWidth = pixmap.width() / pixmap.devicePixelRatio();
                    int pixmapHeight = pixmap.height() / pixmap.devicePixelRatio();
                    int labelWidth = pixmapWidth;
                    int labelHeight = pixmapHeight;
                    int iconSpacing = 4; //### 4 is currently hardcoded in QPushButton::sizeHint()
                    if ( !button->text.isEmpty() ) {
                        int textWidth = button->fontMetrics.boundingRect( option->rect, tf, button->text ).width();
                        labelWidth += ( textWidth + iconSpacing );
                    }

                    QRect iconRect = QRect( textRect.x() + ( textRect.width() - labelWidth ) / 2,
                        textRect.y() + ( textRect.height() - labelHeight ) / 2,
                        pixmapWidth, pixmapHeight );

                    iconRect = visualRect( button->direction, textRect, iconRect );

                    if ( button->direction == Qt::RightToLeft )
                        textRect.setRight( iconRect.left() - iconSpacing / 2 );
                    else
                        textRect.setLeft( iconRect.left() + iconRect.width() + iconSpacing / 2 );

                    // qt_format_text reverses again when painter->layoutDirection is also RightToLeft
                    if ( painter->layoutDirection() == button->direction )
                        tf |= Qt::AlignLeft;
                    else
                        tf |= Qt::AlignRight;

                    if ( button->state & ( QStyle::State_On | QStyle::State_Sunken ) )
                        iconRect.translate( pixelMetric( PM_ButtonShiftHorizontal, option, widget ),
                            pixelMetric( PM_ButtonShiftVertical, option, widget ) );
                    painter->drawPixmap( iconRect, pixmap );
                } else {
                    tf |= Qt::AlignHCenter;
                }
                if ( button->state & ( QStyle::State_On | QStyle::State_Sunken ) )
                    textRect.translate( pixelMetric( PM_ButtonShiftHorizontal, option, widget ),
                        pixelMetric( PM_ButtonShiftVertical, option, widget ) );

                drawItemText( painter, textRect, tf, button->palette, ( button->state & QStyle::State_Enabled ),
                    button->text, QPalette::ButtonText );
            }
            return;

        case CE_MenuBarEmptyArea:
            if ( m_forceClassicPalette )
                painter->fillRect( option->rect, m_colorBackgroundBegin );
            return;

        case CE_MenuBarItem:
            if ( m_forceClassicPalette ) {
                const bool hot = option->state & QStyle::State_Enabled
                    && ( option->state & ( QStyle::State_Selected | QStyle::State_Sunken ) );
                if ( hot ) {
                    painter->fillRect( option->rect.adjusted( 0, 0, -1, -1 ), QColor( 0x31, 0x6a, 0xc5 ) );
                }
                if ( const QStyleOptionMenuItem* optionItem = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) ) {
                    int flags = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                    if ( !styleHint( SH_UnderlineShortcut, option, widget ) )
                        flags |= Qt::TextHideMnemonic;
                    const QPalette::ColorRole role = hot ? QPalette::HighlightedText : QPalette::Text;
                    drawItemText( painter, option->rect, flags, option->palette, true, optionItem->text, role );
                }
                return;
            }
            if ( option->state & QStyle::State_Sunken && option->state & QStyle::State_Enabled ) {
                painter->setPen( m_colorMenuBorder );
                QLinearGradient gradient( option->rect.topLeft(), option->rect.bottomLeft() );
                gradient.setColorAt( 0.0, m_colorMenuTitleBegin );
                gradient.setColorAt( 1.0, m_colorMenuTitleEnd );
                painter->setBrush( gradient );
                painter->drawRect( option->rect.adjusted( 0, 0, -1, 0 ) );
            } else if ( option->state & QStyle::State_Selected && option->state & QStyle::State_Enabled ) {
                painter->setPen( m_colorItemBorder );
                QLinearGradient gradient( option->rect.topLeft(), option->rect.bottomLeft() );
                gradient.setColorAt( 0.0, m_colorItemBackgroundBegin );
                gradient.setColorAt( 1.0, m_colorItemBackgroundEnd );
                painter->setBrush( gradient );
                painter->drawRect( option->rect.adjusted( 0, 0, -1, -1 ) );
            }
            if ( const QStyleOptionMenuItem* optionItem = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) ) {
                int flags = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if ( !styleHint( SH_UnderlineShortcut, option, widget ) )
                    flags |= Qt::TextHideMnemonic;
                if ( !optionItem->icon.isNull() ) {
                    const int smallIconSize = pixelMetric( PM_SmallIconSize, option, widget );
                    QPixmap pixmap = optionItem->icon.pixmap( QSize( smallIconSize, smallIconSize ),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                                              QStyleHelper::getDpr(painter),
#endif
                                                              QIcon::Normal );
                    drawItemPixmap( painter, option->rect, flags, pixmap );
                } else {
                    drawItemText( painter, option->rect, flags, option->palette, true, optionItem->text, QPalette::Text );
                }
            }
            return;

        case CE_MenuEmptyArea:
            painter->fillRect( option->rect, m_colorMenuBackground );
            return;

        case CE_MenuItem: {
            const bool selected = option->state & QStyle::State_Selected && option->state & QStyle::State_Enabled;
            if ( m_forceClassicPalette ) {
                // Plain XP menu: white field, solid Luna-blue selection, no Office margin stripe.
                painter->fillRect( option->rect, m_colorMenuBackground );
                if ( selected )
                    painter->fillRect( option->rect.adjusted( 1, 0, -1, -1 ), QColor( 0x31, 0x6a, 0xc5 ) );
            } else if ( selected ) {
                painter->setPen( m_colorItemBorder );
                painter->setBrush( m_colorItemBackgroundBegin );
                painter->drawRect( option->rect.adjusted( 1, 0, -3, -1 ) );
            } else {
                QLinearGradient gradient( QPoint( 0, 0 ), QPoint( 25, 0 ) );
                gradient.setColorAt( 0.0, m_colorBarBegin );
                gradient.setColorAt( 1.0, m_colorBarEnd );
                QRect margin = option->rect;
                margin.setWidth( 25 );
                painter->fillRect( margin, gradient );
                QRect background = option->rect;
                background.setLeft( margin.right() + 1 );
                painter->fillRect( background, m_colorMenuBackground );
            }
            if ( const QStyleOptionMenuItem* optionItem = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) ) {
                if ( optionItem->menuItemType == QStyleOptionMenuItem::Separator ) {
                    painter->setPen( m_colorSeparator );
                    const int sepLeft = m_forceClassicPalette ? option->rect.left() + 8 : option->rect.left() + 32;
                    painter->drawLine( sepLeft, ( option->rect.top() + option->rect.bottom() ) / 2,
                        option->rect.right(), ( option->rect.top() + option->rect.bottom() ) / 2 );
                    return;
                }
                QRect checkRect = option->rect.adjusted( 2, 1, -2, -2 );
                checkRect.setWidth( 20 );
                if ( optionItem->checked && option->state & QStyle::State_Enabled ) {
                    painter->setPen( m_colorItemBorder );
                    if ( selected )
                        painter->setBrush( m_colorItemSunkenBegin );
                    else
                        painter->setBrush( m_colorItemCheckedBegin );
                    painter->drawRect( checkRect );
                }
                if ( !optionItem->icon.isNull() ) {
                    QIcon::Mode mode;
                    if ( optionItem->state & State_Enabled )
                        mode = ( optionItem->state & State_Selected ) ? QIcon::Active : QIcon::Normal;
                    else
                        mode = QIcon::Disabled;
                    QIcon::State state = optionItem->checked ? QIcon::On : QIcon::Off;
                    const int smallIconSize = pixelMetric( PM_SmallIconSize, option, widget );
                    QPixmap pixmap = optionItem->icon.pixmap( QSize( smallIconSize, smallIconSize ),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                                              QStyleHelper::getDpr(painter),
#endif
                                                              mode, state );
                    QRect rect(0, 0, pixmap.width() / pixmap.devicePixelRatio(),
                               pixmap.height() / pixmap.devicePixelRatio());
                    rect.moveCenter( checkRect.center() );
                    painter->drawPixmap( rect.topLeft(), pixmap );
                } else if ( optionItem->checked ) {
                    QStyleOption optionCheckMark;
                    optionCheckMark.initFrom( widget );
                    optionCheckMark.rect = checkRect;
                    if ( !( option->state & State_Enabled ) )
                        optionCheckMark.palette.setBrush( QPalette::Text, optionCheckMark.palette.brush( QPalette::Disabled, QPalette::Text ) );
                    drawPrimitive( PE_IndicatorMenuCheckMark, &optionCheckMark, painter, widget );
                }
                QRect textRect = option->rect.adjusted( m_forceClassicPalette ? 24 : 32, 1, -16, -1 );
                int flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if ( !styleHint( SH_UnderlineShortcut, option, widget ) )
                    flags |= Qt::TextHideMnemonic;
                QString text = optionItem->text;
                int pos = text.indexOf( '\t' );
                const QPalette::ColorRole textRole = ( m_forceClassicPalette && selected )
                    ? QPalette::HighlightedText : QPalette::Text;
                if ( pos >= 0 ) {
                    drawItemText( painter, textRect, flags | Qt::AlignRight, option->palette, option->state & State_Enabled,
                        text.mid( pos + 1 ), textRole );
                    text = text.left( pos );
                }
                drawItemText( painter, textRect, flags, option->palette, option->state & State_Enabled, text, textRole );
                if ( optionItem->menuItemType == QStyleOptionMenuItem::SubMenu ) {
                    QStyleOption optionArrow;
                    optionArrow.initFrom( widget );
                    optionArrow.rect = option->rect.adjusted( 0, 4, -4, -4 );
                    optionArrow.rect.setLeft( option->rect.right() - 12 );
                    optionArrow.state = option->state & State_Enabled;
                    if ( m_forceClassicPalette && selected )
                        optionArrow.palette.setColor( QPalette::WindowText, Qt::white );
                    drawPrimitive( PE_IndicatorArrowRight, &optionArrow, painter, widget );
                }
            }
            return;
        }

        case CE_ToolBar: {
            QRect rect = option->rect;
            bool vertical = false;
            if ( const QToolBar* toolBar = qobject_cast<const QToolBar*>( widget ) ) {
                vertical = ( toolBar->orientation() == Qt::Vertical );
                if ( vertical )
                    rect.setBottom( toolBar->childrenRect().bottom() + 2 );
                else
                    rect.setRight( toolBar->childrenRect().right() + 2 );
            }
            painter->save();
            QRegion region = rect.adjusted( 2, 0, -2, 0 );
            region += rect.adjusted( 0, 2, 0, -2 );
            region += rect.adjusted( 1, 1, -1, -1 );
            painter->setClipRegion( region );
            QLinearGradient gradient;
            if ( vertical )
                gradient = QLinearGradient( rect.topLeft(), rect.topRight() );
            else
                gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
            gradient.setColorAt( 0.0, m_colorBarBegin );
            gradient.setColorAt( 0.4, m_colorBarMiddle );
            gradient.setColorAt( 0.6, m_colorBarMiddle );
            gradient.setColorAt( 1.0, m_colorBarEnd );
            painter->fillRect( rect, gradient );

            painter->setPen( vertical ? m_colorBorderLight : m_colorBorder );
            painter->drawLine( rect.bottomLeft() + QPoint( 2, 0 ), rect.bottomRight() - QPoint( 2, 0 ) );
            painter->setPen( vertical ? m_colorBorder : m_colorBorderLight );
            painter->drawLine( rect.topRight() + QPoint( 0, 2 ), rect.bottomRight() - QPoint( 0, 2 ) );
            painter->setPen( m_colorBorderLight );
            painter->drawPoint( rect.bottomRight() - QPoint( 1, 1 ) );
            painter->restore();
            return;
        }

        case CE_DockWidgetTitle: {
            QLinearGradient gradient( option->rect.topLeft(), option->rect.bottomLeft() );
            gradient.setColorAt( 0.0, m_colorBarBegin );
            gradient.setColorAt( 1.0, m_colorBarEnd );
            painter->fillRect( option->rect, gradient );
            if ( const QStyleOptionDockWidget* optionDockWidget = qstyleoption_cast<const QStyleOptionDockWidget*>( option ) ) {
                QRect rect = option->rect.adjusted( 6, 0, -4, 0 );
                if ( optionDockWidget->closable )
                    rect.adjust( 0, 0, -16, 0 );
                if ( optionDockWidget->floatable )
                    rect.adjust( 0, 0, -16, 0 );
                QString text = painter->fontMetrics().elidedText( optionDockWidget->title, Qt::ElideRight, rect.width() );
                drawItemText( painter, rect, Qt::AlignLeft | Qt::AlignVCenter, option->palette,
                    option->state & State_Enabled, text, QPalette::WindowText );
            }
            return;
        }

        case CE_TabBarTabShape:
            if ( isStyledTabBar( widget ) ) {
                bool firstTab = false;
                bool lastTab = false;
                bool south = false;
                bool west = false;
                bool east = false;
                if ( const QStyleOptionTab* optionTab = qstyleoption_cast<const QStyleOptionTab*>( option ) ) {
                    if ( optionTab->position == QStyleOptionTab::Beginning )
                        firstTab = true;
                    else if ( optionTab->position == QStyleOptionTab::End )
                        lastTab = true;
                    else if ( optionTab->position == QStyleOptionTab::OnlyOneTab )
                        firstTab = lastTab = true;
                    if ( optionTab->shape == QTabBar::RoundedSouth )
                        south = true;
                    else if ( optionTab->shape == QTabBar::RoundedWest )
                        west = true;
                    else if ( optionTab->shape == QTabBar::RoundedEast )
                        east = true;
                }
                QRect rect = option->rect;
                painter->save();
                if ( option->state & State_Selected ) {
                    // The selected tab sticks out towards the page by one pixel.
                    if ( south )
                        rect.adjust( firstTab ? 0 : -2, -1, lastTab ? -1 : 1, -1 );
                    else if ( west )
                        rect.adjust( firstTab ? 0 : -2, 0, 1, lastTab ? -1 : 1 );
                    else if ( east )
                        rect.adjust( -1, firstTab ? 0 : -2, 0, lastTab ? -1 : 1 );
                    else
                        rect.adjust( firstTab ? 0 : -2, 0, lastTab ? -1 : 1, 1 );
                } else {
                    // Unselected tabs shrink inwards so they stay flush with
                    // the pane; clip keeps the shared edge tidy.
                    if ( south ) {
                        rect.adjust( 0, -1, lastTab ? -1 : 0, -2 );
                        painter->setClipRect( rect.adjusted( 0, 1, 1, 1 ) );
                    } else if ( west ) {
                        rect.adjust( 1, 0, 0, lastTab ? -1 : 0 );
                        painter->setClipRect( rect.adjusted( 0, 0, 0, 1 ) );
                    } else if ( east ) {
                        rect.adjust( 0, 0, -1, lastTab ? -1 : 0 );
                        painter->setClipRect( rect.adjusted( 1, 0, 0, 1 ) );
                    } else {
                        rect.adjust( 0, 1, lastTab ? -1 : 0, 0 );
                        painter->setClipRect( rect.adjusted( 0, 0, 1, 0 ) );
                    }
                }
                QLinearGradient gradient;
                if ( south )
                    gradient = QLinearGradient( rect.bottomLeft(), rect.topLeft() );
                else if ( west )
                    gradient = QLinearGradient( rect.topLeft(), rect.topRight() );
                else if ( east )
                    gradient = QLinearGradient( rect.topRight(), rect.topLeft() );
                else
                    gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
                // Property-sheet tabs (winxp-classic only): selected tab merges
                // into the pane with a 2px orange stripe; inactive tabs are flat
                // beige. Silver/Olive keep the Office candy-tab path below.
                if ( m_forceClassicPalette && ( option->state & State_Selected ) ) {
                    painter->setPen( QColor( 0x91, 0x9b, 0x9c ) );
                    painter->setBrush( option->palette.window().color() );
                    painter->drawRect( rect );
                    const QColor orange( 0xe6, 0x8b, 0x2c );
                    if ( south )
                        painter->fillRect( QRect( rect.left() + 1, rect.bottom() - 1, rect.width() - 2, 2 ), orange );
                    else if ( west )
                        painter->fillRect( QRect( rect.left(), rect.top() + 1, 2, rect.height() - 2 ), orange );
                    else if ( east )
                        painter->fillRect( QRect( rect.right() - 1, rect.top() + 1, 2, rect.height() - 2 ), orange );
                    else
                        painter->fillRect( QRect( rect.left() + 1, rect.top(), rect.width() - 2, 2 ), orange );
                } else if ( m_forceClassicPalette ) {
                    if ( option->state & State_MouseOver && option->state & State_Enabled ) {
                        gradient.setColorAt( 0.0, QColor( 0xff, 0xf8, 0xe0 ) );
                        gradient.setColorAt( 1.0, QColor( 0xf1, 0xf1, 0xec ) );
                        painter->setPen( QColor( 0xfa, 0xc4, 0x58 ) );
                        painter->setBrush( gradient );
                    } else {
                        painter->setPen( QColor( 0xb0, 0xb0, 0xa8 ) );
                        painter->setBrush( QColor( 0xf1, 0xf1, 0xec ) );
                    }
                    painter->drawRect( rect );
                } else if ( option->state & State_Selected ) {
                    gradient.setColorAt( 0.0, m_colorItemBackgroundBegin );
                    gradient.setColorAt( 1.0, option->palette.window().color() );
                    painter->setPen( m_colorBorder );
                    painter->setBrush( gradient );
                    painter->drawRect( rect );
                } else if ( option->state & State_MouseOver && option->state & State_Enabled ) {
                    gradient.setColorAt( 0.0, m_colorItemBackgroundBegin );
                    gradient.setColorAt( 1.0, m_colorItemBackgroundEnd );
                    painter->setPen( m_colorBorderLight );
                    painter->setBrush( gradient );
                    painter->drawRect( rect );
                } else {
                    gradient.setColorAt( 0.0, m_colorBarMiddle );
                    gradient.setColorAt( 1.0, m_colorBarEnd );
                    painter->setPen( m_colorBorderLight );
                    painter->setBrush( gradient );
                    painter->drawRect( rect );
                }
                painter->restore();
                return;
            }
            break;

        case CE_ToolBoxTabShape: {
            QRect rect = option->rect.adjusted( 0, 0, -1, -1 );
            QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
            if ( option->state & QStyle::State_Sunken ) {
                gradient.setColorAt( 0.0, m_colorItemSunkenBegin );
                gradient.setColorAt( 1.0, m_colorItemSunkenEnd );
                painter->setPen( m_colorBorder );
            } else if ( option->state & State_MouseOver && option->state & State_Enabled ) {
                gradient.setColorAt( 0.0, m_colorItemBackgroundBegin );
                gradient.setColorAt( 1.0, m_colorItemBackgroundEnd );
                painter->setPen( m_colorBorder );
            } else {
                gradient.setColorAt( 0.0, m_colorBarMiddle );
                gradient.setColorAt( 1.0, m_colorBarEnd );
                painter->setPen( m_colorBorderLight );
            }
            painter->setBrush( gradient );
            painter->drawRect( rect );
            return;
        }

        case CE_CheckBox:
        case CE_RadioButton: {
            // The base style would draw the indicator/label internally on
            // itself, bypassing the proxy, so the whole pipeline is reproduced
            // here (QCommonStyle layout) and routed back to our own overrides.
            const bool isCheckBox = element == CE_CheckBox;
            if ( const QStyleOptionButton* button = qstyleoption_cast<const QStyleOptionButton*>( option ) ) {
                QStyleOptionButton subopt = *button;
                subopt.rect = subElementRect( isCheckBox ? SE_CheckBoxIndicator : SE_RadioButtonIndicator, button, widget );
                drawPrimitive( isCheckBox ? PE_IndicatorCheckBox : PE_IndicatorRadioButton, &subopt, painter, widget );
                subopt.rect = subElementRect( isCheckBox ? SE_CheckBoxContents : SE_RadioButtonContents, button, widget );
                drawControl( isCheckBox ? CE_CheckBoxLabel : CE_RadioButtonLabel, &subopt, painter, widget );
            }
            return;
        }

        case CE_ComboBoxLabel:
            // QComboBox::initStyleOption marks a focused, non-editable combo
            // State_Selected, so the base QWindowsStyle paints its current
            // text in QPalette::HighlightedText (white) although XP draws no
            // highlight background here - the result was invisible
            // white-on-white text. Luna keeps the field text in the normal
            // text color on focus, so draw the label with the Text role
            // explicitly. The icon is still drawn for editable combos (the
            // embedded QLineEdit has no icon of its own), mirroring the
            // QCommonStyle layout.
            if ( const QStyleOptionComboBox* combo = qstyleoption_cast<const QStyleOptionComboBox*>( option ) ) {
                QRect textRect = subControlRect( CC_ComboBox, combo, SC_ComboBoxEditField, widget );
                if ( !textRect.isValid() )
                    textRect = combo->rect.adjusted( 1, 1, -18, -1 );
                const bool enabled = combo->state & QStyle::State_Enabled;
                if ( !combo->currentIcon.isNull() ) {
                    QIcon::Mode mode = enabled ? QIcon::Normal : QIcon::Disabled;
                    QPixmap pixmap = combo->currentIcon.pixmap( combo->iconSize, mode );
                    QRect iconRect( textRect );
                    iconRect.setWidth( combo->iconSize.width() + 4 );
                    iconRect = alignedRect( combo->direction, Qt::AlignLeft | Qt::AlignVCenter,
                        iconRect.size(), textRect );
                    drawItemPixmap( painter, iconRect, Qt::AlignCenter, pixmap );
                    if ( combo->direction == Qt::RightToLeft )
                        textRect.translate( -4 - combo->iconSize.width(), 0 );
                    else
                        textRect.translate( combo->iconSize.width() + 4, 0 );
                }
                if ( !combo->editable && !combo->currentText.isEmpty() ) {
                    uint alignment = visualAlignment( combo->direction,
                        Qt::AlignLeft | Qt::AlignVCenter );
                    if ( !styleHint( SH_UnderlineShortcut, option, widget ) )
                        alignment |= Qt::TextHideMnemonic;
                    drawItemText( painter, textRect.adjusted( 1, 0, -1, 0 ), alignment,
                        combo->palette, enabled, combo->currentText, QPalette::Text );
                }
            }
            return;

        case CE_CheckBoxLabel:
        case CE_RadioButtonLabel:
            if ( const QStyleOptionButton* button = qstyleoption_cast<const QStyleOptionButton*>( option ) ) {
                uint alignment = visualAlignment( button->direction, Qt::AlignLeft | Qt::AlignVCenter );
                if ( !styleHint( SH_UnderlineShortcut, button, widget ) )
                    alignment |= Qt::TextHideMnemonic;
                QRect textRect = button->rect;
                if ( !button->icon.isNull() ) {
                    QIcon::Mode mode = ( button->state & QStyle::State_Enabled ) ? QIcon::Normal : QIcon::Disabled;
                    QPixmap pixmap = button->icon.pixmap( button->iconSize,
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                                          QStyleHelper::getDpr(painter),
#endif
                                                          mode );
                    drawItemPixmap( painter, button->rect, alignment, pixmap );
                    if ( button->direction == Qt::LeftToRight )
                        textRect.setLeft( textRect.left() + button->iconSize.width() + 4 );
                    else
                        textRect.setRight( textRect.right() - button->iconSize.width() - 4 );
                }
                drawItemText( painter, textRect, alignment | Qt::TextShowMnemonic, button->palette,
                    button->state & QStyle::State_Enabled, button->text, QPalette::WindowText );
            }
            return;

        case CE_ProgressBar:
            if ( const QStyleOptionProgressBar* bar = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) ) {
                QStyleOptionProgressBar subopt = *bar;
                subopt.rect = subElementRect( SE_ProgressBarGroove, bar, widget );
                drawControl( CE_ProgressBarGroove, &subopt, painter, widget );
                subopt.rect = subElementRect( SE_ProgressBarContents, bar, widget );
                drawControl( CE_ProgressBarContents, &subopt, painter, widget );
                subopt.rect = subElementRect( SE_ProgressBarLabel, bar, widget );
                drawControl( CE_ProgressBarLabel, &subopt, painter, widget );
            }
            return;

        case CE_ProgressBarGroove:
            drawXPProgressGroove( painter, option );
            return;

        case CE_ProgressBarContents: {
            if ( const QStyleOptionProgressBar* bar = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) ) {
                const QRect rect = bar->rect;
                // Qt6's QStyleOptionProgressBar has no orientation member and
                // QProgressBar does not set bottomToTop automatically, so the
                // direction is taken from the aspect ratio (same as phase).
                const bool vertical = rect.height() > rect.width();
                const bool reverse = ( ( bar->direction == Qt::RightToLeft ) != bar->invertedAppearance );
                const int total = qMax( bar->maximum - bar->minimum, 1 );
                const int progress = qMax( bar->progress, bar->minimum );

                painter->save();
                painter->setClipRect( rect );

                if ( bar->minimum == 0 && bar->maximum == 0 ) {
                    // Busy indicator: a single chunk travelling back and
                    // forth along the groove (Luna style). The value is
                    // advanced by the animation timer; the ping-pong keeps it
                    // inside the groove at both ends.
                    const int length = vertical ? rect.height() : rect.width();
                    const int chunk = qBound( 10, length / 6, 30 );
                    const int span = qMax( length - chunk, 1 );
                    int pos = progress % ( span * 2 );
                    if ( pos > span )
                        pos = span * 2 - pos;
                    QRect chunkRect;
                    if ( vertical )
                        chunkRect = QRect( rect.left(), rect.top() + pos, rect.width(), chunk );
                    else
                        chunkRect = QRect( rect.left() + pos, rect.top(), chunk, rect.height() );
                    drawXPProgressChunk( painter, chunkRect, option );
                } else if ( progress > 0 ) {
                    QRect chunk;
                    if ( vertical ) {
                        const int h = rect.height() * progress / total;
                        chunk = reverse ? QRect( rect.left(), rect.top(), rect.width(), h )
                                        : QRect( rect.left(), rect.bottom() - h, rect.width(), h );
                    } else {
                        const int w = rect.width() * progress / total;
                        chunk = reverse ? QRect( rect.right() - w, rect.top(), w, rect.height() )
                                        : QRect( rect.left(), rect.top(), w, rect.height() );
                    }
                    drawXPProgressChunk( painter, chunk, option );
                }
                painter->restore();
            }
            return;
        }

        case CE_ProgressBarLabel: {
            // Two-pass label: highlighted on the chunk side, normal on the rest
            // (same layout as QCommonStyle). For a vertical bar the text is
            // rotated 90 degrees so it reads along the bar, honoring
            // QProgressBar::textDirection (bottomToTop => counter-clockwise,
            // otherwise clockwise, per Qt's convention).
            if ( const QStyleOptionProgressBar* bar = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) ) {
                const int total = qMax( bar->maximum - bar->minimum, 1 );
                const int progress = qMax( bar->progress, bar->minimum );
                const bool vertical = bar->rect.height() > bar->rect.width();
                QRect leftRect;
                QRect rightRect;
                if ( !vertical ) {
                    const int w = bar->rect.width() * progress / total;
                    if ( ( bar->invertedAppearance && bar->direction == Qt::RightToLeft )
                        || ( !bar->invertedAppearance && bar->direction != Qt::RightToLeft ) )
                        leftRect = QRect( bar->rect.left(), bar->rect.top(), w, bar->rect.height() );
                    else
                        leftRect = QRect( bar->rect.right() - w, bar->rect.top(), w, bar->rect.height() );
                    rightRect = QRect( leftRect.right() + 1, bar->rect.top(),
                        bar->rect.width() - leftRect.width(), bar->rect.height() );
                } else {
                    const int h = bar->rect.height() * progress / total;
                    leftRect = bar->invertedAppearance
                        ? QRect( bar->rect.left(), bar->rect.top(), bar->rect.width(), bar->rect.height() - h )
                        : QRect( bar->rect.left(), bar->rect.bottom() - h, bar->rect.width(), h );
                    rightRect = QRect( leftRect.left(), bar->rect.top(),
                        bar->rect.width(), bar->rect.height() - leftRect.height() );
                }

                painter->save();
                if ( vertical ) {
                    // Draw in a coordinate system rotated so the bar's long
                    // axis is horizontal, then draw the label as in the
                    // horizontal case. The clip rects follow the transform.
                    const QRect r = bar->rect;
                    const QPoint c = r.center();
                    painter->translate( c );
                    if ( bar->bottomToTop )
                        painter->rotate( -90 );   // counter-clockwise
                    else
                        painter->rotate( 90 );    // clockwise
                    painter->translate( -c );
                }
                if ( leftRect.isValid() && !leftRect.isEmpty() ) {
                    painter->save();
                    painter->setClipRect( leftRect );
                    drawItemText( painter, bar->rect, Qt::AlignCenter | Qt::TextSingleLine, bar->palette,
                        bar->state & QStyle::State_Enabled, bar->text, QPalette::HighlightedText );
                    painter->restore();
                }
                if ( rightRect.isValid() && !rightRect.isEmpty() ) {
                    painter->save();
                    painter->setClipRect( rightRect );
                    drawItemText( painter, bar->rect, Qt::AlignCenter | Qt::TextSingleLine, bar->palette,
                        bar->state & QStyle::State_Enabled, bar->text, QPalette::Text );
                    painter->restore();
                }
                painter->restore();
            }
            return;
        }

        case CE_ScrollBarAddLine:
        case CE_ScrollBarSubLine: {
            const bool horizontal = option->state & QStyle::State_Horizontal;
            Qt::ArrowType arrow;
            if ( element == CE_ScrollBarAddLine )
                arrow = horizontal ? Qt::RightArrow : Qt::DownArrow;
            else
                arrow = horizontal ? Qt::LeftArrow : Qt::UpArrow;
            drawXPArrowButton( painter, option, arrow, 9, 9 ); // ScrollArrowGlyphs 9x9
            return;
        }

        case CE_ScrollBarSubPage:
        case CE_ScrollBarAddPage:
            painter->fillRect( option->rect, m_colors.track );
            return;

        case CE_ScrollBarSlider:
            drawXPScrollThumb( painter, option->rect, option );
            return;

        case CE_ScrollBarFirst:
        case CE_ScrollBarLast:
            return;

        case CE_Header:
            // Full header pipeline (same proxy-bypass pattern as CE_PushButton /
            // CE_TabBarTab): section face, label, then sort arrow in SE_HeaderArrow.
            if ( const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>( option ) ) {
                const QRegion clipRegion = painter->clipRegion();
                painter->setClipRect( header->rect );
                drawControl( CE_HeaderSection, header, painter, widget );
                QStyleOptionHeader subopt = *header;
                subopt.rect = subElementRect( SE_HeaderLabel, header, widget );
                if ( subopt.rect.isValid() )
                    drawControl( CE_HeaderLabel, &subopt, painter, widget );
                if ( header->sortIndicator != QStyleOptionHeader::None ) {
                    subopt.rect = subElementRect( SE_HeaderArrow, option, widget );
                    if ( subopt.rect.isValid() )
                        drawPrimitive( PE_IndicatorHeaderArrow, &subopt, painter, widget );
                }
                painter->setClipRegion( clipRegion );
                return;
            }
            break;

        case CE_HeaderSection:
            // Only the section face. Sort indicator is drawn by CE_Header via
            // SE_HeaderArrow → PE_IndicatorHeaderArrow.
            drawXPHeaderSection( painter, option );
            return;

        case CE_TabBarTab:
            // QCommonStyle would draw shape+label on itself, bypassing the
            // proxy; route both pieces back through our own overrides.
            if ( const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>( option ) ) {
                drawControl( CE_TabBarTabShape, tab, painter, widget );
                drawControl( CE_TabBarTabLabel, tab, painter, widget );
            }
            return;

        case CE_TabBarTabLabel:
            // QCommonStyle rotates the label 90 degrees for vertical tab bars,
            // which is exactly what a vertical tab needs (tall narrow strip,
            // text running top to bottom). Let it fall through to the proxy.
            break;

        case CE_ToolBoxTab:
            // Same proxy-bypass fix as CE_TabBarTab.
            if ( const QStyleOptionToolBox* toolBoxTab = qstyleoption_cast<const QStyleOptionToolBox*>( option ) ) {
                drawControl( CE_ToolBoxTabShape, toolBoxTab, painter, widget );
                drawControl( CE_ToolBoxTabLabel, toolBoxTab, painter, widget );
            }
            return;

        case CE_Splitter:
            // QColumnView and QSplitter inside any window get the XP handle.
            drawXPSplitterHandle( painter, option );
            return;

        case CE_ColumnViewGrip: {
            // QColumnView's resize grip (its internal splitter-like handle).
            // Draw it like a narrow XP splitter handle: gradient fill, thin
            // border, and a short recessed vertical groove (two lines) that
            // hints at dragging column widths horizontally.
            const QRect rect = option->rect;
            if ( rect.width() < 4 || rect.height() < 4 )
                return;
            QLinearGradient gradient( rect.topLeft(), rect.bottomLeft() );
            gradient.setColorAt( 0.0, m_colorBarBegin );
            gradient.setColorAt( 1.0, m_colorBarEnd );
            painter->fillRect( rect, gradient );
            painter->setPen( m_colorBorderLight );
            painter->setBrush( Qt::NoBrush );
            painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
            const int x = rect.center().x();
            const int top = rect.top() + rect.height() * 30 / 100;
            const int bottom = rect.bottom() - rect.height() * 30 / 100;
            painter->setPen( m_colorSeparatorLight );
            painter->drawLine( x, top, x, bottom );
            painter->setPen( m_colorSeparator );
            painter->drawLine( x + 1, top, x + 1, bottom );
            return;
        }

        default:
            break;
    }

    QProxyStyle::drawControl( element, option, painter, widget );
}

void WinXPStyle::drawComplexControl( ComplexControl control, const QStyleOptionComplex* option,
    QPainter* painter, const QWidget* widget ) const
{
    switch ( control ) {
        case CC_ScrollBar:
            if ( const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>( option ) ) {
                // Same pipeline as QCommonStyle, but the sub-elements route
                // back through our own drawControl() overrides (calling them
                // directly on the base style would draw the Win2000 look).
                QStyleOptionSlider subopt = *scrollbar;
                const QStyle::SubControls hotSubControl = scrollbar->activeSubControls & SC_All;

                if ( option->subControls & SC_ScrollBarSubPage ) {
                    // subControlRect must see the whole scroll bar rect, so pass
                    // the original option; subopt.rect is mutated per element.
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarSubPage, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    drawControl( CE_ScrollBarSubPage, &subopt, painter, widget );
                }
                if ( option->subControls & SC_ScrollBarAddPage ) {
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarAddPage, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    drawControl( CE_ScrollBarAddPage, &subopt, painter, widget );
                }
                if ( option->subControls & SC_ScrollBarSubLine ) {
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarSubLine, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    if ( hotSubControl == SC_ScrollBarSubLine )
                        subopt.state |= State_MouseOver;
                    if ( scrollbar->state & State_Sunken ) {
                        if ( hotSubControl == SC_ScrollBarSubLine )
                            subopt.state |= State_Sunken;
                        else if ( subopt.state & State_Enabled )
                            subopt.state |= State_MouseOver;
                    }
                    drawControl( CE_ScrollBarSubLine, &subopt, painter, widget );
                }
                if ( option->subControls & SC_ScrollBarAddLine ) {
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarAddLine, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    if ( hotSubControl == SC_ScrollBarAddLine )
                        subopt.state |= State_MouseOver;
                    if ( scrollbar->state & State_Sunken ) {
                        if ( hotSubControl == SC_ScrollBarAddLine )
                            subopt.state |= State_Sunken;
                        else if ( subopt.state & State_Enabled )
                            subopt.state |= State_MouseOver;
                    }
                    drawControl( CE_ScrollBarAddLine, &subopt, painter, widget );
                }
                if ( option->subControls & SC_ScrollBarFirst ) {
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarFirst, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    drawControl( CE_ScrollBarFirst, &subopt, painter, widget );
                }
                if ( option->subControls & SC_ScrollBarLast ) {
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarLast, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    drawControl( CE_ScrollBarLast, &subopt, painter, widget );
                }
                if ( option->subControls & SC_ScrollBarSlider ) {
                    subopt.rect = subControlRect( control, scrollbar, SC_ScrollBarSlider, widget );
                    subopt.state = scrollbar->state & ~State_Sunken;
                    if ( scrollbar->state & State_Sunken ) {
                        if ( hotSubControl == SC_ScrollBarSlider )
                            subopt.state |= State_Sunken;
                        else
                            subopt.state |= State_MouseOver;
                    }
                    drawControl( CE_ScrollBarSlider, &subopt, painter, widget );
                }
            }
            return;

        case CC_Slider:
            if ( const QStyleOptionSlider* slider = qstyleoption_cast<const QStyleOptionSlider*>( option ) ) {
                const QRect groove = subControlRect( control, slider, SC_SliderGroove, widget );
                const QRect handle = subControlRect( control, slider, SC_SliderHandle, widget );

                if ( ( option->subControls & SC_SliderGroove ) && groove.isValid() ) {
                    QStyleOptionSlider subopt = *slider;
                    subopt.rect = groove;
                    drawXPSliderGroove( painter, &subopt );
                }

                if ( ( option->subControls & SC_SliderHandle ) && handle.isValid() ) {
                    QStyleOptionSlider subopt = *slider;
                    subopt.rect = handle;
                    drawXPSliderThumb( painter, &subopt );
                }

                if ( option->subControls & SC_SliderTickmarks ) {
                    // Qt 6 has no CE_SliderTickmarks control element, so the
                    // tick lines are drawn here directly (same layout as
                    // QCommonStyle::drawControl(CE_SliderTickmarks) in Qt 5).
                    const QStyleOptionSlider* sliderOpt = slider;
                    if ( ( sliderOpt->subControls & SC_SliderHandle ) && ( sliderOpt->subControls & SC_SliderGroove ) ) {
                        const bool horizontal = sliderOpt->orientation == Qt::Horizontal;
                        bool up = false, down = false, left = false, right = false;
                        if ( horizontal ) {
                            if ( sliderOpt->tickPosition & QSlider::TicksAbove )
                                up = true;
                            if ( sliderOpt->tickPosition & QSlider::TicksBelow )
                                down = true;
                        } else {
                            if ( sliderOpt->tickPosition & QSlider::TicksLeft )
                                left = true;
                            if ( sliderOpt->tickPosition & QSlider::TicksRight )
                                right = true;
                        }

                        QRect grooveRect = subControlRect( control, sliderOpt, SC_SliderGroove, widget );
                        const int thickness = horizontal ? grooveRect.height() : grooveRect.width();
                        if ( grooveRect.isValid() && grooveRect.width() > 0 && grooveRect.height() > 0 && thickness >= 5 ) {
                            const int span = horizontal ? grooveRect.width() : grooveRect.height();

                            int min = sliderOpt->minimum;
                            int max = sliderOpt->maximum;
                            if ( sliderOpt->upsideDown )
                                qSwap( min, max );
                            int interval = sliderOpt->tickInterval;
                            if ( interval <= 0 ) {
                                interval = max - min;
                                if ( interval > 10 )
                                    interval = 10;
                                else if ( interval < 2 )
                                    interval = 2;
                            }

                            const int tickSize = 3;
                            const int noTicks = ( max - min ) / interval;
                            if ( noTicks <= 10000 ) {
                                QVector<QLine> lines;
                                lines.reserve( noTicks + 1 );
                                int currentTick = min;
                                for ( int i = 0; i <= noTicks; ++i ) {
                                    if ( currentTick > max )
                                        break;
                                    const int pos = QStyle::sliderPositionFromValue( min, max, currentTick, span, sliderOpt->upsideDown )
                                        + ( horizontal ? grooveRect.left() : grooveRect.top() );
                                    int x = 0, y = 0, len = 0;
                                    if ( horizontal ) {
                                        x = pos;
                                        if ( up ) {
                                            y = grooveRect.top() - tickSize;
                                            len = tickSize;
                                        } else if ( down ) {
                                            y = grooveRect.bottom() + 1;
                                            len = tickSize;
                                        }
                                    } else {
                                        y = pos;
                                        if ( left ) {
                                            x = grooveRect.left() - tickSize;
                                            len = tickSize;
                                        } else if ( right ) {
                                            x = grooveRect.right() + 1;
                                            len = tickSize;
                                        }
                                    }
                                    lines.append( QLine( x, y, x, y + len ) );
                                    currentTick += interval;
                                }
                                painter->save();
                                painter->setPen( option->palette.text().color() );
                                painter->drawLines( lines.constData(), lines.size() );
                                painter->restore();
                            }
                        }
                    }
                }
            }
            return;

        case CC_ComboBox: {
            if ( const QStyleOptionComboBox* combo = qstyleoption_cast<const QStyleOptionComboBox*>( option ) ) {
                const bool enabled = option->state & QStyle::State_Enabled;

                // A calendar-popup QDateTimeEdit is rendered through CC_ComboBox
                // with editable=true and the ComboBox sub-controls. XP draws it
                // exactly like a non-editable combo: one shared field border with
                // the drop-down arrow button fused into the right edge (the
                // embedded QLineEdit's own PE_PanelLineEdit border is suppressed
                // in drawPrimitive).
                const bool isDateTimePopup = qobject_cast<const QDateTimeEdit*>( widget )
                    && ( option->subControls & SC_ComboBoxFrame );

                if ( isDateTimePopup || ( !combo->editable && ( option->subControls & SC_ComboBoxFrame ) ) ) {
                    // XP non-editable combo / calendar-popup field: white body with
                    // a single outer border, arrow button embedded on the right,
                    // flush against the field's right edge.
                    const QRect rect = option->rect;

                    painter->fillRect( rect, enabled ? m_colors.fieldBackground : m_colors.fieldBackgroundDisabled );
                    painter->setPen( enabled ? m_colors.fieldBorder : m_colors.fieldBackgroundDisabled );
                    painter->setBrush( Qt::NoBrush );
                    painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );

                    // The XP arrow button spans the whole inner height of the
                    // field, flush against the field's right side.
                    const QRect arrowRect( rect.right() - 17, rect.top() + 1, 17, rect.height() - 2 );
                    QStyleOption optArrow = *option;
                    optArrow.rect = arrowRect;
                    drawXPArrowButton( painter, &optArrow, Qt::DownArrow, 9, 7 ); // ComboButtonGlyph 9x7
                } else if ( combo->editable ) {
                    // The embedded QLineEdit draws PE_PanelLineEdit (through
                    // the proxy); only the arrow button belongs to the combo.
                    if ( option->subControls & SC_ComboBoxArrow ) {
                        const QRect rect = option->rect;
                        const QRect arrowRect( rect.right() - 17, rect.top() + 1, 17, rect.height() - 2 );
                        QStyleOption optArrow = *option;
                        optArrow.rect = arrowRect;
                        drawXPArrowButton( painter, &optArrow, Qt::DownArrow, 9, 7 ); // ComboButtonGlyph 9x7
                    }
                }
            }
            return;
        }

        case CC_SpinBox: {
            if ( const QStyleOptionSpinBox* spinBox = qstyleoption_cast<const QStyleOptionSpinBox*>( option ) ) {
                if ( spinBox->frame && ( option->subControls & SC_SpinBoxFrame ) ) {
                    QStyleOption optFrame = *option;
                    optFrame.rect = subControlRect( control, option, SC_SpinBoxFrame, widget );
                    drawXPEditField( painter, &optFrame );
                }

                // Validity is decided by the computed heights only (the button
                // width is a constant), avoiding QRect::isValid() on a rect
                // whose right edge is derived from the field's right edge.
                const QRect rect = option->rect;
                const int innerHeight = rect.height() - 2;
                const int upHeight = ( innerHeight + 1 ) / 2;
                const int downHeight = innerHeight - upHeight;
                const QRect upRect( rect.right() - 17, rect.top() + 1, 17, upHeight );
                const QRect downRect( rect.right() - 17, upRect.bottom() + 1, 17, downHeight );

                const bool drawUp = option->subControls & SC_SpinBoxUp;
                const bool drawDown = option->subControls & SC_SpinBoxDown;

                if ( drawUp ) {
                    if ( upHeight > 0 ) {
                        QStyleOption optBtn = *option;
                        optBtn.rect = upRect;
                        optBtn.state = option->state & ~State_Sunken;
                        if ( option->activeSubControls == SC_SpinBoxUp && ( option->state & State_Sunken ) )
                            optBtn.state |= State_Sunken;
                        if ( option->activeSubControls == SC_SpinBoxUp && ( option->state & State_MouseOver ) )
                            optBtn.state |= State_MouseOver;
                        if ( spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus ) {
                            drawXPButtonFace( painter, &optBtn );
                            QRect glyphRect = upRect.adjusted( upRect.width() / 3, upRect.height() / 3,
                                -upRect.width() / 3, -upRect.height() / 3 );
                            painter->setPen( QPen( ( optBtn.state & State_Enabled ) ? m_colors.arrowGlyph : m_colors.arrowGlyphDisabled, 1.6 ) );
                            painter->drawLine( glyphRect.left(), glyphRect.center().y(), glyphRect.right(), glyphRect.center().y() );
                            painter->drawLine( glyphRect.center().x(), glyphRect.top(), glyphRect.center().x(), glyphRect.bottom() );
                        } else {
                            drawXPArrowButton( painter, &optBtn, Qt::UpArrow, 7, 7 ); // SpinUpGlyph 7x7
                        }
                    }
                }
                if ( drawDown ) {
                    if ( downHeight > 0 ) {
                        QStyleOption optBtn = *option;
                        optBtn.rect = downRect;
                        optBtn.state = option->state & ~State_Sunken;
                        if ( option->activeSubControls == SC_SpinBoxDown && ( option->state & State_Sunken ) )
                            optBtn.state |= State_Sunken;
                        if ( option->activeSubControls == SC_SpinBoxDown && ( option->state & State_MouseOver ) )
                            optBtn.state |= State_MouseOver;
                        if ( spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus ) {
                            drawXPButtonFace( painter, &optBtn );
                            QRect glyphRect = downRect.adjusted( downRect.width() / 3, downRect.height() / 3,
                                -downRect.width() / 3, -downRect.height() / 3 );
                            painter->setPen( QPen( ( optBtn.state & State_Enabled ) ? m_colors.arrowGlyph : m_colors.arrowGlyphDisabled, 1.6 ) );
                            painter->drawLine( glyphRect.left(), glyphRect.center().y(), glyphRect.right(), glyphRect.center().y() );
                        } else {
                            drawXPArrowButton( painter, &optBtn, Qt::DownArrow, 7, 7 ); // SpinDownGlyph 7x7
                        }
                    }
                }
            }
            return;
        }

        case CC_ToolButton: {
            const QToolBar* toolBar = widget
                ? qobject_cast<const QToolBar*>( widget->parentWidget() ) : nullptr;
            if ( const QStyleOptionToolButton* optionToolButton = qstyleoption_cast<const QStyleOptionToolButton*>( option ) ) {
                QRect buttonRect = subControlRect( control, option, SC_ToolButton, widget );
                QStyle::State buttonState = option->state & ~State_Sunken;
                if ( option->state & State_Sunken ) {
                    if ( optionToolButton->activeSubControls & SC_ToolButton )
                        buttonState |= State_Sunken;
                    else if ( optionToolButton->activeSubControls & SC_ToolButtonMenu )
                        buttonState |= State_MouseOver;
                }
                bool selected = buttonState & State_MouseOver && option->state & State_Enabled;
                bool checked = buttonState & State_On;
                bool sunken = buttonState & State_Sunken;
                if ( selected || checked || sunken ) {
                    QRect rect = buttonRect.adjusted( 0, 0, -1, -1 );
                    painter->setPen( m_colorItemBorder );
                    QLinearGradient gradient;
                    if ( toolBar && toolBar->orientation() == Qt::Vertical )
                        gradient = QLinearGradient( rect.topLeft(), rect.topRight() );
                    else
                        gradient = QLinearGradient( rect.topLeft(), rect.bottomLeft() );
                    if ( sunken || ( selected && checked ) ) {
                        gradient.setColorAt( 0.0, m_colorItemSunkenBegin );
                        gradient.setColorAt( 0.5, m_colorItemSunkenMiddle );
                        gradient.setColorAt( 1.0, m_colorItemSunkenEnd );
                    } else if ( checked ) {
                        gradient.setColorAt( 0.0, m_colorItemCheckedBegin );
                        gradient.setColorAt( 0.5, m_colorItemCheckedMiddle );
                        gradient.setColorAt( 1.0, m_colorItemCheckedEnd );
                    } else {
                        gradient.setColorAt( 0.0, m_colorItemBackgroundBegin );
                        gradient.setColorAt( 0.5, m_colorItemBackgroundMiddle );
                        gradient.setColorAt( 1.0, m_colorItemBackgroundEnd );
                    }
                    painter->setBrush( gradient );
                    painter->drawRect( rect );
                }
                QStyleOptionToolButton optionLabel = *optionToolButton;
                int fw = pixelMetric( PM_DefaultFrameWidth, option, widget );
                optionLabel.rect = buttonRect.adjusted( fw, fw, -fw, -fw );
                drawControl( CE_ToolButtonLabel, &optionLabel, painter, widget );
                if ( optionToolButton->subControls & SC_ToolButtonMenu ) {
                    QStyleOption optionMenu = *optionToolButton;
                    optionMenu.rect = subControlRect( control, option, SC_ToolButtonMenu, widget );
                    optionMenu.state = optionToolButton->state & ~State_Sunken;
                    if ( optionToolButton->state & State_Sunken ) {
                        if ( optionToolButton->activeSubControls & SC_ToolButton )
                            optionMenu.state |= State_MouseOver | State_Sunken;
                        else if ( optionToolButton->activeSubControls & SC_ToolButtonMenu )
                            optionMenu.state |= State_Sunken;
                    }
                    drawPrimitive( PE_IndicatorButtonDropDown, &optionMenu, painter, widget );
                } else if ( optionToolButton->features & QStyleOptionToolButton::HasMenu ) {
                    int size = pixelMetric( PM_MenuButtonIndicator, option, widget );
                    QRect rect = optionToolButton->rect;
                    QStyleOptionToolButton optionArrow = *optionToolButton;
                    optionArrow.rect = QRect( rect.right() + 4 - size, rect.height() - size + 4, size - 5, size - 5 );
                    drawPrimitive( PE_IndicatorArrowDown, &optionArrow, painter, widget );
                }
            }
            return;
        }

        case CC_GroupBox:
            if ( const QStyleOptionGroupBox* groupBox = qstyleoption_cast<const QStyleOptionGroupBox*>( option ) ) {
                if ( groupBox->subControls & SC_GroupBoxCheckBox ) {
                    QStyleOptionButton box;
                    box.QStyleOption::operator=( *groupBox );
                    box.rect = subControlRect( CC_GroupBox, option, SC_GroupBoxCheckBox, widget );
                    drawPrimitive( PE_IndicatorCheckBox, &box, painter, widget );
                }
                if ( groupBox->subControls & SC_GroupBoxFrame ) {
                    QStyleOptionFrame frame;
                    frame.QStyleOption::operator=( *groupBox );
                    frame.features = groupBox->features;
                    frame.lineWidth = groupBox->lineWidth;
                    frame.midLineWidth = groupBox->midLineWidth;
                    frame.rect = subControlRect( CC_GroupBox, option, SC_GroupBoxFrame, widget );
                    drawPrimitive( PE_FrameGroupBox, &frame, painter, widget );
                }
                if ( ( groupBox->subControls & SC_GroupBoxLabel ) && !groupBox->text.isEmpty() ) {
                    QRect textRect = subControlRect( CC_GroupBox, option, SC_GroupBoxLabel, widget );
                    if ( textRect.isValid() ) {
                        // Erase the frame where the title sits, then paint the
                        // Luna Blue title colour (#0046D5 from property sheets).
                        painter->fillRect( textRect.adjusted( -2, 0, 2, 0 ), option->palette.window() );
                        QPalette pal = groupBox->palette;
                        if ( m_forceClassicPalette || m_mode == Blue )
                            pal.setColor( QPalette::WindowText, QColor( 0x00, 0x46, 0xd5 ) );
                        int alignment = int( groupBox->textAlignment ) | Qt::TextShowMnemonic;
                        if ( !styleHint( SH_UnderlineShortcut, option, widget ) )
                            alignment |= Qt::TextHideMnemonic;
                        drawItemText( painter, textRect, alignment | Qt::AlignVCenter, pal,
                            groupBox->state & State_Enabled, groupBox->text, QPalette::WindowText );
                    }
                }
                return;
            }
            break;

        default:
            break;
    }

    QProxyStyle::drawComplexControl( control, option, painter, widget );
}

// Track busy QProgressBars through the event filter installed in polish(),
// starting/stopping the animation timer as bars appear and disappear. Style
// and Paint events re-evaluate a bar so setRange(0, 0) on an already visible
// bar starts animating too (dirtylooks does the same).
bool WinXPStyle::eventFilter( QObject* object, QEvent* event )
{
    switch ( event->type() ) {
        case QEvent::Show:
        case QEvent::StyleChange:
        case QEvent::Paint:
            if ( QProgressBar* bar = qobject_cast<QProgressBar*>( object ) ) {
                if ( bar->isVisible() && bar->minimum() == 0 && bar->maximum() == 0 )
                    addProgressBar( bar );
                else
                    removeProgressBar( bar );
            }
            break;
        case QEvent::Hide:
        case QEvent::Destroy:
            // Only progress bars get our filter installed, so the cast is
            // safe even while the object is being destroyed.
            removeProgressBar( static_cast<QProgressBar*>( object ) );
            break;
        default:
            break;
    }
    return QProxyStyle::eventFilter( object, event );
}

void WinXPStyle::addProgressBar( QProgressBar* bar )
{
    if ( !m_progressBars.contains( bar ) ) {
        m_progressBars.append( bar );
        if ( !m_progressTimer->isActive() )
            m_progressTimer->start( 30 );
    }
}

void WinXPStyle::removeProgressBar( QProgressBar* bar )
{
    m_progressBars.removeAll( bar );
    if ( m_progressBars.isEmpty() )
        m_progressTimer->stop();
}

// Advance every visible busy bar; setValue() repaints the widget itself, so
// no explicit update() is needed. CE_ProgressBarContents maps the value onto
// the travelling chunk position (+2 per tick keeps the motion brisk).
void WinXPStyle::animateProgressBars()
{
    for ( QProgressBar* bar : m_progressBars ) {
        if ( bar->isVisible() && bar->minimum() == 0 && bar->maximum() == 0 )
            bar->setValue( bar->value() + 2 );
    }
}
