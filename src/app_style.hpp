#pragma once
#include <QtWidgets>
#include "theme.hpp"

// Monocode materials (github.com/hardbeat920/monocode, MIT): hairline
// separators, translucent content fills, small radii, one blue accent and a
// quiet motion language. Placeholders: %1 accent hex, %2 text on accent;
// material tokens follow below and carry their alpha.
inline QString applicationStyle(const QColor& accent){

    const QString accentHex=accent.name();
    const QString onAccent=kbtheme::readableOn(accent).name();
    QString style=QString::fromUtf8(R"CSS(
        QWidget { color:$ink; font-family:'Segoe UI'; font-size:13px; font-weight:400; }
        QWidget#mainWindow { background:transparent; }
        QFrame#titleBar { background:transparent; border:0; }

        QLabel#brand { color:$ink; font-size:15px; font-weight:600; letter-spacing:0.2px; }
        QLabel#pageTitle,QLabel#preferencesTitle { font-size:20px; font-weight:600; color:$ink; }
        QLabel#sidebarTitle { font-size:15px; font-weight:600; color:$ink; }
        QLabel#subtitle,QLabel#hint { color:$muted; font-size:12px; }
        QLabel#phoneAddress { background:$card; color:$ink; border:1px solid $outline; border-radius:6px; padding:6px 8px; font-family:'Consolas'; font-size:12px; }
        QLabel#pairingCode { background:$soft; color:$ink; border:1px solid $outline; border-radius:6px; padding:4px 9px; font-family:'Consolas'; font-size:16px; font-weight:700; }
        QLabel#rowTitle { color:$ink; font-size:13px; font-weight:600; }
        QLabel#rowHint { color:$muted; font-size:12px; font-weight:400; }
        QTextEdit#codeText { background:transparent; color:$ink; border:0; padding:0; font-family:'Consolas'; font-size:12px; }
        QWidget#codeBlock { background:$card; border:1px solid $outline; border-radius:8px; }
        QFrame#driverGuide { background:transparent; border:0; }
        QLabel#driverStepNumber { background:$card; color:$muted; border:0; border-radius:8px; font-size:12px; font-weight:600; }
        QLabel#driverStepTitle { color:$ink; font-size:14px; font-weight:600; }
        QPushButton#driverDownload { background:%1; color:%2; border:0; border-radius:10px; padding:8px 14px; font-size:13px; font-weight:600; }
        QPushButton#driverDownload:hover { background:$accentHover; }
        QPushButton#driverDownload:pressed { background:$accentPressed; }
        QPushButton#driverDownload:disabled { background:$card; color:$muted; }
        QPushButton#driverManualDownload,QPushButton#driverTroubleshooting { background:transparent; color:$muted; border:0; border-radius:6px; padding:7px 4px; font-size:12px; text-align:$sectionAlign; }
        QPushButton#driverManualDownload:hover,QPushButton#driverTroubleshooting:hover { background:$card; color:$ink; }
        QPushButton#driverTroubleshooting:checked { color:$ink; }
        QPushButton#driverDownload[keyboardFocus="true"],QPushButton#driverManualDownload[keyboardFocus="true"],QPushButton#driverTroubleshooting[keyboardFocus="true"] { border:1px solid %1; }
        QLabel#driverState { border:0; padding:4px 0; background:transparent; color:$muted; font-size:12px; font-weight:400; }
        QLabel#driverState[state="ready"] { color:#6DD5AD; }
        QLabel#driverState[state="blocked"] { color:$yellow; }
        QFrame#driverDivider { background:$outline; border:0; min-height:1px; max-height:1px; }
        QLabel#panelTitle { color:$ink; font-size:15px; font-weight:600; }
        QLabel#cardTitle { color:$ink; font-size:16px; font-weight:600; }
        QLabel#sectionArrow { color:$muted; font-size:16px; }

        QFrame#favoritesDivider,QFrame#operatorsDivider { background:$outline; border:0; max-height:1px; }

        /* Cards are a 5% content wash over the glass with one hairline. */
        QFrame#settingCard { background:$card; border:1px solid $outline; border-radius:10px; }
        QWidget#settingRow { background:transparent; border:0; border-radius:8px; }

        QLineEdit#search { background:$card; color:$ink; border:1px solid $outline; border-radius:8px; padding:8px 10px; font-size:13px; }
        QLineEdit#search:focus { border-color:%1; background:$soft; }

        QMenu { background:$surface; color:$ink; border:1px solid $outline; border-radius:10px; padding:5px; font-size:13px; }
        QMenu::item { padding:6px 10px 6px 7px; border-radius:6px; background:transparent; }
        QMenu::item:selected { background:$soft; color:$ink; }
        QMenu::icon { padding-left:4px; }
        QMenu::separator { height:1px; background:$outline; margin:5px 8px; }

        QPushButton#weaponChoice { background:transparent; color:$muted; border:0; border-radius:8px; padding:5px 14px; font-size:13px; font-weight:600; }

        QListWidget { background:transparent; border:0; outline:0; }

        QPushButton#sectionHeading { text-align:$sectionAlign; padding:6px 0; border:0; background:transparent; color:$ink; font-size:15px; font-weight:600; }
        QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl { background:transparent; border:0; padding:0; }

        QPushButton#status, QPushButton#statusRapid { background:$card; border:0; border-radius:16px; padding:0 13px; color:$ink; font-size:12px; font-weight:400; }
        QPushButton#status:hover, QPushButton#statusRapid:hover { background:$soft; }
        QPushButton#status[active="true"], QPushButton#statusRapid[active="true"] { background:rgba(69,155,247,0.18); color:#A4CEFF; }
        QPushButton#status[active="true"]:hover, QPushButton#statusRapid[active="true"]:hover { background:rgba(69,155,247,0.26); }

        QPushButton#preferencesSave,QPushButton#save { background:%1; color:%2; border:0; border-radius:10px; padding:8px 16px; font-weight:600; }
        QPushButton#preferencesSave:hover,QPushButton#save:hover { background:$accentHover; }
        QPushButton#preferencesSave:pressed,QPushButton#save:pressed { background:$accentPressed; }
        QPushButton#preferencesSave:disabled,QPushButton#save:disabled { background:$card; color:$muted; }

        QPushButton#keyButton,QPushButton#secondary,QPushButton#cancelClose,QPushButton#discardChanges { background:$card; border:0; border-radius:10px; padding:8px 13px; color:$ink; font-size:13px; font-weight:400; }
        QPushButton#keyButton:hover,QPushButton#secondary:hover,QPushButton#cancelClose:hover,QPushButton#discardChanges:hover,QPushButton#keyButton:pressed,QPushButton#secondary:pressed,QPushButton#cancelClose:pressed,QPushButton#discardChanges:pressed { background:$soft; }
        QPushButton#secondary:disabled { color:$muted; background:transparent; border-color:$outline; }
        QPushButton#cancelClose[keyboardFocus="true"],QPushButton#discardChanges[keyboardFocus="true"] { border:1px solid %1; }
        QPushButton#keyButton[keyboardFocus="true"],QPushButton#secondary[keyboardFocus="true"],QPushButton#weaponChoice[keyboardFocus="true"],QPushButton#status[keyboardFocus="true"],QPushButton#statusRapid[keyboardFocus="true"],QPushButton#save[keyboardFocus="true"],QPushButton#preferencesSave[keyboardFocus="true"] { border:1px solid %1; }

        QDoubleSpinBox,QSpinBox { background:$card; border:1px solid transparent; border-radius:8px; padding:5px 7px; color:$ink; font-family:'Consolas'; font-weight:400; font-size:13px; selection-background-color:%1; }
        QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; background:$soft; }
        QDoubleSpinBox:disabled,QSpinBox:disabled { background:transparent; color:$muted; }

        /* Slider precision chips sit above each track and stay flat until pressed. */
        QPushButton#stepButton { background:$card; border:0; border-radius:12px; color:$ink; font-size:14px; font-weight:400; padding:0; }
        QPushButton#stepButton:hover,QPushButton#stepButton:pressed { background:$soft; }
        QPushButton#stepButton:disabled { background:transparent; color:$muted; border-color:$outline; }
        QPushButton#stepButton[keyboardFocus="true"] { border:1px solid %1; }

        /* A quiet 4 px lane, an accent fill and a plain white thumb. */
        QSlider { min-height:20px; }
        QSlider::groove:horizontal { height:4px; border-radius:2px; background:$soft; }
        QSlider::sub-page:horizontal { background:%1; border-radius:2px; }
        QSlider::handle:horizontal { width:14px; margin:-5px 0; border:0; border-radius:7px; background:#FFFFFF; }
        QSlider::handle:horizontal:hover { background:#FFFFFF; }

        QComboBox { background:$card; color:$ink; border:1px solid transparent; border-radius:8px; padding:6px 10px; min-width:120px; font-size:13px; font-weight:400; }
        QComboBox:focus { border-color:%1; }
        QComboBox::drop-down { border:0; width:22px; }
        QComboBox::down-arrow { width:0; height:0; border-left:4px solid transparent; border-right:4px solid transparent; border-top:5px solid $muted; margin-right:6px; }
        QComboBox QAbstractItemView { background:$surface; color:$ink; border:1px solid $outline; border-radius:8px; padding:4px; selection-background-color:$soft; selection-color:$ink; }

        QScrollArea { background:transparent; border:0; }
        QScrollArea>QWidget>QWidget { background:transparent; }
        QScrollBar:vertical { background:transparent; width:7px; margin:2px 0; }
        QScrollBar::handle:vertical { background:rgba(235,235,235,56); min-height:36px; border-radius:3px; margin:0 2px; }
        QScrollBar::handle:vertical:hover { background:rgba(235,235,235,92); }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }

        QDialog { background:#1B1B1B; border:1px solid $outline; border-radius:12px; }
        QDialog#hotkeyDialog,QDialog#helpDialog,QDialog#unsavedDialog,QDialog#driverDialog,QDialog#palestinePopup { background:#1B1B1B; border:1px solid $outline; border-radius:14px; }
        QLabel#bindingHint { color:$ink; font-size:15px; font-weight:600; }
        QLabel#bindingHint[invalid="true"] { color:$danger; }
        QLabel#palestineArabic { color:$ink; font-size:24px; font-weight:600; }
        QToolTip { background:$surface; color:$ink; border:1px solid $outline; border-radius:6px; padding:6px 8px; font-size:12px; }
        QPushButton:focus { outline:0; }
    )CSS").arg(accentHex,onAccent);
    style.replace(QLatin1String("$accentHover"),accent.lighter(110).name());
    style.replace(QLatin1String("$accentPressed"),accent.darker(112).name());
    const std::pair<const char*,QColor> tokens[]={{"$ink",kbtheme::ink},{"$paper",kbtheme::paper},{"$surface",kbtheme::surface},{"$yellow",kbtheme::yellow},{"$muted",kbtheme::muted},{"$soft",kbtheme::lavender},{"$lime",kbtheme::lime},{"$outline",kbtheme::outline},{"$card",kbtheme::card},{"$chipInk",kbtheme::chipInk},{"$danger",kbtheme::danger}};
    for(const auto& token:tokens)style.replace(QLatin1String(token.first),kbtheme::css(token.second));
    // Qt mirrors layouts but not explicit alignments, so headings follow the
    // reading direction of the selected language.
    style.replace(QLatin1String("$sectionAlign"),QApplication::layoutDirection()==Qt::RightToLeft?"right":"left");
    return style;
}
