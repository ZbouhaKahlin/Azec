#pragma once
#include <QtWidgets>

// Atelier materials (Bridge Clip dark, Cursor-derived): one solid canvas,
// flat translucent layers, hairline rims and recessed wells. No gradients.
// Colour is reserved for live state (the accent, user-selectable) and
// semantics (danger/error); every other cue is ink on grey.
// Placeholders: %1 accent hex, %2 accent 55% border, %3 accent 10% tint,
// %4 accent 36% ring.
inline QString applicationStyle(const QColor& accent){
    const auto rgba=[&](const QColor& color,qreal alpha){
        return QString("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(alpha,0,'f',3);
    };
    const QString accentHex=accent.name();
    const QString accentBorder=rgba(accent,.55);
    const QString accentTint=rgba(accent,.10);
    const QString accentRing=rgba(accent,.36);
    return QString::fromUtf8(R"CSS(
        QWidget { color:#e6e6e6; font-family:'Geist'; font-size:13px; font-weight:400; }
        QWidget#mainWindow { background:#141414; }
        QFrame#titleBar { background:#141414; border:0; }

        QLabel#brand { font-family:'Consolas'; color:#38ccff; }
        QLabel#pageTitle,QLabel#preferencesTitle { font-size:23px; font-weight:600; color:#e6e6e6; }
        QLabel#sidebarTitle { font-size:15px; font-weight:600; color:#e6e6e6; }
        QLabel#subtitle,QLabel#hint { color:#8a8a8a; font-size:12px; }
        QLabel#groupLabel { color:#8a8a8a; font-size:11px; font-weight:600; }
        QLabel#rowTitle { color:#e6e6e6; font-size:14px; font-weight:500; }
        QLabel#rowHint { color:#8a8a8a; font-size:12px; }
        QLabel#panelTitle { color:#e6e6e6; font-size:16px; font-weight:600; }
        QLabel#cardTitle { color:#e6e6e6; font-size:16px; font-weight:600; }
        QLabel#sectionArrow { color:#8a8a8a; font-size:18px; }

        QFrame#favoritesDivider,QFrame#operatorsDivider { background:rgba(255,255,255,0.06); border:0; max-height:1px; }

        /* Panels group tiles; each tile is one flat row over the panel. */
        QFrame#settingCard { background:rgba(255,255,255,0.045); border:1px solid rgba(255,255,255,0.08); border-radius:14px; }
        QWidget#settingRow { background:rgba(255,255,255,0.033); border:1px solid rgba(255,255,255,0.08); border-radius:10px; }
        QWidget#settingRow:hover { background:rgba(255,255,255,0.055); border-color:rgba(255,255,255,0.14); }

        QLineEdit#search { background:rgba(0,0,0,0.26); color:#e6e6e6; border:1px solid rgba(255,255,255,0.085); border-radius:8px; padding:8px 10px; font-size:13px; }
        QLineEdit#search:focus { border-color:%1; background:rgba(0,0,0,0.30); }

        QMenu { background:#252525; color:#b4b4b4; border:1px solid rgba(255,255,255,0.10); border-radius:10px; padding:4px; font-size:13px; }
        QMenu::item { padding:6px 10px 6px 7px; border-radius:8px; background:transparent; }
        QMenu::item:selected { background:rgba(255,255,255,0.08); color:#e6e6e6; }
        QMenu::icon { padding-left:4px; }

        QPushButton#weaponChoice { background:transparent; color:#8a8a8a; border:1px solid transparent; border-radius:8px; padding:4px 12px; font-size:13px; font-weight:500; }

        QListWidget { background:transparent; border:0; outline:0; }

        QPushButton#sectionHeading { text-align:left; padding:6px 0; border:0; background:transparent; color:#e6e6e6; font-size:16px; font-weight:600; }
        QPushButton#sectionHeading:hover { color:white; }
        QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl { background:transparent; border:0; padding:0; }

        QPushButton#status { background:rgba(255,255,255,0.045); border:1px solid rgba(255,255,255,0.09); border-radius:9px; padding:7px 12px; color:#b4b4b4; font-size:12px; font-weight:500; }
        QPushButton#status:hover { color:#e6e6e6; border-color:rgba(255,255,255,0.15); }
        QPushButton#status[active="true"] { color:%1; border-color:%4; background:%3; }

        QPushButton#preferencesSave,QPushButton#save { background:%1; color:#141414; border:1px solid transparent; border-radius:8px; padding:9px 16px; font-weight:600; }

        QPushButton#keyButton,QPushButton#secondary,QPushButton#cancelClose,QPushButton#discardChanges { background:rgba(255,255,255,0.05); border:1px solid rgba(255,255,255,0.09); border-radius:8px; padding:6px 12px; color:#b4b4b4; font-size:13px; font-weight:500; }
        QPushButton#keyButton:hover,QPushButton#secondary:hover,QPushButton#cancelClose:hover,QPushButton#discardChanges:hover { background:rgba(255,255,255,0.08); border-color:rgba(255,255,255,0.14); color:#e6e6e6; }
        QPushButton#keyButton:pressed,QPushButton#secondary:pressed,QPushButton#cancelClose:pressed,QPushButton#discardChanges:pressed { background:rgba(255,255,255,0.04); }
        QPushButton#cancelClose[keyboardFocus="true"],QPushButton#discardChanges[keyboardFocus="true"] { border-color:#8a8a8a; }
        QPushButton#keyButton[keyboardFocus="true"],QPushButton#secondary[keyboardFocus="true"],QPushButton#weaponChoice[keyboardFocus="true"],QPushButton#status[keyboardFocus="true"],QPushButton#save[keyboardFocus="true"],QPushButton#preferencesSave[keyboardFocus="true"] { border-color:#8a8a8a; }

        QDoubleSpinBox,QSpinBox { background:rgba(0,0,0,0.26); border:1px solid rgba(255,255,255,0.085); border-radius:8px; padding:5px 7px; color:#e6e6e6; font-size:13px; }
        QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; }
        QDoubleSpinBox:disabled,QSpinBox:disabled { color:#707070; background:rgba(0,0,0,0.16); }

        QSlider::groove:horizontal { height:8px; border-radius:4px; background:rgba(0,0,0,0.36); }
        QSlider::sub-page:horizontal { background:%1; border-radius:4px; }
        QSlider::handle:horizontal { width:16px; margin:-4px 0; border:0; border-radius:8px; background:#ededed; }
        QSlider::handle:horizontal:hover { background:#ffffff; }

        QComboBox { background:rgba(0,0,0,0.26); color:#e6e6e6; border:1px solid rgba(255,255,255,0.085); border-radius:8px; padding:6px 10px; min-width:120px; font-size:13px; }
        QComboBox:focus { border-color:%1; }
        QComboBox::drop-down { border:0; width:22px; }
        QComboBox::down-arrow { image:url(:/ui/chevron.svg); width:12px; height:12px; }
        QComboBox QAbstractItemView { background:#252525; border:1px solid rgba(255,255,255,0.10); border-radius:8px; padding:4px; selection-background-color:rgba(255,255,255,0.08); selection-color:#e6e6e6; }

        QScrollArea { background:transparent; border:0; }
        QScrollArea>QWidget>QWidget { background:transparent; }
        QScrollBar:vertical { background:transparent; width:8px; margin:2px 0; }
        QScrollBar::handle:vertical { background:rgba(255,255,255,0.13); min-height:36px; border-radius:2px; margin:0 2px; }
        QScrollBar::handle:vertical:hover { background:rgba(255,255,255,0.22); }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }

        QDialog { background:#141414; }
        QToolTip { background:#252525; color:#e6e6e6; border:1px solid rgba(255,255,255,0.09); border-radius:8px; padding:6px 8px; font-size:12px; }
        QPushButton:focus { outline:0; }
    )CSS").arg(accentHex,accentBorder,accentTint,accentRing);
}
