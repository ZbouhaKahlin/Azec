#pragma once
#include "ui_widgets.hpp"
#include "strings.hpp"

class OnboardingArrow final : public QWidget {
    QSvgRenderer svg_{QStringLiteral(":/ui/tour-arrow.svg")};
    QVariantAnimation animation_;qreal tap_=0;int rotation_=0;
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.translate(8,8);
        QTransform direction;direction.rotate(rotation_);const auto movement=direction.map(QPointF(-5,5))*tap_;p.translate(movement);
        p.translate(56,56);p.rotate(rotation_);p.translate(-56,-56);
        svg_.render(&p,QRectF(0,0,112,112));
    }
    void showEvent(QShowEvent* event) override {QWidget::showEvent(event);animation_.start();}
    void hideEvent(QHideEvent* event) override {animation_.stop();QWidget::hideEvent(event);}
public:
    explicit OnboardingArrow(QWidget* parent):QWidget(parent){
        setObjectName("tourArrow");setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);setFocusPolicy(Qt::NoFocus);setFixedSize(128,128);hide();
        animation_.setDuration(3200);animation_.setLoopCount(-1);animation_.setStartValue(0.0);animation_.setKeyValueAt(.12,0.0);animation_.setKeyValueAt(.24,1.0);animation_.setKeyValueAt(.34,.2);animation_.setKeyValueAt(.40,.6);animation_.setKeyValueAt(.48,0.0);animation_.setEndValue(0.0);animation_.setEasingCurve(QEasingCurve::InOutCubic);
        connect(&animation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& v){tap_=v.toReal();update();});
    }
    void pointTo(const QRect& target){
        const QPoint local=parentWidget()->mapFromGlobal(target.topLeft());
        const QRect area(local,target.size());const bool right=area.right()+114<parentWidget()->width(),above=area.top()>=114;
        rotation_=right?(above?0:90):(above?270:180);
        QTransform orientation;orientation.translate(56,56);orientation.rotate(rotation_);orientation.translate(-56,-56);
        QTransform direction;direction.rotate(rotation_);
        const QPoint tip=(orientation.map(QPointF(12,101))+QPointF(8,8)).toPoint();
        const QPoint corner=rotation_==0?area.topRight():rotation_==90?area.bottomRight():rotation_==180?area.bottomLeft():area.topLeft();
        QPoint position=corner+direction.map(QPointF(5,-5)).toPoint()-tip;
        position.setX(std::clamp(position.x(),0,std::max(0,parentWidget()->width()-width())));
        position.setY(std::clamp(position.y(),0,std::max(0,parentWidget()->height()-height())));
        move(position);raise();update();
    }
};

class TourNextButton final : public QPushButton {
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const qreal offset=isDown()?2:0;
        const QRectF face(1+offset,1+offset,width()-5,height()-5);
        p.setPen(Qt::NoPen);p.setBrush(kbtheme::outline);p.drawRoundedRect(QRectF(3,3,width()-5,height()-5),12,12);
        const auto fill=isEnabled()?kbtheme::defaultAccent:kbtheme::lavender;
        p.setPen(QPen(kbtheme::outline,2));p.setBrush(fill);p.drawRoundedRect(face,12,12);
        QFont type("Geist");type.setPixelSize(13);type.setWeight(QFont::Bold);p.setFont(type);p.setPen(isEnabled()?kbtheme::readableOn(fill):kbtheme::muted);
        p.drawText(face.adjusted(10,0,-27,0),Qt::AlignCenter,text());
        const QPointF center(face.right()-16,face.center().y());QPainterPath arrow;arrow.moveTo(center+QPointF(-6,0));arrow.lineTo(center+QPointF(6,0));arrow.moveTo(center+QPointF(1,-5));arrow.lineTo(center+QPointF(6,0));arrow.lineTo(center+QPointF(1,5));p.setPen(QPen(p.pen().color(),2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.drawPath(arrow);
        if(property("keyboardFocus").toBool()){p.setPen(QPen(kbtheme::defaultAccent,2));p.setBrush(Qt::NoBrush);p.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),14,14);}
    }
public:
    explicit TourNextButton(QWidget* parent=nullptr):QPushButton(parent){}
    QSize sizeHint() const override {QFont type("Geist");type.setPixelSize(13);type.setWeight(QFont::Bold);return {std::max(82,QFontMetrics(type).horizontalAdvance(text())+48),42};}
};

// Owned tool window above the taskbar, without blocking or dimming the app.
class OnboardingPill final : public QWidget {
    QLabel *count_=nullptr,*title_=nullptr,*description_=nullptr;QPushButton *skip_=nullptr;TourNextButton* next_=nullptr;
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const QRectF face(1.5,1.5,width()-7,height()-7);
        p.setPen(Qt::NoPen);p.setBrush(kbtheme::outline);p.drawRoundedRect(face.translated(4,4),21,21);
        p.setPen(QPen(kbtheme::defaultAccent,3));p.setBrush(kbtheme::surface);p.drawRoundedRect(face,21,21);
    }
    void closeEvent(QCloseEvent* event) override {event->ignore();if(onSkip)onSkip();}
public:
    std::function<void()> onNext,onSkip;
    void setNextEnabled(bool enabled){next_->setEnabled(enabled);}
    explicit OnboardingPill(QWidget* parent):QWidget(parent,Qt::Tool|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint){
        setObjectName("tourPill");setAttribute(Qt::WA_TranslucentBackground);setAttribute(Qt::WA_ShowWithoutActivating);setWindowTitle("Azec");
        auto* row=new QHBoxLayout(this);row->setContentsMargins(18,14,20,18);row->setSpacing(14);
        count_=new QLabel;count_->setFixedSize(42,42);count_->setAlignment(Qt::AlignCenter);row->addWidget(count_);
        auto* instructions=new QVBoxLayout;instructions->setContentsMargins(0,0,0,0);instructions->setSpacing(2);
        title_=new QLabel;title_->setWordWrap(true);description_=new QLabel;description_->setWordWrap(true);instructions->addWidget(title_);instructions->addWidget(description_);row->addLayout(instructions,1);
        skip_=new QPushButton;skip_->setObjectName("tourSkip");row->addWidget(skip_);
        next_=new TourNextButton;next_->setObjectName("tourNext");next_->setMinimumSize(82,42);row->addWidget(next_);
        connect(skip_,&QPushButton::clicked,this,[this]{if(onSkip)onSkip();});connect(next_,&QPushButton::clicked,this,[this]{if(onNext)onNext();});
    }
    void present(int step,int total,const QString& title,const QString& description,int language,const QRect& available){
        count_->setText(QString("%1 / %2").arg(step+1).arg(total));title_->setText(title);description_->setText(description);
        skip_->setText(uiText(UiText::SkipTour,language));next_->setText(uiText(step==total-1?UiText::Done:UiText::Next,language));
        const auto color=[](const QColor& value){return value.name();};
        setStyleSheet(QString("QLabel{background:transparent;border:0;color:%1;}"
            "QPushButton#tourSkip{background:transparent;border:0;color:%2;font-size:12px;padding:8px 6px;}"
            "QPushButton#tourNext{background:%3;color:%4;border:2px solid %5;border-radius:12px;font-size:13px;font-weight:700;padding:0 12px;}"
            "QPushButton#tourNext:pressed{background:%6;}"
            "QPushButton#tourNext:disabled{background:%6;color:%2;}"
            "QPushButton#tourSkip[keyboardFocus=\"true\"],QPushButton#tourNext[keyboardFocus=\"true\"]{border:2px solid %3;}")
            .arg(color(kbtheme::ink),color(kbtheme::muted),color(kbtheme::defaultAccent),color(kbtheme::readableOn(kbtheme::defaultAccent)),color(kbtheme::outline),color(kbtheme::lavender)));
        count_->setStyleSheet(QString("background:%1;color:%2;border:2px solid %3;border-radius:12px;font-size:13px;font-weight:700;").arg(color(kbtheme::yellow),color(kbtheme::chipInk),color(kbtheme::outline)));
        title_->setStyleSheet("font-family:Fredoka;font-size:20px;font-weight:500;");
        description_->setStyleSheet(QString("font-size:13px;color:%1;").arg(color(kbtheme::muted)));
        setFixedWidth(std::min(620,available.width()-24));fitHeight();
        place(available);update();
    }
    void place(const QRect& available){
        const int desiredWidth=std::min(620,available.width()-24);
        if(width()!=desiredWidth){setFixedWidth(desiredWidth);fitHeight();}
        move(available.center().x()-width()/2,available.bottom()-height()-19);
    }
private:
    void fitHeight(){
        setMinimumHeight(80);setMaximumHeight(QWIDGETSIZE_MAX);layout()->invalidate();
        const int wrapped=layout()->totalHeightForWidth(width());
        setFixedHeight(std::max(80,wrapped>=0?wrapped:layout()->totalSizeHint().height()));
    }
};
