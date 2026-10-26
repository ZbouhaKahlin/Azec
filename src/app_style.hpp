#pragma once
#include <QtWidgets>
#include "theme.hpp"

// Monocode materials (github.com/hardbeat920/monocode, MIT): hairline
// separators, translucent content fills, small radii, one blue accent and a
// quiet motion language. Placeholder: %1 accent hex;
// material tokens follow below and carry their alpha.
inline QString applicationStyle(const QColor& accent){

    const QString accentHex=accent.name();
    QString style=QString::fromUtf8(R"CSS(
        QWidget { color:$ink; font-family:'Segoe UI'; font-size:13px; font-weight:400; }
        QWidget#mainWindow { background:transparent; }
        QFrame#titleBar { background:transparent; border:0; }
        /* Native MonoButton owns painting; QSS supplies only type and sizing. */
        QPushButton { background:transparent; border:0; padding:4px 10px; font-size:12px; font-weight:400; }
        QPushButton#status,QPushButton#statusRapid { padding:0 13px; }
        QPushButton#weaponChoice { padding:4px 10px; }
        QPushButton#sectionHeading { padding:4px 0; font-size:13px; font-weight:600; }
        QPushButton#driverManualDownload,QPushButton#driverTroubleshooting { padding:4px 6px; }
        QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl,QPushButton#stepButton { padding:0; }
        QPushButton:focus { outline:0; }


        QLabel#brand { color:$ink; font-size:13px; font-weight:500; letter-spacing:0.2px; }
        QLabel#pageTitle,QLabel#preferencesTitle { font-size:18px; font-weight:500; color:$ink; }
        QLabel#sidebarTitle { font-size:15px; font-weight:600; color:$ink; }
        QLabel#subtitle,QLabel#hint { color:$muted; font-size:12px; }
        QLabel#phoneAddress { background:transparent; color:$ink; border:0; padding:4px 0; font-family:'Consolas'; font-size:12px; }
        QLabel#pairingCode { background:transparent; color:$ink; border:0; padding:4px 0; font-family:'Consolas'; font-size:20px; font-weight:400; letter-spacing:1px; }
        QLabel#rowTitle { color:$ink; font-size:13px; font-weight:500; }
        QLabel#sectionTitle { color:$ink; font-size:13px; font-weight:600; }
        QLabel#rowHint { color:$muted; font-size:12px; font-weight:400; }
        QLabel#shortcutTitle { color:$ink; font-size:20px; font-weight:500; }
        QPushButton#keyButton { font-family:'Consolas'; font-size:15px; }
        QTextEdit#codeText { background:transparent; color:$ink; border:0; padding:0; font-family:'Consolas'; font-size:12px; }
        QWidget#codeBlock { background:$card; border:1px solid $outline; border-radius:8px; }
        QFrame#driverGuide { background:transparent; border:0; }
        QLabel#driverStepNumber { background:transparent; color:$muted; border:0; font-size:11px; font-weight:400; }
        QLabel#driverStepTitle { color:$ink; font-size:13px; font-weight:500; }
        QLabel#driverState { border:0; padding:4px 0; background:transparent; color:$muted; font-size:12px; font-weight:400; }
        QLabel#driverState[state="ready"] { color:$successInk; }
        QLabel#driverState[state="blocked"] { color:$yellow; }
        QFrame#driverDivider { background:$outline; border:0; min-height:1px; max-height:1px; }
        QLabel#panelTitle { color:$ink; font-size:15px; font-weight:600; }
        QLabel#cardTitle { color:$ink; font-size:16px; font-weight:600; }
        QLabel#sectionArrow { color:$muted; font-size:16px; }

        QFrame#favoritesDivider,QFrame#operatorsDivider { background:$outline; border:0; max-height:1px; }

        /* Open profile and Settings sections share the same quiet hierarchy. */
        QFrame#settingCard { background:transparent; border:0; }
        QWidget#settingGroup { background:$card; border:1px solid $outline; border-radius:12px; }
        QWidget#profileGroup,QWidget#settingsGroup { background:transparent; border:0; }
        QWidget#weaponChoices { background:transparent; border:0; border-bottom:1px solid $outline; }
        QFrame#profileDivider { background:$outline; border:0; }
        QFrame[openProfile="true"] QLabel#sectionTitle,QFrame[openSettings="true"] QLabel#sectionTitle { color:$muted; font-size:12px; font-weight:500; }
        QWidget#settingRow { background:transparent; border:0; border-radius:8px; }
        QWidget#settingsSurface QWidget#codeBlock { background:transparent; border:0; border-bottom:1px solid $outline; border-radius:0; }
        QWidget#settingsSurface QComboBox,QWidget#settingsSurface QLineEdit,
        QWidget#settingsSurface QDoubleSpinBox,QWidget#settingsSurface QSpinBox {
            background:transparent; border:1px solid transparent; border-bottom:1px solid $outline; border-radius:0; padding:6px 7px;
        }
        QWidget#settingsSurface QComboBox:focus,QWidget#settingsSurface QLineEdit:focus,
        QWidget#settingsSurface QDoubleSpinBox:focus,QWidget#settingsSurface QSpinBox:focus { border-color:%1; background:transparent; }
        QWidget#settingsSurface QComboBox:disabled,QWidget#settingsSurface QLineEdit:disabled,
        QWidget#settingsSurface QDoubleSpinBox:disabled,QWidget#settingsSurface QSpinBox:disabled { color:$muted; }

        QLineEdit#search { background:transparent; color:$ink; border:0; border-radius:0; padding:8px 10px; font-size:13px; }

        QMenu { background:$surface; color:$ink; border:1px solid $outline; border-radius:10px; padding:5px; font-size:13px; }
        QMenu::item { padding:6px 10px 6px 7px; border-radius:6px; background:transparent; }
        QMenu::item:selected { background:$soft; color:$ink; }
        QMenu::icon { padding-left:4px; }
        QMenu::separator { height:1px; background:$outline; margin:5px 8px; }


        QListWidget { background:transparent; border:0; outline:0; }





        QDoubleSpinBox,QSpinBox { background:$card; border:1px solid transparent; border-radius:8px; padding:5px 7px; color:$ink; font-family:'Consolas'; font-weight:400; font-size:13px; selection-background-color:%1; }
        QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; background:$soft; }
        QDoubleSpinBox:disabled,QSpinBox:disabled { background:transparent; color:$muted; }
        QDoubleSpinBox#recoilValue { background:transparent; border:1px solid transparent; border-radius:6px; padding:2px 0; font-size:22px; }
        QDoubleSpinBox#recoilValue:focus { border-color:%1; }
        QSpinBox#delayValue { background:transparent; border:0; padding:0; }


        /* A quiet 4 px lane, an accent fill and a plain white thumb. */
        QSlider { min-height:20px; }
        QSlider::groove:horizontal { height:4px; border-radius:2px; background:$soft; }
        QSlider::sub-page:horizontal { background:%1; border-radius:2px; }
        QSlider::handle:horizontal { width:14px; margin:-5px 0; border:0; border-radius:7px; background:$sliderThumb; }
        QSlider::handle:horizontal:hover { background:$sliderThumb; }

        QComboBox { background:$card; color:$ink; border:1px solid transparent; border-radius:8px; padding:6px 10px; min-width:120px; font-size:13px; font-weight:400; }
        QComboBox:hover { background:$soft; }
        QComboBox:focus { border-color:%1; }
        QComboBox::drop-down { border:0; width:22px; }
        QComboBox::down-arrow { image:none; width:0; height:0; border:0; }
        QComboBox QAbstractItemView { background:$surface; color:$ink; border:1px solid $outline; border-radius:8px; padding:4px; selection-background-color:$soft; selection-color:$ink; }

        QScrollArea { background:transparent; border:0; }
        QScrollArea>QWidget>QWidget { background:transparent; }
        QScrollBar:vertical { background:transparent; width:7px; margin:2px 0; }
        QScrollBar::handle:vertical { background:$scrollbar; min-height:36px; border-radius:3px; margin:0 2px; }
        QScrollBar::handle:vertical:hover { background:$scrollbarHover; }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }

        QDialog { background:$dialog; border:1px solid $outline; border-radius:12px; }
        QDialog#hotkeyDialog,QDialog#helpDialog,QDialog#unsavedDialog,QDialog#driverDialog,QDialog#palestinePopup { background:$dialog; border:1px solid $outline; border-radius:14px; }
        QLabel#bindingHint { color:$ink; font-size:15px; font-weight:600; }
        QLabel#bindingHint[invalid="true"] { color:$danger; }
        QLabel#palestineArabic { color:$ink; font-size:24px; font-weight:600; }
        QToolTip { background:$surface; color:$ink; border:1px solid $outline; border-radius:6px; padding:6px 8px; font-size:12px; }
    )CSS").arg(accentHex);
    const std::pair<const char*,QColor> tokens[]={{"$dialog",kbtheme::dialog},{"$sliderThumb",kbtheme::sliderThumb},{"$successInk",kbtheme::successInk},{"$scrollbarHover",kbtheme::content(.36)},{"$scrollbar",kbtheme::content(.22)},{"$ink",kbtheme::ink},{"$surface",kbtheme::surface},{"$yellow",kbtheme::yellow},{"$muted",kbtheme::muted},{"$soft",kbtheme::lavender},{"$outline",kbtheme::outline},{"$card",kbtheme::card},{"$danger",kbtheme::danger}};
    for(const auto& token:tokens)style.replace(QLatin1String(token.first),kbtheme::css(token.second));
    return style;
}
