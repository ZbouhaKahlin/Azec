#pragma once
#include <QtWidgets>
#include <QSvgRenderer>
#include "theme.hpp"
#include "mono_controls.hpp"
#include <functional>
#include <algorithm>
#include <cmath>

inline int operatorRowHeight=52;
inline constexpr int operatorIconSize=44;
// Shared raised-panel geometry: the page stack fills this surface exactly, so
// transitions must use the same radius and hairline when clipping snapshots.
inline constexpr qreal raisedPanelRadius=32;
inline constexpr qreal raisedPanelBorder=1;

// Monocode's signature motion: a long, settled ease-out on everything.
inline QEasingCurve monoEase(){
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(0.22,1.0),QPointF(0.36,1.0),QPointF(1.0,1.0));
    return curve;
}

// Double-clicking a slider returns it to the profile default for its control.
class ResetSlider final : public QSlider {
    int reset_=0;
public:
    explicit ResetSlider(Qt::Orientation orientation=Qt::Horizontal,QWidget* parent=nullptr):QSlider(orientation,parent){}
    void setResetValue(int value){reset_=value;}
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override{
        if(event->button()==Qt::LeftButton){setValue(reset_);event->accept();return;}
        QSlider::mouseDoubleClickEvent(event);
    }
};

// A selectable command view that can wrap inside a path segment. QLabel's
// ordinary word wrap either reserves the longest token or clips that token.
class WrappingCommandText final : public QTextEdit {
public:
    explicit WrappingCommandText(QWidget* parent=nullptr):QTextEdit(parent){
        setObjectName("codeText");setReadOnly(true);setAcceptRichText(false);
        setFrameShape(QFrame::NoFrame);document()->setDocumentMargin(0);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setLineWrapMode(QTextEdit::WidgetWidth);setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        auto policy=QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);policy.setHeightForWidth(true);setSizePolicy(policy);
    }
    bool hasHeightForWidth() const override{return true;}
    int heightForWidth(int width) const override{
        QTextDocument measurement;measurement.setDefaultFont(font());measurement.setDocumentMargin(0);
        measurement.setDefaultTextOption(document()->defaultTextOption());measurement.setPlainText(toPlainText());
        measurement.setTextWidth(qMax(1,width));return qCeil(measurement.size().height());
    }
    QSize minimumSizeHint() const override{return {0,fontMetrics().height()};}
    QSize sizeHint() const override{return {240,heightForWidth(240)};}
};

// Monospace command box with its own copy action, used by setup guides.
class CodeBlock final : public QWidget {
    WrappingCommandText* code_;
    QPushButton* copy_;
    QString copyLabel_,copiedLabel_;
    std::function<void()> copyFeedback_;
    bool sized_=false;
public:
    explicit CodeBlock(const QString& copy,const QString& copied,QWidget* parent=nullptr):QWidget(parent){
        setObjectName("codeBlock");
        copyLabel_=copy;copiedLabel_=copied;
        auto* layout=new QHBoxLayout(this);layout->setContentsMargins(12,9,10,9);layout->setSpacing(10);
        code_=new WrappingCommandText;
        QFont font(QStringLiteral("Consolas"));font.setStyleHint(QFont::Monospace);font.setPixelSize(12);
        code_->setFont(font);
        layout->addWidget(code_,1);
        copy_=new MonoButton(copyLabel_);copy_->setObjectName("secondary");
        copy_->setAccessibleName(copyLabel_);
        layout->addWidget(copy_,0,Qt::AlignTop);
        connect(copy_,&QPushButton::clicked,this,[this]{
            QApplication::clipboard()->setText(code_->toPlainText());
            if(copyFeedback_)copyFeedback_();
            copy_->setText(copiedLabel_);
            QTimer::singleShot(1400,copy_,[this]{if(copy_)copy_->setText(copyLabel_);});
        });
    }
    void setText(const QString& text){code_->setPlainText(text);code_->updateGeometry();updateGeometry();}
    void setCopyFeedback(std::function<void()> feedback){copyFeedback_=std::move(feedback);}
    QString text() const{return code_->toPlainText();}
protected:
    // Reserve the wider of the two labels once the themed font is applied, so
    // swapping to "Copied" cannot shrink the button and flash the layout.
    void showEvent(QShowEvent* event) override{
        QWidget::showEvent(event);
        if(sized_)return;
        sized_=true;
        const auto metrics=copy_->fontMetrics();
        const int width=qMax(metrics.horizontalAdvance(copyLabel_),metrics.horizontalAdvance(copiedLabel_));
        copy_->setFixedWidth(width+20);
    }
};

class InkPopup final : public QFrame {
public:
    explicit InkPopup(QWidget* parent):QFrame(parent,Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint){setAttribute(Qt::WA_TranslucentBackground);}
protected:
    void paintEvent(QPaintEvent*) override{

        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        const QRectF face(4,4,width()-12,height()-12);
        // A soft two-step shadow, then the panel: dark fill, one hairline.
        painter.setPen(Qt::NoPen);painter.setBrush(QColor(0,0,0,36));painter.drawRoundedRect(face.adjusted(2,3,-2,3),12,12);
        painter.setBrush(QColor(0,0,0,52));painter.drawRoundedRect(face.adjusted(1,1,-1,2),11,11);
        painter.setBrush(kbtheme::surface);painter.drawRoundedRect(face,10,10);
        painter.setBrush(Qt::NoBrush);painter.setPen(QPen(kbtheme::outline,1));painter.drawRoundedRect(face.adjusted(.5,.5,-.5,-.5),10,10);
    }
};

// Keep the combobox model/API, but draw its popup ourselves on every platform.
class LanguageComboBox final : public QComboBox {
    InkPopup* popup_;
    QListWidget* choices_;
    void choose(){const int index=choices_->currentRow();hidePopup();if(index>=0){setCurrentIndex(index);emit activated(index);}}
    void styleChoices(){choices_->setStyleSheet(QString("QListWidget{background:transparent;border:0;padding:0;margin:0;outline:0;color:%1;} QListWidget::item{height:36px;padding:0 8px;border-radius:6px;background:transparent;} QListWidget::item:selected{background:%2;color:%1;} QListWidget::item:hover:!selected{background:transparent;}").arg(kbtheme::ink.name(),kbtheme::css(kbtheme::lavender)));}
public:
    explicit LanguageComboBox(QWidget* parent=nullptr,const QString& popupName=QStringLiteral("languagePopup")):QComboBox(parent),popup_(new InkPopup(this)),choices_(new QListWidget(popup_)){
        popup_->setObjectName(popupName);auto* layout=new QVBoxLayout(popup_);layout->setContentsMargins(12,12,14,15);layout->addWidget(choices_);
        choices_->setIconSize(QSize(22,15));choices_->setMouseTracking(false);choices_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        styleChoices();

        choices_->installEventFilter(this);connect(choices_,&QListWidget::clicked,this,[this]{choose();});
    }
    void showPopup() override{
        choices_->setIconSize(iconSize());
        styleChoices();
        choices_->clear();for(int i=0;i<count();++i)new QListWidgetItem(itemIcon(i),itemText(i),choices_);
        choices_->setCurrentRow(currentIndex());const auto screen=QGuiApplication::screenAt(mapToGlobal(rect().center()));const QRect bounds=(screen?screen:QGuiApplication::primaryScreen())->availableGeometry();
        // QSS padding and font metrics can make rows taller than 36 px.
        // Measure the actual contents so the two-theme menu cannot clip Dark
        // above the selected Light row. Only screen-limited menus may scroll.
        choices_->ensurePolished();choices_->doItemsLayout();
        const auto margins=popup_->layout()->contentsMargins();
        int rowsHeight=0;for(int i=0;i<count();++i)rowsHeight+=qMax(36,choices_->sizeHintForRow(i));
        const auto listMargins=choices_->contentsMargins();
        const int chrome=listMargins.top()+listMargins.bottom();
        popup_->resize(qMin(bounds.width(),qMax(width(),choices_->sizeHintForColumn(0)+40)),qMin(bounds.height()-16,rowsHeight+chrome+margins.top()+margins.bottom()));
        popup_->layout()->activate();choices_->doItemsLayout();choices_->verticalScrollBar()->setValue(0);
        QPoint position=mapToGlobal(QPoint(0,height()+4));if(position.y()+popup_->height()>bounds.bottom())position.setY(mapToGlobal(QPoint()).y()-popup_->height()-4);
        position.setX(qBound(bounds.left(),position.x(),bounds.right()-popup_->width()+1));position.setY(qMax(bounds.top(),position.y()));popup_->move(position);popup_->show();choices_->setFocus(Qt::PopupFocusReason);choices_->scrollToItem(choices_->currentItem());
    }
    void hidePopup() override{popup_->hide();QComboBox::hidePopup();}
protected:
    void paintEvent(QPaintEvent* event) override{
        QComboBox::paintEvent(event);
        // The stylesheet suppresses Qt's native arrow; draw one open chevron.
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setBrush(Qt::NoBrush);p.setPen(QPen(kbtheme::muted,1.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        const int x=layoutDirection()==Qt::RightToLeft?16:width()-16,y=height()/2;const QPolygon chevron{{x-4,y-2},{x,y+2},{x+4,y-2}};p.drawPolyline(chevron);
    }
    bool eventFilter(QObject* watched,QEvent* event) override{
        if(watched==choices_&&event->type()==QEvent::KeyPress){const int key=static_cast<QKeyEvent*>(event)->key();if(key==Qt::Key_Return||key==Qt::Key_Enter||key==Qt::Key_Space){choose();return true;}if(key==Qt::Key_Escape||key==Qt::Key_Tab){hidePopup();setFocus(Qt::PopupFocusReason);return true;}}
        return QComboBox::eventFilter(watched,event);
    }
};

inline QColor mixColor(const QColor& from,const QColor& to,qreal amount){
    return QColor::fromRgbF(from.redF()+(to.redF()-from.redF())*amount,from.greenF()+(to.greenF()-from.greenF())*amount,from.blueF()+(to.blueF()-from.blueF())*amount,from.alphaF()+(to.alphaF()-from.alphaF())*amount);
}

// Changing QVariantAnimation's duration/endpoints can emit valueChanged even
// while stopped. Suppress those setup values so they cannot resize the page or
// overwrite the current position before a reversal starts.
inline void retargetAnimation(QVariantAnimation& animation,qreal from,qreal to,int duration){
    {QSignalBlocker blocked(&animation);animation.stop();animation.setDuration(duration);animation.setStartValue(from);animation.setEndValue(to);animation.setCurrentTime(0);}
    animation.start();
}

// The hit target stays stationary; only the vector ink shows save state and focus.
class IconButton final : public QPushButton {
    QSvgRenderer renderer_;
    std::function<QColor()> accent_;
    QPixmap mask_;
    qreal maskRatio_=0;
    QVariantAnimation hoverMotion_;
    qreal hover_=0;
public:
    IconButton(const QString& path,std::function<QColor()> accent,QWidget* parent=nullptr):QPushButton(parent),renderer_(path),accent_(std::move(accent)){
        setIcon(QIcon(path));setIconSize(QSize(20,20));setFixedSize(34,34);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);
        hoverMotion_.setEasingCurve(monoEase());
        connect(&hoverMotion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){hover_=value.toReal();update();});
    }
protected:
    void enterEvent(QEnterEvent* event) override{QPushButton::enterEvent(event);if(property("sidebarAction").toBool())retargetAnimation(hoverMotion_,hover_,1,120);}
    void leaveEvent(QEvent* event) override{QPushButton::leaveEvent(event);if(property("sidebarAction").toBool())retargetAnimation(hoverMotion_,hover_,0,120);}
    void hideEvent(QHideEvent* event) override{hoverMotion_.stop();hover_=0;QPushButton::hideEvent(event);}
    void paintEvent(QPaintEvent*) override {
        const qreal ratio=devicePixelRatioF();
        if(mask_.isNull()||maskRatio_!=ratio){maskRatio_=ratio;mask_=QPixmap(QSize(20,20)*ratio);mask_.setDevicePixelRatio(ratio);mask_.fill(Qt::transparent);QPainter maskPainter(&mask_);renderer_.render(&maskPainter,QRectF(0,0,20,20));}
        const auto state=property("saveState").toString();
        const bool unsaved=state=="dirty"||property("unsaved").toBool();
        QColor color=unsaved||property("activePage").toBool()?accent_():state=="error"?kbtheme::danger:kbtheme::muted;
        if(isDown())color=mixColor(color,kbtheme::ink,.25);
        QPixmap ink=mask_;QPainter tint(&ink);tint.setCompositionMode(QPainter::CompositionMode_SourceIn);tint.fillRect(ink.rect(),color);tint.end();
        QPainter painter(this);if(!isEnabled())painter.setOpacity(.35);
        const bool sidebarAction=property("sidebarAction").toBool();
        if(sidebarAction){painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);painter.setBrush(kbtheme::content(.05*hover_));painter.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),8,8);}
        const bool rtl=layoutDirection()==Qt::RightToLeft;
        painter.drawPixmap(QPointF(sidebarAction?(rtl?width()-42:22):(width()-20)/2.0,(height()-20)/2.0),ink);
        if(sidebarAction&&!text().isEmpty()){
            painter.setOpacity(std::clamp((width()-64.0)/124.0,0.0,1.0));painter.setPen(color);
            painter.drawText(rtl?QRect(0,0,width()-64,height()):QRect(64,0,width()-64,height()),Qt::AlignAbsolute|Qt::AlignVCenter|(rtl?Qt::AlignRight:Qt::AlignLeft),text());painter.setOpacity(1);
        }
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setBrush(Qt::NoBrush);painter.setPen(QPen(accent_(),1.5));painter.drawRoundedRect(QRectF(rect()).adjusted(2,2,-2,-2),8,8);}
    }
};

// Quiet monochrome section icons: a plain stroke, accent while selected.
enum class SettingsGlyph{General,Shortcuts,Phone,Profile};

inline QPainterPath settingsGlyphShape(SettingsGlyph glyph){
    QPainterPath shape;
    switch(glyph){
    case SettingsGlyph::General:
        shape.moveTo(-9,-6);shape.lineTo(-5,-6);shape.moveTo(1,-6);shape.lineTo(9,-6);
        shape.moveTo(-9,0);shape.lineTo(1,0);shape.moveTo(7,0);shape.lineTo(9,0);
        shape.moveTo(-9,6);shape.lineTo(-3,6);shape.moveTo(3,6);shape.lineTo(9,6);
        break;
    case SettingsGlyph::Shortcuts:shape.addRoundedRect(QRectF(-9.5,-6.5,19,13),2.5,2.5);break;
    case SettingsGlyph::Phone:shape.addRoundedRect(QRectF(-6,-10,12,20),2.5,2.5);break;
    case SettingsGlyph::Profile:
        shape.addRoundedRect(QRectF(-9,-8,18,5),1.5,1.5);
        shape.moveTo(-7,-3);shape.lineTo(-7,8);shape.lineTo(7,8);shape.lineTo(7,-3);
        break;
    }
    return shape;
}

inline void paintSettingsGlyphDetails(QPainter& painter,SettingsGlyph glyph,const QColor& stroke){
    painter.setPen(QPen(stroke,1.65,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    switch(glyph){
    case SettingsGlyph::General:
        painter.drawEllipse(QPointF(-2,-6),2.5,2.5);
        painter.drawEllipse(QPointF(4,0),2.5,2.5);
        painter.drawEllipse(QPointF(0,6),2.5,2.5);
        break;
    case SettingsGlyph::Shortcuts:
        for(int row=0;row<2;++row)for(int column=0;column<4;++column)
            painter.drawPoint(QPointF(-6+column*4,-3+row*3));
        painter.drawLine(QPointF(-4,3.5),QPointF(4,3.5));
        break;
    case SettingsGlyph::Phone:
        painter.drawLine(QPointF(-2,-7),QPointF(2,-7));
        painter.drawLine(QPointF(-1,7),QPointF(1,7));
        break;
    case SettingsGlyph::Profile:
        painter.drawLine(QPointF(-2,1),QPointF(2,1));
        break;
    }
}

inline void paintSettingsGlyph(QPainter& painter,const QRectF& box,SettingsGlyph glyph,bool active){
    const QColor stroke=active?kbtheme::ink:kbtheme::content(.5);
    const QPainterPath shape=settingsGlyphShape(glyph);
    const qreal scale=box.width()/24.0;
    painter.save();painter.translate(box.topLeft());painter.scale(scale,scale);painter.translate(12,12);
    painter.setPen(QPen(stroke,1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.setBrush(Qt::NoBrush);painter.drawPath(shape);
    paintSettingsGlyphDetails(painter,glyph,stroke);
    painter.restore();
}

inline QPainterPath favoriteStarShape(){
    constexpr qreal pi=3.14159265358979323846;QPainterPath path;
    for(int point=0;point<10;++point){
        const qreal radius=point%2?4.8:10.4,angle=-pi/2+point*pi/5;
        const QPointF position(std::cos(angle)*radius,std::sin(angle)*radius);
        if(point==0)path.moveTo(position);else path.lineTo(position);
    }
    path.closeSubpath();return path;
}

class FavoriteStarButton final : public QAbstractButton {
public:
    explicit FavoriteStarButton(QWidget* parent=nullptr):QAbstractButton(parent){
        setFixedSize(34,34);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);
    }
protected:
    void paintEvent(QPaintEvent*) override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        const QPainterPath shape=favoriteStarShape();
        painter.save();painter.translate((width()-26)/2.0,(height()-26)/2.0);painter.scale(26.0/24.0,26.0/24.0);painter.translate(12,12);
        painter.setPen(QPen(isDown()?kbtheme::ink:kbtheme::muted,1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.setBrush(Qt::NoBrush);painter.drawPath(shape);
        painter.restore();
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setBrush(Qt::NoBrush);painter.setPen(QPen(kbtheme::defaultAccent,1.5));painter.drawRoundedRect(QRectF(rect()).adjusted(2,2,-2,-2),8,8);}
    }
};

class SettingsNavButton final : public QAbstractButton {
    SettingsGlyph glyph_;QString description_;
    QVariantAnimation hoverMotion_;
    qreal hover_=0;
    void hoverTo(qreal target){retargetAnimation(hoverMotion_,hover_,target,120);}
public:
    explicit SettingsNavButton(SettingsGlyph glyph,QWidget* parent=nullptr):QAbstractButton(parent),glyph_(glyph){
        setCheckable(true);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);setFixedHeight(62);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setMinimumWidth(0);
        hoverMotion_.setEasingCurve(monoEase());
        connect(&hoverMotion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){hover_=value.toReal();update();});
    }
    SettingsGlyph glyph() const{return glyph_;}
    void setDescription(const QString& description){description_=description;update();}
protected:
    void enterEvent(QEnterEvent* event) override{QAbstractButton::enterEvent(event);hoverTo(1);}
    void leaveEvent(QEvent* event) override{QAbstractButton::leaveEvent(event);hoverTo(0);}
    void hideEvent(QHideEvent* event) override{hoverMotion_.stop();hover_=0;QAbstractButton::hideEvent(event);}
    void paintEvent(QPaintEvent*) override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        const QRectF pill=QRectF(rect()).adjusted(1,2,-1,-2);
        const QColor fill=isChecked()?kbtheme::lavender:kbtheme::content(.05*hover_);
        painter.setPen(Qt::NoPen);painter.setBrush(fill);painter.drawRoundedRect(pill,8,8);
        const qreal progress=std::clamp((width()-64.0)/124.0,0.0,1.0);
        const int art=22;const qreal compactX=(width()-art)/2.0,iconX=compactX+(16-compactX)*progress;
        const bool rtl=QApplication::layoutDirection()==Qt::RightToLeft;
        const QRectF iconRect(rtl?width()-iconX-art:iconX,(height()-art)/2.0,art,art);
        paintSettingsGlyph(painter,iconRect,glyph_,isChecked());
        if(progress>0.01){
            QFont nameFont=font();nameFont.setPixelSize(13);nameFont.setWeight(QFont::Medium);
            QFont hintFont=font();hintFont.setPixelSize(11);hintFont.setWeight(QFont::Normal);
            const qreal left=rtl?8:iconRect.right()+12,available=rtl?iconRect.left()-16:width()-iconRect.right()-20;
            const qreal textTop=(height()-38)/2.0;
            const QRectF nameRect(left,textTop,available,20),hintRect(left,textTop+20,available,18);
            const Qt::Alignment alignment=Qt::AlignAbsolute|Qt::AlignVCenter|(rtl?Qt::AlignRight:Qt::AlignLeft);
            painter.setOpacity(progress);
            painter.setFont(nameFont);painter.setPen(kbtheme::ink);
            painter.drawText(nameRect,alignment,QFontMetrics(nameFont).elidedText(text(),Qt::ElideRight,qMax(0,qRound(available))));
            painter.setFont(hintFont);painter.setPen(kbtheme::muted);
            painter.drawText(hintRect,alignment,QFontMetrics(hintFont).elidedText(description_,Qt::ElideRight,qMax(0,qRound(available))));
            painter.setOpacity(1);
        }
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setBrush(Qt::NoBrush);painter.setPen(QPen(kbtheme::defaultAccent,1.5));painter.drawRoundedRect(QRectF(rect()).adjusted(2,2,-2,-2),10,10);}
    }
};

// Monocode switch: a quiet pill whose white knob travels between a 20% well
// and the accent fill. No faces, no outlines — the state is the fill itself.
class Toggle final : public QAbstractButton {
    QVariantAnimation motion_;
    qreal position_=0;
    std::function<QColor()> accent_;
public:
    explicit Toggle(std::function<QColor()> accent,QWidget* parent=nullptr):QAbstractButton(parent),accent_(std::move(accent)){
        setCheckable(true);setFixedSize(48,28);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);
        motion_.setEasingCurve(monoEase());
        connect(&motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){position_=value.toReal();update();});
        connect(this,&QAbstractButton::toggled,this,[this](bool checked){
            if(!isVisible()){motion_.stop();position_=checked?1:0;update();return;}
            retargetAnimation(motion_,position_,checked?1:0,160);
        });
    }
    void syncChecked(bool checked){
        if(checked==isChecked())return;
        QSignalBlocker block(this);motion_.stop();position_=checked?1:0;setChecked(checked);update();
    }
protected:
    void hideEvent(QHideEvent* event) override{motion_.stop();position_=isChecked()?1:0;QAbstractButton::hideEvent(event);}
    void paintEvent(QPaintEvent*) override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const qreal position=motion_.state()==QAbstractAnimation::Running?position_:(isChecked()?1.0:0.0);
        const qreal clamped=std::clamp(position,0.0,1.0);
        if(!isEnabled())p.setOpacity(.4);
        const QColor accent=accent_();
        const QRectF track=QRectF(6,4,36,20);
        // A 20% content well when off; the accent fill fades in when on.
        const QColor fill=mixColor(kbtheme::content(.2),accent,clamped);
        p.setPen(Qt::NoPen);p.setBrush(fill);p.drawRoundedRect(track,10,10);
        const qreal knobX=16.0+16.0*clamped;
        p.setBrush(Qt::white);p.drawEllipse(QPointF(knobX,14),8,8);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setBrush(Qt::NoBrush);p.setPen(QPen(accent,1.5));p.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),11,11);}

    }
};

// Bare adjustment sign with the same hit target and hold-to-repeat behavior.
class StepButton final : public MonoButton {
    QTimer repeat_,delay_;
    int direction_;
    std::function<void(int)> callback_;
    void fire(){if(isEnabled()&&isDown()&&callback_)callback_(direction_);}
public:
    StepButton(const QString& glyph,int direction,std::function<void(int)> callback,QWidget* parent=nullptr)
        :MonoButton(glyph,parent),direction_(direction),callback_(std::move(callback)){
        setObjectName("stepButton");setFixedSize(26,26);setAutoDefault(false);setDefault(false);setCursor(Qt::PointingHandCursor);
        repeat_.setInterval(70);delay_.setSingleShot(true);delay_.setInterval(350);
        connect(&delay_,&QTimer::timeout,this,[this]{repeat_.start();});
        connect(&repeat_,&QTimer::timeout,this,[this]{fire();});
        connect(this,&QPushButton::pressed,this,[this]{fire();delay_.start();});
        connect(this,&QPushButton::released,this,[this]{delay_.stop();repeat_.stop();});
    }
protected:
    void hideEvent(QHideEvent* event) override{delay_.stop();repeat_.stop();MonoButton::hideEvent(event);}
};

// Keep the content at its natural size while its viewport opens/closes. Reversals
// start at the displayed progress, and an empty viewport reserves no layout gap.
class ExpandingContent final : public QWidget {
    QWidget* content_;
    QVariantAnimation animation_;
    qreal progress_=0;
    int fullHeight_=0,duration_;
    bool expanded_=false,initialized_=false;
    int naturalHeight() const {
        return std::max(0,content_->hasHeightForWidth()?content_->heightForWidth(width()):content_->sizeHint().height());
    }
    void apply(){
        setFixedHeight(qRound(fullHeight_*progress_));update();
    }
    void sizeContent(){content_->ensurePolished();fullHeight_=naturalHeight();content_->setGeometry(0,0,width(),fullHeight_);if(content_->layout())content_->layout()->activate();}
    void settle(){animation_.stop();progress_=expanded_?1:0;sizeContent();content_->setAttribute(Qt::WA_TransparentForMouseEvents,!expanded_);content_->setEnabled(true);content_->setVisible(expanded_);apply();}
public:
    explicit ExpandingContent(QWidget* content,int duration=200,QWidget* parent=nullptr):QWidget(parent),content_(content),duration_(duration){
        content_->setParent(this);
        content_->installEventFilter(this);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setFixedHeight(0);
        animation_.setEasingCurve(monoEase());
        connect(&animation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){progress_=value.toReal();apply();});
        connect(&animation_,&QVariantAnimation::finished,this,[this]{settle();});
    }
    bool expanded() const{return expanded_;}
    bool transitioning() const{return animation_.state()==QAbstractAnimation::Running;}
    qreal progress() const{return progress_;}
    void setExpanded(bool expanded,bool animate=true){
        if(initialized_&&expanded_==expanded)return;
        const bool first=!initialized_;initialized_=true;expanded_=expanded;animation_.stop();
        if(first||!animate||!isVisible()){settle();return;}
        // Clip the live content at its natural size. Its ink and background keep
        // the same opacity throughout, with no snapshot/live-widget handover.
        sizeContent();content_->setEnabled(true);content_->setAttribute(Qt::WA_TransparentForMouseEvents,!expanded_);content_->show();
        retargetAnimation(animation_,progress_,expanded?1.0:0.0,std::max(120,qRound(duration_*std::abs((expanded?1.0:0.0)-progress_))));
    }
    QSize sizeHint() const override{return {content_->sizeHint().width(),qRound(fullHeight_*progress_)};}
protected:
    bool eventFilter(QObject* watched,QEvent* event) override{
        if(watched==content_&&event->type()==QEvent::LayoutRequest){sizeContent();apply();}
        return QWidget::eventFilter(watched,event);
    }
    void resizeEvent(QResizeEvent* event) override{QWidget::resizeEvent(event);if(event->oldSize().width()!=width()){sizeContent();apply();}}
    void hideEvent(QHideEvent* event) override{settle();QWidget::hideEvent(event);}
};

class WeaponButton final : public QPushButton {
    QVariantAnimation motion_;
    qreal amount_=0;
    bool chosen_=false,initialized_=false;
    std::function<QColor()> accent_;
public:
    explicit WeaponButton(std::function<QColor()> accent):accent_(std::move(accent)){
        setObjectName("weaponChoice");setCursor(Qt::PointingHandCursor);setMinimumSize(88,40);setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Fixed);setCheckable(true);setAutoExclusive(true);motion_.setEasingCurve(monoEase());
        connect(&motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){amount_=value.toReal();update();});
    }
    void setChosen(bool chosen){
        if(initialized_&&chosen_==chosen)return;
        const bool animate=initialized_&&isVisible();initialized_=true;chosen_=chosen;
        setProperty("chosen",chosen);{QSignalBlocker blocked(this);setChecked(chosen);}
        if(animate)retargetAnimation(motion_,amount_,chosen?1:0,140);
        else{motion_.stop();amount_=chosen?1:0;update();}
    }
    QSize sizeHint() const override{return {std::max(88,fontMetrics().horizontalAdvance(text())+24),40};}
protected:
    void hideEvent(QHideEvent* event) override{motion_.stop();amount_=chosen_?1:0;QPushButton::hideEvent(event);}
    void paintEvent(QPaintEvent*) override{

        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);if(!isEnabled())painter.setOpacity(.4);
        const QRectF pill=QRectF(rect()).adjusted(1,1,-1,-1);
        const qreal amount=std::clamp(amount_,0.0,1.0);
        // Compact tabs share a hairline baseline; only the selection has color.
        QColor underline=accent_();underline.setAlphaF(amount);
        painter.setPen(QPen(underline,2,Qt::SolidLine,Qt::FlatCap));
        painter.drawLine(QPointF(12,height()-1),QPointF(width()-12,height()-1));
        QColor ink=mixColor(kbtheme::content(.7),kbtheme::ink,amount);if(isDown())ink=mixColor(ink,kbtheme::ink,.35);
        painter.setFont(font());painter.setPen(ink);painter.drawText(rect().adjusted(0,0,0,-4),Qt::AlignCenter,text());
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setBrush(Qt::NoBrush);painter.setPen(QPen(accent_(),1.5));painter.drawRoundedRect(pill,8,8);}

    }
};

inline QString operatorAssetName(const QString& name) {
    QString asset; asset.reserve(name.size());
    for (const QChar ch : name) if (ch.isLetterOrNumber()) asset.append(ch.toLower());
    return asset;
}
inline QPixmap operatorPixmap(const QString& name,int size=32,qreal ratio=1) {
    const QString asset=operatorAssetName(name);
    const QString key="azec/operator/"+asset+"/"+QString::number(size)+"/"+QString::number(ratio,'g',12);
    QPixmap cached;if(QPixmapCache::find(key,&cached))return cached;
    QPixmap pixmap(QSize(size,size)*ratio);pixmap.setDevicePixelRatio(ratio);pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);painter.setRenderHint(QPainter::Antialiasing);
    QSvgRenderer renderer(":/operators/"+asset+".svg");
    if(renderer.isValid())renderer.render(&painter,QRectF(0,0,size,size));
    painter.end();QPixmapCache::insert(key,pixmap);
    return pixmap;
}
inline QIcon operatorIcon(const QString& name) {QIcon icon;for(int scale:{1,2,3})icon.addPixmap(operatorPixmap(name,operatorIconSize,scale));return icon;}

class OperatorDelegate final : public QStyledItemDelegate {
    std::function<QColor()> accent_;
public:
    explicit OperatorDelegate(std::function<QColor()> accent,QObject* parent):QStyledItemDelegate(parent),accent_(std::move(accent)){}
    QSize sizeHint(const QStyleOptionViewItem&,const QModelIndex&) const override{return {200,operatorRowHeight};}
    void paint(QPainter* p,const QStyleOptionViewItem& option,const QModelIndex& index) const override {

        p->save();p->setRenderHint(QPainter::Antialiasing);p->setRenderHint(QPainter::TextAntialiasing);const QRect r=option.rect;
        const bool selected=option.state&QStyle::State_Selected;
        const int visibleWidth=option.widget?option.widget->width():r.width();
        const QRectF pill=QRectF(QRect(r.left(),r.top(),visibleWidth,r.height()).adjusted(4,2,-4,-2));
        if(selected){p->setPen(Qt::NoPen);p->setBrush(kbtheme::lavender);p->drawRoundedRect(pill,8,8);}
        // Anchor artwork and names while the sidebar clips/fades their trailing
        // edge. Recentring icons or re-eliding text on every frame produces
        // sideways jumps. Right-to-left languages anchor from the other edge.
        const bool rtl=option.widget&&option.widget->layoutDirection()==Qt::RightToLeft;
        // The item is 200 px wide while the viewport is narrower, so the far
        // edge of option.rect is already outside the visible row.
        const int visibleRight=option.widget?option.widget->width()-1:r.right();
        const int iconX=rtl?visibleRight-operatorIconSize-9:r.left()+10;
        qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).paint(p,QRect(iconX,r.top()+4,operatorIconSize,operatorIconSize));
        QFont f=option.font;f.setFamily("Segoe UI");f.setPixelSize(13);f.setWeight(QFont::Normal);p->setFont(f);p->setPen(kbtheme::ink);
        p->setClipRect(r);p->setOpacity(std::clamp((r.width()-58.0)/116.0,0.0,1.0));
        const int nameWidth=qMax(0,visibleRight-r.left()-73);
        const QFontMetrics metrics(f);
        const auto name=metrics.elidedText(index.data().toString(),Qt::ElideRight,nameWidth);
        if(rtl){
            // Place mirrored text explicitly, retaining the original baseline.
            p->drawText(QPoint(qMax(r.left(),visibleRight-65-metrics.horizontalAdvance(name)),r.top()+(r.height()+metrics.ascent()-metrics.descent())/2),name);
        }else{
            p->drawText(QRect(r.left()+65,r.top(),nameWidth,r.height()),Qt::AlignVCenter|Qt::AlignLeft,name);
        }
        if(selected&&(option.state&QStyle::State_HasFocus)){
            p->setOpacity(1);p->setBrush(Qt::NoBrush);p->setPen(QPen(accent_(),1.5));p->drawRoundedRect(pill.adjusted(2,2,-2,-2),9,9);
        }
        p->restore();
    }
};

class FavoritePopup final : public QMenu {
    bool chosen_=false;
public:
    FavoritePopup(const QString& text,QWidget* parent):QMenu(parent){
        setObjectName("favoritePopup");setMinimumWidth(158);setAttribute(Qt::WA_TranslucentBackground);
        auto* action=addAction(QIcon(":/ui/star.svg"),text);action->setObjectName("toggleFavorite");
        connect(action,&QAction::triggered,this,[this]{chosen_=true;close();});
    }
    bool execAt(QPoint position){exec(position);return chosen_;}
};

class BrandLabel final : public QLabel {
public:
    BrandLabel(){
        setText("Azec");setObjectName("brand");setAccessibleName("Azec");setAlignment(Qt::AlignCenter);
        QFont brand=QApplication::font();brand.setPixelSize(13);brand.setWeight(QFont::Medium);brand.setLetterSpacing(QFont::AbsoluteSpacing,0.4);
        setFont(brand);setFixedHeight(36);
    }
};

// The lists expand inside one sidebar scroll area; wheel events go to that area.
class OperatorList final : public QListWidget {
    QScrollArea* area_;
public:explicit OperatorList(QScrollArea* area):area_(area){}
protected:
    void wheelEvent(QWheelEvent* event) override;
};

// Paint the identity directly: nested graphics effects would disturb the panel.
class FadeLabel final : public QLabel {
    qreal opacity_=1;
public:
    void setOpacity(qreal opacity){opacity_=opacity;update();}
    qreal opacity() const{return opacity_;}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);painter.setOpacity(opacity_);
        if(!pixmap().isNull())style()->drawItemPixmap(&painter,contentsRect(),alignment(),pixmap());
        else {painter.setFont(font());style()->drawItemText(&painter,contentsRect(),alignment(),palette(),isEnabled(),text(),QPalette::WindowText);}
    }
};

class RaisedPanel final : public QWidget {
public:
    explicit RaisedPanel(QWidget* parent=nullptr):QWidget(parent){}
    QRectF surfaceRect() const{return QRectF(rect()).adjusted(8,3,-12,-13);}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);
        const auto surface=surfaceRect();
        // The inset card: a deep translucent panel over the glass, one hairline.
        painter.setBrush(kbtheme::panelGlass);painter.drawRoundedRect(surface,raisedPanelRadius,raisedPanelRadius);
        painter.setBrush(Qt::NoBrush);painter.setPen(QPen(kbtheme::outline,1));painter.drawRoundedRect(surface.adjusted(.5,.5,-.5,-.5),raisedPanelRadius,raisedPanelRadius);
    }
};

class PageSnapshot final : public QWidget {
    QPixmap previous_,next_;qreal progress_=0;bool wholeWindow_=false;
public:
    PageSnapshot(QPixmap previous,QPixmap next,QWidget* parent,bool wholeWindow=false):QWidget(parent),previous_(std::move(previous)),next_(std::move(next)),wholeWindow_(wholeWindow){
        setAttribute(Qt::WA_NoSystemBackground);setObjectName("pageTransition");
        if(wholeWindow_){setAttribute(Qt::WA_TransparentForMouseEvents);setObjectName("themeTransition");}
    }
    void setProgress(qreal progress){progress_=progress;update();}
    QPixmap frame() const{
        QPixmap result(previous_.size());result.setDevicePixelRatio(previous_.devicePixelRatio());result.fill(Qt::transparent);
        QPainter painter(&result);
        // Sum two premultiplied images instead of stacking translucent cards.
        // Both pages keep the live acrylic underneath at exactly the same alpha.
        painter.setOpacity(1-progress_);painter.drawPixmap(0,0,previous_);
        painter.setCompositionMode(QPainter::CompositionMode_Plus);
        painter.setOpacity(progress_);painter.drawPixmap(0,0,next_);
        painter.end();
        return result;
    }
protected:
    void paintEvent(QPaintEvent*) override {
        if(wholeWindow_){
            // Replace the backing pixels instead of stacking another translucent
            // window over the live controls. Acrylic stays live underneath.
            QPainter painter(this);painter.setCompositionMode(QPainter::CompositionMode_Source);painter.drawPixmap(0,0,frame());return;
        }
        // The snapshot is a rectangular pixmap; clip it to the panel interior so
        // the hairline and rounded corners stay visible while it fades out.
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        const qreal inset=raisedPanelBorder;
        QPainterPath clip;
        clip.addRoundedRect(QRectF(rect()).adjusted(inset,inset,-inset,-inset),raisedPanelRadius-raisedPanelBorder/2.0,raisedPanelRadius-raisedPanelBorder/2.0);
        painter.setClipPath(clip);painter.drawPixmap(0,0,frame());
    }
};

class PageStack final : public QStackedWidget {
    QVariantAnimation fade_;PageSnapshot* snapshot_=nullptr;
public:
    explicit PageStack(QWidget* parent=nullptr):QStackedWidget(parent){
        fade_.setDuration(160);fade_.setStartValue(0.0);fade_.setEndValue(1.0);fade_.setEasingCurve(monoEase());
        connect(&fade_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){if(snapshot_)snapshot_->setProgress(value.toReal());});
        connect(&fade_,&QVariantAnimation::finished,this,[this]{settle();});
    }
    void settle(){fade_.stop();if(snapshot_){delete snapshot_;snapshot_=nullptr;if(currentWidget())currentWidget()->show();}}
    bool transitioning() const{return snapshot_!=nullptr;}
    void transition(const std::function<void()>& change){
        QPixmap previous;
        const auto capture=[this](QWidget* widget){
            QPixmap image(size()*devicePixelRatioF());image.setDevicePixelRatio(devicePixelRatioF());image.fill(Qt::transparent);
            QPainter painter(&image);widget->render(&painter,QPoint(),QRegion(),QWidget::DrawChildren);return image;
        };
        if(isVisible())previous=snapshot_?snapshot_->frame():capture(currentWidget());
        settle();change();auto* page=currentWidget();
        if(previous.isNull())return;
        if(page->layout())page->layout()->activate();
        QPixmap next=capture(page);
        // Only the two cached foregrounds animate. Painting the live target as
        // well would double the card fills and briefly darken the whole panel.
        page->hide();
        snapshot_=new PageSnapshot(std::move(previous),std::move(next),this);snapshot_->setGeometry(rect());snapshot_->show();snapshot_->raise();fade_.start();
    }
    void switchTo(QWidget* page){
        if(page==currentWidget()||indexOf(page)<0)return;
        transition([this,page]{setCurrentWidget(page);});
    }
protected:
    void resizeEvent(QResizeEvent* event) override {settle();QStackedWidget::resizeEvent(event);}
    void hideEvent(QHideEvent* event) override {settle();QStackedWidget::hideEvent(event);}
};

// Wheel notches glide to a nearby target instead of jumping a whole chunk.
// Precision-touchpad pixel deltas stay one-to-one so native scrolling is kept.
class SmoothScrollArea final : public QScrollArea {
    QVariantAnimation glide_;
    int target_=0;
    bool wired_=false;
public:
    explicit SmoothScrollArea(QWidget* parent=nullptr):QScrollArea(parent){
        glide_.setDuration(130);
        glide_.setEasingCurve(QEasingCurve::OutCubic);
        connect(&glide_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){
            if(auto* bar=verticalScrollBar())bar->setValue(qRound(value.toReal()));
        });
    }
    // The fading bar is installed after construction; bind to the bar that is
    // actually used and stop a glide as soon as the user grabs it.
    void wireScrollBar(){
        auto* bar=verticalScrollBar();if(!bar||wired_)return;wired_=true;
        connect(bar,&QScrollBar::sliderPressed,&glide_,&QVariantAnimation::stop);
        connect(bar,&QScrollBar::actionTriggered,&glide_,&QVariantAnimation::stop);
    }
    static double notchDistance(int angleDelta){return angleDelta/120.0*96.0;}
    void glideBy(double distance){
        auto* bar=verticalScrollBar();if(!bar||bar->maximum()<=bar->minimum())return;
        // Repeated notches add to the pending target so quick wheel bursts keep
        // their momentum instead of restarting from the visible position.
        const int origin=glide_.state()==QAbstractAnimation::Running?target_:bar->value();
        target_=qBound(bar->minimum(),qRound(origin-distance),bar->maximum());
        if(target_==bar->value()&&glide_.state()!=QAbstractAnimation::Running)return;
        glide_.stop();glide_.setStartValue(qreal(bar->value()));glide_.setEndValue(qreal(target_));glide_.start();
    }
    void scrollByWheel(int angleDelta,int pixelDelta){
        auto* bar=verticalScrollBar();if(!bar)return;
        if(pixelDelta){glide_.stop();bar->setValue(bar->value()-pixelDelta);}
        else if(angleDelta)glideBy(notchDistance(angleDelta));
    }
    bool gliding() const{return glide_.state()==QAbstractAnimation::Running;}
    void stopScrolling(){glide_.stop();}
protected:
    void wheelEvent(QWheelEvent* event) override {
        auto* bar=verticalScrollBar();
        if(bar&&bar->maximum()>bar->minimum()){
            if(int pixels=event->pixelDelta().y();pixels){scrollByWheel(0,pixels);event->accept();return;}
            if(int angle=event->angleDelta().y();angle){scrollByWheel(angle,0);event->accept();return;}
        }
        QScrollArea::wheelEvent(event);
    }
};

// The sidebar lists live inside one SmoothScrollArea, but they receive the
// wheel themselves. Route those events straight into the shared glide instead
// of relying on viewport event forwarding, which would jump a whole step.
inline void OperatorList::wheelEvent(QWheelEvent* event) {
    if(auto* smooth=dynamic_cast<SmoothScrollArea*>(area_)){
        smooth->scrollByWheel(event->angleDelta().y(),event->pixelDelta().y());
        event->accept();
        return;
    }
    QWheelEvent forwarded(QPointF(area_->viewport()->mapFromGlobal(event->globalPosition().toPoint())),event->globalPosition(),event->pixelDelta(),event->angleDelta(),event->buttons(),event->modifiers(),event->phase(),event->inverted());
    QApplication::sendEvent(area_->viewport(),&forwarded);event->setAccepted(forwarded.isAccepted());
}

class FadingScrollBar final : public QScrollBar {
    QTimer idle_;
    QVariantAnimation fade_;
    qreal opacity_=0;
    qreal fadeTarget_=0;
    void fadeTo(qreal target){
        // A glide emits valueChanged on every frame; without this guard the
        // fade would restart repeatedly and never reach full opacity.
        if(fadeTarget_==target&&fade_.state()==QAbstractAnimation::Running)return;
        if(qFuzzyCompare(opacity_,target)&&fade_.state()!=QAbstractAnimation::Running)return;
        fadeTarget_=target;
        fade_.stop();fade_.setDuration(target>opacity_?100:240);
        fade_.setStartValue(opacity_);fade_.setEndValue(target);fade_.start();
    }
public:
    explicit FadingScrollBar(QWidget* parent=nullptr):QScrollBar(Qt::Vertical,parent){
        idle_.setSingleShot(true);idle_.setInterval(3000);
        fade_.setEasingCurve(QEasingCurve::InOutQuad);
        connect(&fade_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){opacity_=value.toReal();update();});
        connect(&idle_,&QTimer::timeout,this,[this]{if(!isSliderDown())fadeTo(0);});
        connect(this,&QScrollBar::valueChanged,this,[this]{reveal();});
        connect(this,&QScrollBar::sliderPressed,this,[this]{idle_.stop();fadeTo(1);});
        connect(this,&QScrollBar::sliderReleased,this,[this]{reveal();});
    }
    void reveal(){if(maximum()<=minimum())return;fadeTo(1);if(!isSliderDown())idle_.start();}
    qreal opacity() const{return opacity_;}
    static FadingScrollBar* install(QAbstractScrollArea* area){
        auto* bar=new FadingScrollBar(area);area->setVerticalScrollBar(bar);
        // The lane is always reserved, including when there is no overflow.
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        area->viewport()->installEventFilter(bar);
        if(auto* smooth=dynamic_cast<SmoothScrollArea*>(area))smooth->wireScrollBar();
        return bar;
    }
protected:
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()==QEvent::Wheel)reveal();
        return QScrollBar::eventFilter(object,event);
    }
    void paintEvent(QPaintEvent*) override {
        if(opacity_<=0||maximum()<=minimum())return;
        QStyleOptionSlider option;initStyleOption(&option);QPainter painter(this);painter.setOpacity(opacity_);
        style()->drawComplexControl(QStyle::CC_ScrollBar,&option,&painter,this);
    }
};

class DragHeader final : public QFrame {
public:using QFrame::QFrame;
protected:
    void mousePressEvent(QMouseEvent* e) override {if(e->button()==Qt::LeftButton&&window()->windowHandle()){window()->windowHandle()->startSystemMove();e->accept();}else QFrame::mousePressEvent(e);}
};

// Plain section heading: quiet, uppercase-free, no chip and no tilt.
class SectionHeading final : public QLabel {
public:
    SectionHeading(const QString& text,QWidget* parent=nullptr):QLabel(parent){setText(text);setObjectName("sectionHeading");setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);}
    QSize sizeHint() const override {const QFontMetrics metrics(headingFont());return {metrics.horizontalAdvance(text())+2,26};}
    // Long translations must never widen the 200 px sidebar: the heading elides instead.
    QSize minimumSizeHint() const override {return {qMin(sizeHint().width(),150),26};}
    static QFont headingFont(){QFont font=QApplication::font();font.setPixelSize(13);font.setWeight(QFont::DemiBold);return font;}
protected:
    void paintEvent(QPaintEvent*) override {

        QPainter painter(this);painter.setRenderHint(QPainter::TextAntialiasing);
        const QFontMetrics metrics(headingFont());painter.setFont(headingFont());painter.setPen(kbtheme::ink);
        painter.drawText(QRectF(rect()),Qt::AlignVCenter|Qt::AlignLeft,metrics.elidedText(text(),Qt::ElideRight,qMax(8.0,width()-2.0)));
    }
};

class Section final : public QFrame {
public:
    QVBoxLayout* body;QWidget* content;QHBoxLayout* headerRow;
    explicit Section(const QString& title,QWidget* parent=nullptr):QFrame(parent) {
        setObjectName("settingCard");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(8);
        auto* header=new QWidget;auto* row=headerRow=new QHBoxLayout(header);row->setContentsMargins(2,0,2,0);header->setMinimumHeight(24);
        auto* titleLabel=new QLabel(title);titleLabel->setObjectName("sectionTitle");titleLabel->setWordWrap(true);titleLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);row->addWidget(titleLabel,1);layout->addWidget(header);
        content=new QWidget;body=new QVBoxLayout(content);body->setContentsMargins(1,1,1,1);body->setSpacing(0);layout->addWidget(content);
        content->setObjectName("settingGroup");content->setAttribute(Qt::WA_StyledBackground);
    }
    void setOpenProfile(bool separator=false){setOpenSection("openProfile","profileGroup",separator);}
    void setOpenSettings(bool separator=false){setOpenSection("openSettings","settingsGroup",separator);}
private:
    void setOpenSection(const char* propertyName,const char* contentName,bool separator){
        setProperty(propertyName,true);content->setObjectName(contentName);body->setContentsMargins(0,0,0,0);
        headerRow->setContentsMargins(0,0,0,0);
        auto* column=static_cast<QVBoxLayout*>(layout());column->setSpacing(12);
        if(separator){auto* line=new QFrame;line->setObjectName("profileDivider");line->setFixedHeight(1);column->insertWidget(0,line);}
    }
};
