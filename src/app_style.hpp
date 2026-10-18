#pragma once
#include <QtWidgets>
#include "theme.hpp"

// GoofyUI materials (github.com/Azec-Zuckerberg/goofy-ui-library): named
// palettes, chunky 2-3 px outlines, rounded controls and hard offset shadows.
// Placeholders: %1 accent hex, %2 text on accent; material tokens follow below.
inline QString applicationStyle(const QColor& accent){
    if(presentation::isRally())return rally::style(accent);
    const QString accentHex=accent.name();
    const QString onAccent=kbtheme::readableOn(accent).name();
    QString style=QString::fromUtf8(R"CSS(
        QWidget { color:$ink; font-family:'Geist'; font-size:13px; font-weight:400; }
        QWidget#mainWindow { background:$paper; }
        QFrame#titleBar { background:$header; border:0; }

        QLabel#brand { font-family:'Press Start 2P'; color:#FFFFFF; }
        QLabel#pageTitle,QLabel#preferencesTitle { font-family:'Fredoka'; font-size:24px; font-weight:600; color:$ink; }
        QLabel#sidebarTitle { font-family:'Fredoka'; font-size:16px; font-weight:600; color:$ink; }
        QLabel#subtitle,QLabel#hint { color:$muted; font-size:12px; }
        QLabel#rowTitle { color:$ink; font-size:14px; font-weight:600; }
        QLabel#rowHint { color:$muted; font-size:12px; }
        QLabel#panelTitle { font-family:'Fredoka'; color:$ink; font-size:17px; font-weight:600; }
        QLabel#cardTitle { font-family:'Fredoka'; color:$ink; font-size:18px; font-weight:600; }
        QLabel#sectionArrow { color:$muted; font-size:18px; }

        QFrame#favoritesDivider,QFrame#operatorsDivider { background:$soft; border:0; max-height:2px; }

        /* Sections are cream cards outlined in ink; rows stay flat inside. */
        QFrame#settingCard { background:$card; border:2px solid $outline; border-radius:16px; }
        QWidget#settingRow { background:transparent; border:0; border-radius:10px; }

        QLineEdit#search { background:$surface; color:$ink; border:2px solid $outline; border-radius:10px; padding:8px 10px; font-size:13px; }
        QLineEdit#search:focus { border-color:%1; }

        QMenu { background:$surface; color:$ink; border:2px solid $outline; border-radius:12px; padding:4px; font-size:13px; }
        QMenu::item { padding:5px 10px 5px 7px; border-radius:8px; background:transparent; }
        QMenu::item:selected { background:$soft; color:$ink; }
        QMenu::icon { padding-left:4px; }
        QMenu::separator { height:2px; background:$soft; margin:4px 6px; }

        QPushButton#weaponChoice { background:transparent; color:$muted; border:0; border-radius:12px; padding:5px 14px; font-size:13px; font-weight:600; }

        QListWidget { background:transparent; border:0; outline:0; }

        QPushButton#sectionHeading { text-align:left; padding:6px 0; border:0; background:transparent; color:$ink; font-size:16px; font-weight:600; }
        QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl { background:transparent; border:0; padding:0; }

        QPushButton#status { background:$soft; border:2px solid $outline; border-radius:10px; padding:6px 12px; color:$ink; font-size:12px; font-weight:700; }
        QPushButton#status[active="true"] { background:$lime; border-color:$outline; color:$chipInk; }

        QPushButton#preferencesSave,QPushButton#save { background:%1; color:%2; border:2px solid $outline; border-radius:12px; padding:8px 16px; font-weight:700; }
        QPushButton#preferencesSave:disabled,QPushButton#save:disabled { background:$soft; color:$muted; }

        QPushButton#keyButton,QPushButton#secondary,QPushButton#cancelClose,QPushButton#discardChanges { background:$surface; border:2px solid $outline; border-radius:12px; padding:6px 12px; color:$ink; font-size:13px; font-weight:600; }
        QPushButton#keyButton:pressed,QPushButton#secondary:pressed,QPushButton#cancelClose:pressed,QPushButton#discardChanges:pressed { background:$soft; }
        QPushButton#cancelClose[keyboardFocus="true"],QPushButton#discardChanges[keyboardFocus="true"] { border-color:%1; }
        QPushButton#keyButton[keyboardFocus="true"],QPushButton#secondary[keyboardFocus="true"],QPushButton#weaponChoice[keyboardFocus="true"],QPushButton#status[keyboardFocus="true"],QPushButton#save[keyboardFocus="true"],QPushButton#preferencesSave[keyboardFocus="true"] { border-color:%1; }

        QDoubleSpinBox,QSpinBox { background:$yellow; border:2px solid $outline; border-radius:9px; padding:5px 7px; color:$chipInk; font-family:'Consolas'; font-weight:700; font-size:13px; }
        QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; }
        QDoubleSpinBox:disabled,QSpinBox:disabled { background:$soft; color:$muted; }

        /* The 20 px minimum keeps the round handle inside the widget bounds. */
        QSlider { min-height:20px; }
        QSlider::groove:horizontal { height:10px; border-radius:5px; background:$soft; }
        QSlider::sub-page:horizontal { background:%1; border-radius:5px; }
        QSlider::handle:horizontal { width:14px; margin:-4px 0; border:2px solid $outline; border-radius:9px; background:#FFFFFF; }

        QComboBox { background:$surface; color:$ink; border:2px solid $outline; border-radius:10px; padding:6px 10px; min-width:120px; font-size:13px; font-weight:600; }
        QComboBox:focus { border-color:%1; }
        QComboBox::drop-down { border:0; width:22px; }
        QComboBox::down-arrow { image:none; width:12px; height:12px; }
        QComboBox QAbstractItemView { background:$surface; border:2px solid $outline; border-radius:10px; padding:4px; selection-background-color:$soft; selection-color:$ink; }

        QScrollArea { background:transparent; border:0; }
        QScrollArea>QWidget>QWidget { background:transparent; }
        QScrollBar:vertical { background:transparent; width:8px; margin:2px 0; }
        QScrollBar::handle:vertical { background:$muted; min-height:36px; border-radius:4px; margin:0 2px; }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }

        QDialog { background:$paper; }
        QDialog#hotkeyDialog { background:$paper; border:3px solid $outline; border-radius:24px; }
        QDialog#palestinePopup { background:$paper; border:3px solid $outline; border-radius:24px; }
        QLabel#palestineArabic { color:$ink; font-size:24px; font-weight:900; }
        QToolTip { background:$surface; color:$ink; border:2px solid $outline; border-radius:10px; padding:6px 8px; font-size:12px; }
        QPushButton#secondary:disabled { color:$muted; border-color:$soft; }
        QPushButton:focus { outline:0; }
    )CSS").arg(accentHex,onAccent);
    const std::pair<const char*,QColor> tokens[]={{"$ink",kbtheme::ink},{"$paper",kbtheme::paper},{"$surface",kbtheme::surface},{"$yellow",kbtheme::yellow},{"$muted",kbtheme::muted},{"$soft",kbtheme::lavender},{"$lime",kbtheme::lime},{"$outline",kbtheme::outline},{"$card",kbtheme::card},{"$header",kbtheme::header},{"$chipInk",kbtheme::chipInk}};
    for(const auto& token:tokens)style.replace(QLatin1String(token.first),token.second.name());
    return style;
}
