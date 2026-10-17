#pragma once
#include <QtWidgets>
#include "theme.hpp"

// GoofyUI materials (github.com/Azec-Zuckerberg/goofy-ui-library): one cream
// paper canvas, white surfaces, chunky 2-3 px ink outlines, big radii, flat
// state colour and hard offset shadows. No gradients, no glow. Colour is
// reserved for live state (the accent, user-selectable) and the semantic
// palette. Placeholders: %1 accent hex, %2 text on accent, %3 accent hover.
inline QString applicationStyle(const QColor& accent){
    const QString accentHex=accent.name();
    const QString onAccent=kbtheme::readableOn(accent).name();
    const QString accentHover=(kbtheme::readableOn(accent)==kbtheme::ink?accent.darker(112):accent.lighter(112)).name();
    return QString::fromUtf8(R"CSS(
        QWidget { color:#31204C; font-family:'Geist'; font-size:13px; font-weight:400; }
        QWidget#mainWindow { background:#FFFAEE; }
        QFrame#titleBar { background:#FFDD59; border:0; border-bottom:3px solid #31204C; }

        QLabel#brand { font-family:'Consolas'; color:#31204C; }
        QLabel#pageTitle,QLabel#preferencesTitle { font-family:'Fredoka'; font-size:24px; font-weight:600; color:#31204C; }
        QLabel#sidebarTitle { font-family:'Fredoka'; font-size:16px; font-weight:600; color:#31204C; }
        QLabel#subtitle,QLabel#hint { color:#5D4973; font-size:12px; }
        QLabel#rowTitle { color:#31204C; font-size:14px; font-weight:600; }
        QLabel#rowHint { color:#5D4973; font-size:12px; }
        QLabel#panelTitle { font-family:'Fredoka'; color:#31204C; font-size:17px; font-weight:600; }
        QLabel#cardTitle { font-family:'Fredoka'; color:#31204C; font-size:18px; font-weight:600; }
        QLabel#sectionArrow { color:#5D4973; font-size:18px; }

        QFrame#favoritesDivider,QFrame#operatorsDivider { background:#EBDEFF; border:0; max-height:2px; }

        /* Sections are cream cards outlined in ink; rows stay flat inside. */
        QFrame#settingCard { background:#FFFAEE; border:2px solid #31204C; border-radius:16px; }
        QWidget#settingRow { background:transparent; border:0; border-radius:10px; }
        QWidget#settingRow:hover { background:#F6F0FF; }

        QLineEdit#search { background:#FFFFFF; color:#31204C; border:2px solid #31204C; border-radius:10px; padding:8px 10px; font-size:13px; }
        QLineEdit#search:focus { border-color:%1; }

        QMenu { background:#FFFFFF; color:#31204C; border:2px solid #31204C; border-radius:12px; padding:4px; font-size:13px; }
        QMenu::item { padding:5px 10px 5px 7px; border-radius:8px; background:transparent; }
        QMenu::item:selected { background:#EBDEFF; color:#31204C; }
        QMenu::icon { padding-left:4px; }
        QMenu::separator { height:2px; background:#EBDEFF; margin:4px 6px; }

        QPushButton#weaponChoice { background:transparent; color:#5D4973; border:0; border-radius:12px; padding:5px 14px; font-size:13px; font-weight:600; }

        QListWidget { background:transparent; border:0; outline:0; }

        QPushButton#sectionHeading { text-align:left; padding:6px 0; border:0; background:transparent; color:#31204C; font-size:16px; font-weight:600; }
        QPushButton#sectionHeading:hover { color:%1; }
        QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl { background:transparent; border:0; padding:0; }

        QPushButton#status { background:#EBDEFF; border:2px solid #31204C; border-radius:10px; padding:6px 12px; color:#31204C; font-size:12px; font-weight:700; }
        QPushButton#status:hover { background:#F6F0FF; }
        QPushButton#status[active="true"] { background:#CCF47F; border-color:#31204C; color:#31204C; }

        QPushButton#preferencesSave,QPushButton#save { background:%1; color:%2; border:2px solid #31204C; border-radius:12px; padding:8px 16px; font-weight:700; }
        QPushButton#preferencesSave:hover,QPushButton#save:hover { background:%3; }
        QPushButton#preferencesSave:disabled,QPushButton#save:disabled { background:#EBDEFF; color:#5D4973; }

        QPushButton#keyButton,QPushButton#secondary,QPushButton#cancelClose,QPushButton#discardChanges { background:#FFFFFF; border:2px solid #31204C; border-radius:12px; padding:6px 12px; color:#31204C; font-size:13px; font-weight:600; }
        QPushButton#keyButton:hover,QPushButton#secondary:hover,QPushButton#cancelClose:hover,QPushButton#discardChanges:hover { background:#F6F0FF; }
        QPushButton#keyButton:pressed,QPushButton#secondary:pressed,QPushButton#cancelClose:pressed,QPushButton#discardChanges:pressed { background:#EBDEFF; }
        QPushButton#cancelClose[keyboardFocus="true"],QPushButton#discardChanges[keyboardFocus="true"] { border-color:%1; }
        QPushButton#keyButton[keyboardFocus="true"],QPushButton#secondary[keyboardFocus="true"],QPushButton#weaponChoice[keyboardFocus="true"],QPushButton#status[keyboardFocus="true"],QPushButton#save[keyboardFocus="true"],QPushButton#preferencesSave[keyboardFocus="true"] { border-color:%1; }

        QDoubleSpinBox,QSpinBox { background:#FFDD59; border:2px solid #31204C; border-radius:9px; padding:5px 7px; color:#31204C; font-family:'Consolas'; font-weight:700; font-size:13px; }
        QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; }
        QDoubleSpinBox:disabled,QSpinBox:disabled { background:#EBDEFF; color:#5D4973; }

        QSlider::groove:horizontal { height:10px; border-radius:5px; background:#EBDEFF; }
        QSlider::sub-page:horizontal { background:%1; border-radius:5px; }
        QSlider::handle:horizontal { width:14px; margin:-4px 0; border:2px solid #31204C; border-radius:9px; background:#FFFFFF; }
        QSlider::handle:horizontal:hover { background:#F6F0FF; }

        QComboBox { background:#FFFFFF; color:#31204C; border:2px solid #31204C; border-radius:10px; padding:6px 10px; min-width:120px; font-size:13px; font-weight:600; }
        QComboBox:focus { border-color:%1; }
        QComboBox::drop-down { border:0; width:22px; }
        QComboBox::down-arrow { image:url(:/ui/chevron.svg); width:12px; height:12px; }
        QComboBox QAbstractItemView { background:#FFFFFF; border:2px solid #31204C; border-radius:10px; padding:4px; selection-background-color:#EBDEFF; selection-color:#31204C; }

        QScrollArea { background:transparent; border:0; }
        QScrollArea>QWidget>QWidget { background:transparent; }
        QScrollBar:vertical { background:transparent; width:8px; margin:2px 0; }
        QScrollBar::handle:vertical { background:rgba(49,32,76,0.22); min-height:36px; border-radius:4px; margin:0 2px; }
        QScrollBar::handle:vertical:hover { background:rgba(49,32,76,0.38); }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }

        QDialog { background:#FFFAEE; }
        QToolTip { background:#FFFFFF; color:#31204C; border:2px solid #31204C; border-radius:10px; padding:6px 8px; font-size:12px; }
        QPushButton:focus { outline:0; }
    )CSS").arg(accentHex,onAccent,accentHover);
}
