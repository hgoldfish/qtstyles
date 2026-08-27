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

#ifndef WINXPSTYLE_H
#define WINXPSTYLE_H

#include <QList>
#include <QProxyStyle>

class QWidget;
class QTimer;
class QProgressBar;

/**
* WinXP Qt style.
*
* %WinXPStyle is a Qt style which imitates the look
* of MS Office 2003. It changes the style of toolbars, menus, docked
* windows and toolboxes. The color scheme used by this style is
* automatically adjusted to system settings.
*
* The style is built on QProxyStyle so it works on any platform.
* The common controls are drawn by the style itself with a Windows XP
* "Luna" appearance: push buttons, tool buttons, check boxes, radio buttons,
* line edits, combo boxes, spin boxes, sliders, scroll bars, progress bars,
* group boxes, header views and all tab widgets. Widgets the style does not
* re-draw are delegated to the base style. The default
* constructor uses the "Windows" style as the base
* (the same choice as the other qtstyles plugins); on Windows you can keep
* the native look by passing the platform style as the base, e.g.
* QProxyStyle::setBaseStyle(QStyleFactory::create(QStringLiteral("windowsvista"))).
*
* The color scheme is selected with the \a mode constructor argument:
* Blue, Silver and Olive reproduce the fixed Windows XP "Luna" palettes,
* Classic derives the colors from the active QPalette (and is the default).
*
* When \a forceClassicPalette is true (the "winxp-classic" key), the style
* forces \c standardPalette() — the authentic Luna Blue Control Panel
* Colors — through polish(QPalette&), and draws dialog chrome to match
* the default XP property-sheet look (beige faces, orange selected-tab
* stripe, blue group-box titles) instead of the Office 2003 candy chrome.
*/
class WinXPStyle : public QProxyStyle
{
    Q_OBJECT
public:
    /**
    * Color scheme of the style.
    */
    enum Mode
    {
        Blue,
        Silver,
        Olive,
        Classic
    };

    /**
    * Default constructor.
    * @param forceClassicPalette when true, polish(QPalette&) installs
    *        standardPalette() (Luna Blue) and dialog-faithful chrome is used.
    */
    WinXPStyle( Mode mode = Classic, bool forceClassicPalette = false );

    /**
    * Destructor.
    */
    ~WinXPStyle();

public: // overrides
    void polish( QPalette& palette ) override;

    void polish( QWidget* widget ) override;
    void unpolish( QWidget* widget ) override;

    QPalette standardPalette() const override;

    int pixelMetric( PixelMetric metric, const QStyleOption* option, const QWidget* widget ) const override;

    QSize sizeFromContents( ContentsType type, const QStyleOption* option,
        const QSize& contentsSize, const QWidget* widget ) const override;

    QRect subElementRect( SubElement element, const QStyleOption* option, const QWidget* widget ) const override;

    void drawPrimitive( PrimitiveElement element, const QStyleOption* option,
        QPainter* painter, const QWidget* widget ) const override;
    void drawControl( ControlElement element, const QStyleOption* option,
        QPainter* painter, const QWidget* widget ) const override;
    void drawComplexControl( ComplexControl control, const QStyleOptionComplex* option,
        QPainter* painter, const QWidget* widget ) const override;

    int layoutSpacing( QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
        Qt::Orientation orientation, const QStyleOption* option, const QWidget* widget ) const override;

    bool eventFilter( QObject* object, QEvent* event ) override;

private:
    /**
    * Colors of the common controls drawn by the style (inputs, check boxes,
    * radio buttons, sliders, scroll bars, progress bars, combo boxes, spin
    * boxes, headers and group boxes). The values reproduce the Windows XP
    * "Luna" look for the three fixed schemes; Classic derives them from the
    * active QPalette.
    */
    struct ControlColors
    {
        // line edits, combo box and spin box fields
        QColor fieldBorder;
        QColor fieldBorderHot;
        QColor fieldBackground;
        QColor fieldBackgroundDisabled;
        QColor fieldTextDisabled;

        // check box / radio button indicators
        QColor indicatorBorder;
        QColor indicatorHotBorder;
        QColor indicatorCheck;
        QColor indicatorDot;

        // arrow-button face (scroll bars, combo box, spin box)
        QColor arrowFaceBegin;
        QColor arrowFaceEnd;
        QColor arrowBorder;
        QColor arrowGlyph;
        QColor arrowGlyphDisabled;

        // scroll bar track, slider groove
        QColor track;
        QColor trackBorder;

        // slider thumb
        QColor thumbFaceBegin;
        QColor thumbFaceEnd;
        QColor thumbBorder;
        QColor thumbGripper;

        // progress bar
        QColor progressBorder;
        QColor progressGroove;
        QColor progressChunk;

        // header sections
        QColor headerBegin;
        QColor headerEnd;
        QColor headerBorder;

        // group box frame
        QColor groupBoxBorder;
        QColor groupBoxBorderLight;
    };

    /**
    * Draws a Windows XP "Luna" push button (gradient face, colored border and
    * a soft transition animation when the state changes) entirely with QPainter,
    * without relying on a Windows theme engine or a base style.
    */
    void drawXPButton( QPainter* painter, const QStyleOption* option ) const;

    // Common-control drawing helpers, all plain QPainter (Luna look).
    void drawXPEditField( QPainter* painter, const QStyleOption* option ) const;
    void drawXPIndicator( QPainter* painter, const QStyleOption* option, bool radio, bool checked ) const;
    void drawXPButtonFace( QPainter* painter, const QStyleOption* option ) const;
    void drawXPArrowButton( QPainter* painter, const QStyleOption* option,
        Qt::ArrowType arrow, int glyphWidth, int glyphHeight ) const;
    void drawXPScrollThumb( QPainter* painter, const QRect& rect, const QStyleOption* option ) const;
    void drawXPSliderGroove( QPainter* painter, const QStyleOption* option ) const;
    void drawXPSliderThumb( QPainter* painter, const QStyleOption* option ) const;
    void drawXPProgressGroove( QPainter* painter, const QStyleOption* option ) const;
    void drawXPProgressChunk( QPainter* painter, const QRect& rect, const QStyleOption* option ) const;
    void drawXPHeaderSection( QPainter* painter, const QStyleOption* option ) const;
    void drawXPGroupBox( QPainter* painter, const QStyleOption* option ) const;
    void drawXPSplitterHandle( QPainter* painter, const QStyleOption* option ) const;

public:
    enum {
        /**
        * Primitive element filled with the main window's background gradient.
        */
        PE_WindowGradient = PE_CustomBase + 1
    };

private:
    Mode m_mode;
    bool m_forceClassicPalette;
    ControlColors m_colors;

    // Busy (indeterminate) QProgressBar animation. A single QTimer advances
    // the value of every visible busy bar (phase/dirtylooks do the same); the
    // bars are tracked through the event filter installed in polish().
    void addProgressBar( QProgressBar* bar );
    void removeProgressBar( QProgressBar* bar );
    void animateProgressBars();

    QTimer* m_progressTimer;
    QList<QProgressBar*> m_progressBars;

    // main window, toolbox
    QColor m_colorBackgroundBegin;
    QColor m_colorBackgroundEnd;

    // menu
    QColor m_colorMenuBorder;
    QColor m_colorMenuBackground;
    QColor m_colorMenuTitleBegin;
    QColor m_colorMenuTitleEnd;

    // toolbar, tab, toolbox
    QColor m_colorBarBegin;
    QColor m_colorBarMiddle;
    QColor m_colorBarEnd;

    // toolbar handle
    QColor m_colorHandle;
    QColor m_colorHandleLight;

    // menu, toolbar
    QColor m_colorSeparator;
    QColor m_colorSeparatorLight;

    // menu, toolbar, tab, toolbox
    QColor m_colorItemBorder;
    QColor m_colorItemBackgroundBegin;
    QColor m_colorItemBackgroundMiddle;
    QColor m_colorItemBackgroundEnd;
    QColor m_colorItemCheckedBegin;
    QColor m_colorItemCheckedMiddle;
    QColor m_colorItemCheckedEnd;
    QColor m_colorItemSunkenBegin;
    QColor m_colorItemSunkenMiddle;
    QColor m_colorItemSunkenEnd;

    // toolbar shadow, tab, toolbox tab
    QColor m_colorBorder;
    QColor m_colorBorderLight;
};

#endif
