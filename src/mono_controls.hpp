#pragma once
#include <QtWidgets>
#include <algorithm>
#include "theme.hpp"

// Qt ports of MonoCode's SecondaryButton.tsx and SettingsView.tsx Row:
// 12 px / 400 actions, content/70 ink, 10x4 padding, 6 px corners;
// descriptions use content/45 with a 1.625 line height. MIT notice in licenses/monocode.
class MonoButton : public QPushButton {
    QVariantAnimation hoverMotion_;
    qreal hover_=0;
    void hoverTo(qreal target){
        QSignalBlocker block(&hoverMotion_);hoverMotion_.stop();
        hoverMotion_.setStartValue(hover_);hoverMotion_.setEndValue(target);
        hoverMotion_.setCurrentTime(0);block.unblock();hoverMotion_.start();
    }
    static QColor content(qreal opacity){QColor color=kbtheme::ink;color.setAlphaF(opacity);return color;}
public:
    explicit MonoButton(QWidget* parent=nullptr):QPushButton(parent){
        setCursor(Qt::PointingHandCursor);setAutoDefault(false);
        hoverMotion_.setDuration(120);
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(.22,1),QPointF(.36,1),QPointF(1,1));
        hoverMotion_.setEasingCurve(curve);
        connect(&hoverMotion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){hover_=value.toReal();update();});
    }
    explicit MonoButton(const QString& text,QWidget* parent=nullptr):MonoButton(parent){setText(text);}
    QSize sizeHint() const override{auto size=QPushButton::sizeHint();size.setHeight(qMax(26,size.height()));return size;}
protected:
    void enterEvent(QEnterEvent* event) override{QPushButton::enterEvent(event);hoverTo(1);}
    void leaveEvent(QEvent* event) override{QPushButton::leaveEvent(event);hoverTo(0);}
    void hideEvent(QHideEvent* event) override{hoverMotion_.stop();hover_=0;QPushButton::hideEvent(event);}
    void paintEvent(QPaintEvent*) override{
        const auto role=objectName();
        const bool status=role=="status"||role=="statusRapid";
        const bool primary=role=="save"||role=="preferencesSave";
        const bool quiet=role=="driverManualDownload"||role=="driverTroubleshooting"||role=="sectionHeading";
        const bool step=role=="stepButton";
        const bool active=property("active").toBool();
        const bool danger=role=="discardChanges";
        const qreal feedback=isDown()?1:std::clamp(hover_,qreal(0),qreal(1));
        QColor fill=content((quiet?0.0:.04)+feedback*.06);
        QColor ink=content(.7+feedback*.3);
        if(primary){fill=isDown()?kbtheme::defaultAccent.darker(112):kbtheme::defaultAccent.lighter(100+qRound(feedback*8));ink=kbtheme::readableOn(fill);}
        if(status&&active){fill=kbtheme::defaultAccent;fill.setAlphaF(.12+feedback*.06);ink=QColor("#8FC1FA");}
        if(danger){ink=kbtheme::danger;if(feedback>0){fill=kbtheme::pink;fill.setAlphaF(feedback*.12);}}
        QPainter painter(this);painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
        if(!isEnabled())painter.setOpacity(.4);
        const QRectF face=QRectF(rect()).adjusted(.5,.5,-.5,-.5);
        const qreal radius=status||step?face.height()/2:6;
        painter.setPen(Qt::NoPen);painter.setBrush(fill);painter.drawRoundedRect(face,radius,radius);
        painter.setFont(font());painter.setPen(ink);
        const auto alignment=quiet?(Qt::AlignVCenter|(layoutDirection()==Qt::RightToLeft?Qt::AlignRight:Qt::AlignLeft)):Qt::AlignCenter;
        QRect label=rect().adjusted(quiet?6:10,0,quiet?-6:-10,0);
        if(!icon().isNull()){
            const int size=14,gap=6;
            const int textWidth=fontMetrics().horizontalAdvance(text());
            const int left=(width()-size-gap-textWidth)/2;
            icon().paint(&painter,QRect(left,(height()-size)/2,size,size));
            label.setLeft(left+size+gap);
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
    void prepare(QTextDocument& document,int width) const{
        document.setDocumentMargin(0);document.setDefaultFont(font());
        QTextOption option;option.setWrapMode(wordWrap()?QTextOption::WrapAtWordBoundaryOrAnywhere:QTextOption::NoWrap);
        option.setTextDirection(layoutDirection());document.setDefaultTextOption(option);
        option.setAlignment(alignment()&Qt::AlignHorizontal_Mask);document.setDefaultTextOption(option);
        document.setPlainText(text());
        QTextCursor cursor(&document);cursor.select(QTextCursor::Document);
        QTextBlockFormat block;block.setLineHeight(QFontInfo(font()).pixelSize()*1.625,QTextBlockFormat::FixedHeight);cursor.setBlockFormat(block);
        QTextCharFormat ink;ink.setForeground(kbtheme::muted);cursor.setCharFormat(ink);
        document.setTextWidth(qMax(1,width));
    }
public:
    explicit MonoDescription(const QString& text,QWidget* parent=nullptr):QLabel(text,parent){
        auto policy=sizePolicy();policy.setHeightForWidth(true);setSizePolicy(policy);
    }
    bool hasHeightForWidth() const override{return wordWrap();}
    int heightForWidth(int width) const override{
        const auto margins=contentsMargins();QTextDocument document;prepare(document,width-margins.left()-margins.right());
        return qCeil(document.size().height())+margins.top()+margins.bottom();
    }
    QSize minimumSizeHint() const override{return {0,20+contentsMargins().top()+contentsMargins().bottom()};}
    QSize sizeHint() const override{return {QLabel::sizeHint().width(),heightForWidth(qMax(1,QLabel::sizeHint().width()))};}
protected:
    void paintEvent(QPaintEvent*) override{
        QTextDocument document;prepare(document,contentsRect().width());
        QPainter painter(this);QPointF origin=contentsRect().topLeft();
        const qreal spare=qMax(0.0,contentsRect().height()-document.size().height());
        if(alignment()&Qt::AlignVCenter)origin.ry()+=spare/2;else if(alignment()&Qt::AlignBottom)origin.ry()+=spare;
        painter.translate(origin);
        if(!isEnabled())painter.setOpacity(.4);
        document.drawContents(&painter);
    }
};
