#pragma once
#include <QtWidgets>

inline QString applicationStyle(const QColor& accent){
    return QString::fromUtf8(R"CSS(
            QWidget { color:#dedee5; font-family:'Inter'; font-size:14px; font-weight:400; }
            QWidget#mainWindow { background:#141416; }
            QFrame#titleBar { background:#141416; border:0; }
            QLabel#brand { font-family:'Consolas'; color:#ff5b63; }
            QLabel#pageTitle,QLabel#preferencesTitle { font-size:21px; font-weight:500; color:#f0f0f4; }
            QLabel#sidebarTitle { font-size:15px; font-weight:500; }
            QLabel#hint,QLabel#subtitle { color:#8c8c98; font-size:12px; }
            QLabel#favoritesTitle { color:#a3a3ae; font-size:12px; }
            QFrame#favoritesDivider,QFrame#operatorsDivider { background:#2c2c32; border:0; max-height:1px; }
            QLabel#rowTitle { color:#d5d5dd; font-size:14px; }
            QLabel#cardTitle { color:#91919e; font-size:12px; font-weight:500; }
            QLabel#sectionArrow { color:#8c8c98; font-size:18px; }
            QLineEdit#search { background:#202023; color:#dedee5; border:1px solid transparent; border-radius:12px; padding:7px 9px; font-size:12px; }
            QLineEdit#search:focus { border-color:%1; }
            QMenu { background:#222226; color:#bdbdc7; border:1px solid #33333b; border-radius:10px; padding:4px; font-size:13px; }
            QMenu::item { padding:5px 10px 5px 7px; border-radius:7px; background:transparent; }
            QMenu::item:selected { background:#2a2a30; color:#f4f4f7; }
            QMenu::icon { padding-left:4px; }
            QPushButton#weaponChoice { background:transparent; color:#90909c; border:1px solid transparent; border-radius:13px; padding:6px 13px; font-size:13px; }
            QPushButton#weaponChoice:hover { color:#e9e9ef; }
            QPushButton#weaponChoice[chosen="true"] { background:#29292f; color:%1; }
            QLabel#phoneAddress { font-family:'Consolas'; font-size:12px; color:#b9b9c6; }
            QListWidget { background:transparent; border:0; outline:0; }
            QFrame#settingCard { background:transparent; border:0; }
            QWidget#settingRow { background:#222225; border-radius:12px; border-left:3px solid transparent; }
            QPushButton#sectionHeading { text-align:left; padding:12px 0; border:0; background:transparent; color:#d5d5dd; font-weight:500; }
            QPushButton#sectionHeading:hover { color:white; }
            QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl { background:transparent; border:0; padding:0; }
            QPushButton#status { background:transparent; border:1px solid transparent; border-radius:13px; padding:7px 12px; color:#9696a2; font-size:12px; }
            QPushButton#status:hover { color:#e7e7ee; }
            QPushButton#status[active="true"] { color:%1; }
            QPushButton#preferencesSave,QPushButton#save { background:%1; color:#151518; border:1px solid transparent; border-radius:14px; padding:9px 16px; font-weight:500; }
            QPushButton#preferencesSave:hover,QPushButton#save:hover { color:#fff; }
            QPushButton#keyButton,QPushButton#secondary,QPushButton#cancelClose,QPushButton#discardChanges { background:#2a2a30; border:1px solid transparent; border-radius:12px; padding:6px 11px; color:#c4c4ce; font-size:13px; }
            QPushButton#keyButton:hover,QPushButton#secondary:hover,QPushButton#cancelClose:hover,QPushButton#discardChanges:hover { background:#303037; color:#f0f0f5; }
            QPushButton#keyButton:pressed,QPushButton#secondary:pressed,QPushButton#cancelClose:pressed,QPushButton#discardChanges:pressed { background:#25252b; }
            QPushButton#cancelClose[keyboardFocus="true"],QPushButton#discardChanges[keyboardFocus="true"] { border-color:#686875; }
            QPushButton#keyButton[keyboardFocus="true"],QPushButton#secondary[keyboardFocus="true"],QPushButton#weaponChoice[keyboardFocus="true"],QPushButton#status[keyboardFocus="true"],QPushButton#save[keyboardFocus="true"],QPushButton#preferencesSave[keyboardFocus="true"] { border-color:#686875; }
            QDoubleSpinBox,QSpinBox { background:#2a2a2f; border:1px solid transparent; border-radius:10px; padding:5px; color:#e1e1e9; font-size:13px; }
            QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; }
            QDoubleSpinBox:disabled,QSpinBox:disabled { color:#6e6e7a; background:#252529; }
            QSlider::groove:horizontal { height:6px; background:#35353d; border-radius:3px; }
            QSlider::sub-page:horizontal { background:%1; border-radius:3px; }
            QSlider::handle:horizontal { width:14px; margin:-4px 0; background:#ececf1; border:0; border-radius:7px; }
            QSlider::handle:horizontal:hover { background:white; }
            QComboBox { background:#2a2a30; color:#d3d3dc; border:1px solid transparent; border-radius:12px; padding:6px 10px; min-width:120px; font-size:13px; }
            QComboBox:focus { border-color:%1; }
            QComboBox::drop-down { border:0; width:22px; }
            QComboBox::down-arrow { image:url(:/ui/chevron.svg); width:12px; height:12px; }
            QComboBox QAbstractItemView { background:#25252b; selection-background-color:#35353e; border:0; padding:4px; }
            QScrollArea { background:transparent; border:0; }
            QScrollArea>QWidget>QWidget { background:transparent; }
            QScrollBar:vertical { background:transparent; width:6px; margin:3px 0; }
            QScrollBar::handle:vertical { background:#3d3d47; min-height:36px; border-radius:3px; }
            QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
            QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }
            QDialog { background:#1c1c1f; }
            QToolTip { background:#29292f; color:#e1e1e9; border:0; padding:6px 8px; font-size:12px; }
            QPushButton:focus { outline:0; }
        )CSS").arg(accent.name());
}
