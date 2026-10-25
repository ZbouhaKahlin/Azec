#pragma once
#include <QtWidgets>
#include <algorithm>
#include "theme.hpp"
#include "interaction_icons.hpp"

// Qt ports of MonoCode's SecondaryButton.tsx and SettingsView.tsx Row:
// 12 px / 400 actions, content/70 ink, 10x4 padding, 6 px corners;
// descriptions use content/45 with a 1.625 line height. MIT notice in licenses/monocode.
class MonoButton : public QPushButton {
    QVariantAnimation hoverMotion_;
    QVariantAnimation pressMotion_,feedbackMotion_;
    QTimer feedbackHold_;
    qreal hover_=0;
    qreal press_=0,feedback_=0;
    InteractionIcon glyph_=InteractionIcon::None;
    bool glyphActive_=false;
    void hoverTo(qreal target){
        QSignalBlocker block(&hoverMotion_);hoverMotion_.stop();
        hoverMotion_.setStartValue(hover_);hoverMotion_.setEndValue(target);
        hoverMotion_.setCurrentTime(0);block.unblock();hoverMotion_.start();
    }
    static QColor content(qreal opacity){QColor color=kbtheme::ink;color.setAlphaF(opacity);return color;}
    void pressTo(qreal target){
        QSignalBlocker blocked(&pressMotion_);pressMotion_.stop();pressMotion_.setDuration(target>press_?70:160);pressMotion_.setStartValue(press_);pressMotion_.setEndValue(target);pressMotion_.setCurrentTime(0);blocked.unblock();pressMotion_.start();
    }
public:
    explicit MonoButton(QWidget* parent=nullptr):QPushButton(parent){
        setCursor(Qt::PointingHandCursor);setAutoDefault(false);
        hoverMotion_.setDuration(120);
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(.22,1),QPointF(.36,1),QPointF(1,1));
        hoverMotion_.setEasingCurve(curve);
        pressMotion_.setEasingCurve(curve);
        connect(&hoverMotion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){hover_=value.toReal();update();});
        connect(&pressMotion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){press_=value.toReal();update();});
        connect(this,&QPushButton::pressed,this,[this]{if(glyph_!=InteractionIcon::None)pressTo(1);});connect(this,&QPushButton::released,this,[this]{if(glyph_!=InteractionIcon::None)pressTo(0);});
        connect(&feedbackMotion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){feedback_=value.toReal();update();});
        feedbackHold_.setSingleShot(true);feedbackHold_.setInterval(1400);
        connect(&feedbackHold_,&QTimer::timeout,this,[this]{feedbackMotion_.stop();feedback_=0;update();});
    }
    explicit MonoButton(const QString& text,QWidget* parent=nullptr):MonoButton(parent){setText(text);}
    QSize sizeHint() const override{auto size=QPushButton::sizeHint();size.setHeight(qMax(26,size.height()));return size;}
    void setInteractionIcon(InteractionIcon glyph){glyph_=glyph;setIcon(QIcon(interactionIconAsset(glyph)));setIconSize(QSize(14,14));updateGeometry();update();}
    void confirmCopy(){
        feedbackMotion_.stop();feedback_=0;
        if(isVisible()){feedbackMotion_.setDuration(380);feedbackMotion_.setStartValue(0.0);feedbackMotion_.setEndValue(1.0);feedbackMotion_.start();feedbackHold_.start();}update();
    }
    void clearCopyFeedback(){feedbackHold_.stop();feedbackMotion_.stop();feedback_=0;setToolTip(QString());update();}
    void setStatusActive(bool active){
        setProperty("active",active);if(glyphActive_==active)return;glyphActive_=active;
        feedbackMotion_.stop();feedback_=active?1:0;
        if(active&&isVisible()){feedbackMotion_.setDuration(360);feedbackMotion_.setStartValue(0.0);feedbackMotion_.setEndValue(1.0);feedbackMotion_.start();}update();
    }
protected:
    void enterEvent(QEnterEvent* event) override{QPushButton::enterEvent(event);hoverTo(1);}
    void leaveEvent(QEvent* event) override{QPushButton::leaveEvent(event);hoverTo(0);}
    void hideEvent(QHideEvent* event) override{hoverMotion_.stop();pressMotion_.stop();feedbackMotion_.stop();feedbackHold_.stop();hover_=press_=0;feedback_=glyphActive_?1:0;QPushButton::hideEvent(event);}
    void paintEvent(QPaintEvent*) override{
        const auto role=objectName();
        const bool status=role=="status"||role=="statusRapid";
        const bool primary=role=="save"||role=="preferencesSave";
        const bool quiet=role=="driverManualDownload"||role=="driverTroubleshooting"||role=="sectionHeading";
        const bool step=role=="stepButton";
        const bool plain=property("plainAction").toBool();
        const bool active=property("active").toBool();
        const bool danger=role=="discardChanges";
        const qreal feedback=isDown()?1:std::clamp(hover_,qreal(0),qreal(1));
        QColor fill=content((quiet||step||status||plain?0.0:.04)+feedback*.06);
        QColor ink=content(.7+feedback*.3);
        if(primary){fill=isDown()?kbtheme::defaultAccent.darker(112):kbtheme::defaultAccent.lighter(100+qRound(feedback*8));ink=kbtheme::readableOn(fill);}
        if(status&&active){fill=kbtheme::defaultAccent;fill.setAlphaF(feedback*.06);ink=kbtheme::activeInk;}
        if(status)fill=Qt::transparent;
        if(glyph_==InteractionIcon::Copy&&feedback_>0)ink=kbtheme::activeInk;
        if(danger){ink=kbtheme::danger;if(feedback>0){fill=kbtheme::pink;fill.setAlphaF(feedback*.12);}}
        QPainter painter(this);painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
        if(!isEnabled())painter.setOpacity(.4);
        const QRectF face=QRectF(rect()).adjusted(.5,.5,-.5,-.5);
        const qreal radius=status||step?face.height()/2:6;
        painter.setPen(Qt::NoPen);painter.setBrush(fill);painter.drawRoundedRect(face,radius,radius);
        if(plain&&role=="keyButton"){
            painter.setPen(QPen(content(.14),1));
            painter.drawLine(QPointF(10,height()-1),QPointF(width()-10,height()-1));
        }
        painter.setFont(font());painter.setPen(ink);
        const auto alignment=quiet?(Qt::AlignAbsolute|Qt::AlignVCenter|(layoutDirection()==Qt::RightToLeft?Qt::AlignRight:Qt::AlignLeft)):Qt::AlignCenter;
        QRect label=rect().adjusted(quiet?6:10,0,quiet?-6:-10,0);
        if(!icon().isNull()){
            const int size=14,gap=6;
            const int textWidth=fontMetrics().horizontalAdvance(text());
            const int left=(width()-size-gap-textWidth)/2;
            const bool rtl=layoutDirection()==Qt::RightToLeft;
            const QRectF iconArea(rtl?left+textWidth+gap:left,(height()-size)/2.0,size,size);
            if(glyph_!=InteractionIcon::None){
                painter.save();painter.translate(iconArea.center());painter.scale(1-.06*press_,1-.06*press_);painter.translate(-iconArea.center());
                paintInteractionIcon(painter,iconArea,glyph_,ink,status?(active?feedback_:1):feedback_);painter.restore();
            }else icon().paint(&painter,iconArea.toRect());
            label=QRect(rtl?left:left+size+gap,0,textWidth,height());
        }
        if(step){
            painter.setPen(QPen(ink,1.4,Qt::SolidLine,Qt::RoundCap));
            const QPointF center=face.center();painter.drawLine(center+QPointF(-3.5,0),center+QPointF(3.5,0));
            if(text()=="+")painter.drawLine(center+QPointF(0,-3.5),center+QPointF(0,3.5));
        }else{
            QPalette colors=palette();colors.setColor(QPalette::ButtonText,ink);colors.setColor(QPalette::Disabled,QPalette::ButtonText,ink);
            style()->drawItemText(&painter,label,alignment|Qt::TextShowMnemonic,colors,isEnabled(),text(),QPalette::ButtonText);
        }
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){
            painter.setPen(QPen(kbtheme::defaultAccent,1.5));painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(face.adjusted(1,1,-1,-1),radius,radius);
        }
    }
};

// QLabel keeps its public text/accessibility API; QTextDocument supplies the
// reference's 19.5 px paragraph leading instead of Qt's tighter default.
class MonoDescription final : public QLabel {
    QStringList reservedMessages_;
    void prepare(QTextDocument& document,int width,const QString& message) const{
        document.setDocumentMargin(0);document.setDefaultFont(font());
        QTextOption option;option.setWrapMode(wordWrap()?QTextOption::WrapAtWordBoundaryOrAnywhere:QTextOption::NoWrap);
        option.setTextDirection(layoutDirection());document.setDefaultTextOption(option);
        option.setAlignment(alignment()&Qt::AlignHorizontal_Mask);document.setDefaultTextOption(option);
        document.setPlainText(message);
        QTextCursor cursor(&document);cursor.select(QTextCursor::Document);
        QTextBlockFormat block;block.setLineHeight(QFontInfo(font()).pixelSize()*1.625,QTextBlockFormat::FixedHeight);cursor.setBlockFormat(block);
        QTextCharFormat ink;ink.setForeground(kbtheme::muted);cursor.setCharFormat(ink);
        document.setTextWidth(qMax(1,width));
    }
public:
    explicit MonoDescription(const QString& text,QWidget* parent=nullptr):QLabel(text,parent){
        auto policy=sizePolicy();policy.setHeightForWidth(true);setSizePolicy(policy);
    }
    void reserveMessages(const QStringList& messages){reservedMessages_=messages;updateGeometry();}
    bool hasHeightForWidth() const override{return wordWrap();}
    int heightForWidth(int width) const override{
        const auto margins=contentsMargins();const int available=width-margins.left()-margins.right();
        QTextDocument document;prepare(document,available,text());int height=qCeil(document.size().height());
        for(const auto& message:reservedMessages_){QTextDocument reserved;prepare(reserved,available,message);height=qMax(height,qCeil(reserved.size().height()));}
        return height+margins.top()+margins.bottom();
    }
    QSize minimumSizeHint() const override{return {0,20+contentsMargins().top()+contentsMargins().bottom()};}
    QSize sizeHint() const override{
        int width=QLabel::sizeHint().width();
        for(const auto& message:reservedMessages_)width=qMax(width,qMin(400,fontMetrics().horizontalAdvance(message)));
        return {width,heightForWidth(qMax(1,width))};
    }
protected:
    void paintEvent(QPaintEvent*) override{
        QTextDocument document;prepare(document,contentsRect().width(),text());
        QPainter painter(this);QPointF origin=contentsRect().topLeft();
        const qreal spare=qMax(0.0,contentsRect().height()-document.size().height());
        if(alignment()&Qt::AlignVCenter)origin.ry()+=spare/2;else if(alignment()&Qt::AlignBottom)origin.ry()+=spare;
        painter.translate(origin);
        if(!isEnabled())painter.setOpacity(.4);
        document.drawContents(&painter);
    }
};
