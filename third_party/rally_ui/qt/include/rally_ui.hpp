#pragma once
#include <QtWidgets>
#include <functional>
#include <algorithm>
#include <cmath>

// Rally UI 0.1.0 - a flat, compact interface kit. No app-specific state.
namespace rally {
inline const QColor canvas("#171C19"),panel("#242B25"),well("#101512"),line("#414A40"),text("#EEEADE"),muted("#A5AD9E"),signal("#F16B47"),danger("#FF967F");
inline QColor onColor(const QColor& c){const auto linear=[](qreal v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};const auto luminance=[&](const QColor& v){return .2126*linear(v.redF())+.7152*linear(v.greenF())+.0722*linear(v.blueF());};const auto y=luminance(c),dark=luminance(canvas);const auto darkContrast=(std::max(y,dark)+.05)/(std::min(y,dark)+.05);return darkContrast>=1.05/(y+.05)?canvas:QColor("#FFFFFF");}
inline QColor blend(const QColor& a,const QColor& b,qreal t){return QColor::fromRgbF(a.redF()+(b.redF()-a.redF())*t,a.greenF()+(b.greenF()-a.greenF())*t,a.blueF()+(b.blueF()-a.blueF())*t);}
inline QPalette palette(const QColor& accent=signal){QPalette p;p.setColor(QPalette::Window,canvas);p.setColor(QPalette::WindowText,text);p.setColor(QPalette::Base,well);p.setColor(QPalette::AlternateBase,panel);p.setColor(QPalette::Text,text);p.setColor(QPalette::Button,panel);p.setColor(QPalette::ButtonText,text);p.setColor(QPalette::Highlight,accent);p.setColor(QPalette::HighlightedText,canvas);p.setColor(QPalette::PlaceholderText,muted);p.setColor(QPalette::Disabled,QPalette::Text,QColor("#747F70"));return p;}
inline QString style(const QColor& accent=signal){return QString::fromUtf8(R"RALLYCSS(

QWidget { color:#EEEADE; font-family:'Geist'; font-size:13px; font-weight:400; }
QWidget#mainWindow { background:#171C19; }
QFrame#titleBar { background:#101512; border:0; border-bottom:1px solid #414A40; }
QLabel#pageTitle,QLabel#preferencesTitle { font-size:34px; font-weight:600; color:#EEEADE; }
QLabel#subtitle,QLabel#hint,QLabel#rowHint { color:#A5AD9E; font-size:12px; }
QLabel#rowTitle { font-size:13px; font-weight:500; }
QLabel#panelTitle,QLabel#cardTitle { font-size:19px; font-weight:600; }
QFrame#favoritesDivider,QFrame#operatorsDivider { background:#414A40; border:0; max-height:1px; }
QFrame#settingCard { background:#242B25; border:1px solid #414A40; border-radius:6px; }
QWidget#settingRow { background:transparent; border:0; }
QWidget#forceField { background:#1B211C; border:0; border-radius:4px; }
QLineEdit { background:#101512; color:#EEEADE; border:1px solid #414A40; border-radius:4px; padding:9px 12px; selection-background-color:%1; selection-color:%2; }
QLineEdit:focus { border:2px solid %1; padding:8px 11px; }
QMenu { background:#242B25; color:#EEEADE; border:1px solid #414A40; border-radius:4px; padding:4px; font-size:13px; }
QMenu::item { padding:6px 10px 6px 7px; border-radius:2px; background:transparent; }
QMenu::item:selected { background:#414A40; color:#EEEADE; }
QMenu::icon { padding-left:4px; }
QMenu::separator { height:1px; background:#414A40; margin:4px 6px; }
QListWidget { background:transparent; border:0; outline:0; }
QPushButton { min-height:20px; }
QPushButton#pairingQr { min-width:184px; max-width:184px; min-height:208px; max-height:208px; border:0; padding:0; }
QPushButton#sectionHeading { text-align:left; padding:6px 0; border:0; background:transparent; color:#A5AD9E; font-size:12px; font-weight:600; }
QPushButton#windowControl,QPushButton#saveControl,QPushButton#closeControl { background:transparent; border:0; padding:0; }
QPushButton#status { background:#242B25; border:1px solid #414A40; border-radius:3px; padding:5px 12px; color:#A5AD9E; font-size:12px; font-weight:500; }
QPushButton#status[active="true"] { background:%1; border-color:%1; color:%2; }
QPushButton#preferencesSave,QPushButton#save { background:%1; color:%2; border:1px solid %1; border-radius:4px; padding:8px 16px; font-weight:600; }
QPushButton#preferencesSave:disabled,QPushButton#save:disabled { background:#242B25; color:#A5AD9E; }
QPushButton#keyButton,QPushButton#secondary,QPushButton#operatorPicker,QPushButton#cancelClose,QPushButton#discardChanges { background:#242B25; border:1px solid #414A40; border-radius:4px; padding:6px 12px; color:#EEEADE; font-size:13px; font-weight:500; }
QPushButton#keyButton:pressed,QPushButton#secondary:pressed,QPushButton#operatorPicker:pressed,QPushButton#cancelClose:pressed,QPushButton#discardChanges:pressed { background:#101512; }
QPushButton[keyboardFocus="true"] { border:2px solid %1; }
QDoubleSpinBox,QSpinBox { background:#101512; border:1px solid #414A40; border-radius:4px; padding:6px 8px; color:#EEEADE; font-family:'Consolas'; font-weight:500; font-size:16px; selection-background-color:%1; selection-color:%2; }
QDoubleSpinBox:focus,QSpinBox:focus { border:2px solid %1; padding:5px 7px; }
QDoubleSpinBox#forceValue { background:transparent; border:0; border-radius:2px; padding:0; font-size:42px; font-weight:400; }
QDoubleSpinBox#forceValue:focus { border-bottom:2px solid %1; }
QDoubleSpinBox:disabled,QSpinBox:disabled { color:#747F70; }
QSlider { min-height:26px; }
QSlider::groove:horizontal { height:5px; background:#414A40; border-radius:1px; }
QSlider::sub-page:horizontal { background:%1; border-radius:1px; }
QSlider::handle:horizontal { width:10px; margin:-8px 0; border:2px solid #242B25; border-radius:2px; background:#EEEADE; }
QSlider:focus::handle:horizontal { background:%1; }
QComboBox { background:#101512; color:#EEEADE; border:1px solid #414A40; border-radius:4px; padding:7px 10px; min-width:155px; font-size:13px; }
QComboBox:focus { border:2px solid %1; padding:6px 9px; }
QComboBox::drop-down { border:0; width:25px; }
QComboBox::down-arrow { image:url(:/ui/chevron.svg); width:12px; height:12px; }
QComboBox QAbstractItemView { background:#242B25; color:#EEEADE; border:1px solid #414A40; border-radius:0; padding:4px; selection-background-color:#414A40; selection-color:#EEEADE; outline:0; }
QComboBox QAbstractItemView::item { min-height:28px; padding:2px 6px; }
QScrollArea { background:transparent; border:0; }
QScrollArea>QWidget>QWidget { background:transparent; }
QScrollBar:vertical { background:transparent; width:8px; margin:2px 0; }
QScrollBar::handle:vertical { background:#747F70; min-height:36px; border-radius:2px; margin:0 2px; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }
QDialog { background:#171C19; }
QDialog#palestinePopup { background:#171C19; border:1px solid #414A40; border-radius:8px; }
QLabel#palestineArabic { color:#EEEADE; font-size:24px; font-weight:900; }
QToolTip { background:#242B25; color:#EEEADE; border:1px solid #414A40; border-radius:3px; padding:6px 8px; font-size:12px; }
QPushButton#secondary:disabled { color:#747F70; border-color:#414A40; }
QPushButton:focus { outline:0; }

)RALLYCSS").arg(accent.name(),onColor(accent).name());}
inline void apply(QApplication& app,const QColor& accent=signal){app.setPalette(palette(accent));app.setStyleSheet(style(accent));QFont font("Geist");font.setPixelSize(13);app.setFont(font);}

class Switch : public QAbstractButton {
protected:
    QVariantAnimation motion_;qreal position_=0;std::function<QColor()> accent_;
public:
    explicit Switch(std::function<QColor()> accent=[] {return signal;},QWidget* parent=nullptr):QAbstractButton(parent),accent_(std::move(accent)){
        setCheckable(true);setFixedSize(46,28);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);motion_.setDuration(160);motion_.setEasingCurve(QEasingCurve::OutCubic);
        connect(&motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& v){position_=v.toReal();update();});
        connect(this,&QAbstractButton::toggled,this,[this](bool on){motion_.stop();if(!isVisible()){position_=on?1:0;update();return;}motion_.setStartValue(position_);motion_.setEndValue(on?1.:0.);motion_.start();});
    }
    void syncChecked(bool on){if(on==isChecked())return;QSignalBlocker blocked(this);motion_.stop();position_=on?1:0;setChecked(on);update();}
protected:
    void hideEvent(QHideEvent* e) override{motion_.stop();position_=isChecked()?1:0;QAbstractButton::hideEvent(e);}
    void paintEvent(QPaintEvent*) override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);if(!isEnabled())p.setOpacity(.4);const auto a=accent_();const auto t=std::clamp(motion_.state()==QAbstractAnimation::Running?position_:(isChecked()?1.:0.),0.,1.);
        p.setPen(QPen(blend(line,a,t),1));p.setBrush(blend(well,a,t));p.drawRoundedRect(QRectF(1,4,44,20),3,3);
        p.setPen(Qt::NoPen);p.setBrush(blend(muted,onColor(a),t));p.drawRoundedRect(QRectF(4+24*t,7,14,14),1,1);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setBrush(Qt::NoBrush);p.setPen(QPen(a,2));p.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),4,4);}
    }
};
class Segment : public QPushButton {
protected:
    QVariantAnimation motion_;qreal amount_=0;bool chosen_=false,initialized_=false;std::function<QColor()> accent_;
public:
    explicit Segment(std::function<QColor()> accent=[] {return signal;},QWidget* parent=nullptr):QPushButton(parent),accent_(std::move(accent)){setObjectName("weaponChoice");setMinimumHeight(36);setCursor(Qt::PointingHandCursor);motion_.setDuration(160);motion_.setEasingCurve(QEasingCurve::OutCubic);connect(&motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& v){amount_=v.toReal();update();});}
    void setChosen(bool on){if(initialized_&&chosen_==on)return;const bool animate=initialized_&&isVisible();initialized_=true;chosen_=on;setProperty("chosen",on);setAccessibleDescription(on?"Selected":"");motion_.stop();if(!animate){amount_=on?1:0;update();return;}motion_.setStartValue(amount_);motion_.setEndValue(on?1.:0.);motion_.start();}
protected:
    void hideEvent(QHideEvent* e) override{motion_.stop();amount_=chosen_?1:0;QPushButton::hideEvent(e);}
    void paintEvent(QPaintEvent*) override{QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const auto t=std::clamp(amount_,0.,1.);const auto a=accent_();const QRectF r=QRectF(rect()).adjusted(1,1,-1,-1);p.setBrush(blend(isDown()?canvas:well,a,t));p.setPen(QPen(blend(line,a,t),1));p.drawRoundedRect(r,3,3);p.setFont(font());p.setPen(blend(muted,onColor(a),t));p.drawText(rect(),Qt::AlignCenter,text());if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setBrush(Qt::NoBrush);p.setPen(QPen(a,2));p.drawRoundedRect(r,3,3);}}
};
class Panel : public QFrame {public:explicit Panel(QWidget* parent=nullptr):QFrame(parent){setObjectName("settingCard");}};
class Caption : public QLabel {
public:
    Caption(const QString& title,const QColor& =signal,QWidget* parent=nullptr):QLabel(title,parent){setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);setObjectName("rallyCaption");}
    static QFont captionFont(){QFont f("Geist");f.setPixelSize(10);f.setWeight(QFont::DemiBold);f.setLetterSpacing(QFont::AbsoluteSpacing,1.3);return f;}
    QSize sizeHint() const override{return {QFontMetrics(captionFont()).horizontalAdvance(text().toUpper())+20,22};}
    QSize minimumSizeHint() const override{return {std::min(100,sizeHint().width()),22};}
protected:
    void paintEvent(QPaintEvent*) override{QPainter p(this);p.setPen(Qt::NoPen);p.setBrush(signal);p.drawRect(0,height()/2-3,5,5);p.setFont(captionFont());p.setPen(muted);p.drawText(rect().adjusted(15,0,0,0),Qt::AlignVCenter,QFontMetrics(captionFont()).elidedText(text().toUpper(),Qt::ElideRight,width()-15));}
};
class Slider : public QSlider {
    void seek(qreal x){QStyleOptionSlider o;initStyleOption(&o);setValue(QStyle::sliderValueFromPosition(minimum(),maximum(),qRound(x)-7,std::max(1,width()-14),o.upsideDown));}
public:
    explicit Slider(QWidget* parent=nullptr):QSlider(Qt::Horizontal,parent){setMinimumHeight(26);setFocusPolicy(Qt::StrongFocus);setCursor(Qt::PointingHandCursor);}
protected:
    void mousePressEvent(QMouseEvent* e) override{if(e->button()!=Qt::LeftButton){QSlider::mousePressEvent(e);return;}setFocus(Qt::MouseFocusReason);setSliderDown(true);seek(e->position().x());e->accept();}
    void mouseMoveEvent(QMouseEvent* e) override{if(isSliderDown()){seek(e->position().x());e->accept();}else QSlider::mouseMoveEvent(e);}
    void mouseReleaseEvent(QMouseEvent* e) override{if(e->button()==Qt::LeftButton&&isSliderDown()){seek(e->position().x());setSliderDown(false);e->accept();}else QSlider::mouseReleaseEvent(e);}
    void paintEvent(QPaintEvent*) override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);if(!isEnabled())p.setOpacity(.4);QStyleOptionSlider o;initStyleOption(&o);const int travel=std::max(1,width()-14);const qreal x=7+QStyle::sliderPositionFromValue(minimum(),maximum(),value(),travel,o.upsideDown);const qreal y=height()/2.;const auto a=palette().color(QPalette::Highlight);
        p.setPen(Qt::NoPen);p.setBrush(line);p.drawRoundedRect(QRectF(7,y-2,width()-14,4),1,1);p.setBrush(a);const qreal start=o.upsideDown?x:7;const qreal end=o.upsideDown?width()-7:x;p.drawRoundedRect(QRectF(start,y-2,std::max(0.,end-start),4),1,1);
        p.setPen(QPen(panel,2));p.setBrush(rally::text);p.drawRoundedRect(QRectF(x-6,y-10,12,20),2,2);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setPen(QPen(a,2));p.setBrush(Qt::NoBrush);p.drawRoundedRect(QRectF(x-7,y-12,14,24),3,3);}
    }
};
}
