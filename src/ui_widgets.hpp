#pragma once
#include <QtWidgets>
#include <QSvgRenderer>
#include <functional>
#include <algorithm>

class Toggle final : public QAbstractButton {
    QVariantAnimation animation_;
    qreal position_=0;
    std::function<QColor()> accent_;
public:
    explicit Toggle(std::function<QColor()> accent,QWidget* parent=nullptr):QAbstractButton(parent),accent_(std::move(accent)) {
        setCheckable(true);setCursor(Qt::PointingHandCursor);setFixedSize(42,26);setFocusPolicy(Qt::StrongFocus);
        animation_.setDuration(120);animation_.setEasingCurve(QEasingCurve::OutCubic);
        connect(&animation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){position_=value.toReal();update();});
        connect(this,&QAbstractButton::toggled,this,[this](bool on){animation_.stop();if(!isVisible()){position_=on?1:0;update();return;}animation_.setStartValue(position_);animation_.setEndValue(on?1.0:0.0);animation_.start();});
    }
    void syncChecked(bool checked){QSignalBlocker blocked(this);animation_.stop();position_=checked?1:0;setChecked(checked);update();}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const qreal position=animation_.state()==QAbstractAnimation::Running?position_:(isChecked()?1.0:0.0);
        const QColor off("#424143"),on=accent_();
        QColor track=QColor::fromRgbF(off.redF()+(on.redF()-off.redF())*position,off.greenF()+(on.greenF()-off.greenF())*position,off.blueF()+(on.blueF()-off.blueF())*position);
        if(!isEnabled())p.setOpacity(.38);
        p.setPen(Qt::NoPen);p.setBrush(track);p.drawRoundedRect(QRectF(1,3,40,20),10,10);
        p.setBrush(QColor(isChecked()?"#173b32":"#252426"));p.drawEllipse(QPointF(11+20*position,13),7,7);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setPen(QPen(accent_(),1));p.drawLine(12,25,30,25);}
    }
};

inline QPixmap operatorPixmap(const QString& name,int size=32,qreal ratio=1) {
    QPixmap pixmap(QSize(size,size)*ratio);pixmap.setDevicePixelRatio(ratio);pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);painter.setRenderHint(QPainter::Antialiasing);
    QSvgRenderer renderer(":/operators/"+name.toLower()+".svg");
    if(renderer.isValid())renderer.render(&painter,QRectF(0,0,size,size));
    return pixmap;
}
inline QIcon operatorIcon(const QString& name) {QIcon icon;for(int scale:{1,2,3})icon.addPixmap(operatorPixmap(name,40,scale));return icon;}

class OperatorDelegate final : public QStyledItemDelegate {
    std::function<QColor()> accent_;
public:
    explicit OperatorDelegate(std::function<QColor()> accent,QObject* parent):QStyledItemDelegate(parent),accent_(std::move(accent)){}
    QSize sizeHint(const QStyleOptionViewItem&,const QModelIndex&) const override{return {200,56};}
    void paint(QPainter* p,const QStyleOptionViewItem& option,const QModelIndex& index) const override {
        p->save();p->setRenderHint(QPainter::Antialiasing);p->setRenderHint(QPainter::TextAntialiasing);const QRect r=option.rect;
        const bool selected=option.state&QStyle::State_Selected,hovered=option.state&QStyle::State_MouseOver;
        if(selected||hovered){p->setPen(Qt::NoPen);p->setBrush(QColor(selected?"#2b2a2c":"#222122"));p->drawRoundedRect(r.adjusted(6,2,-6,-2),7,7);}
        if(selected){p->setPen(Qt::NoPen);p->setBrush(accent_());p->drawRoundedRect(QRectF(r.left()+6,r.top()+12,3,32),1.5,1.5);}
        qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).paint(p,QRect(r.left()+15,r.top()+8,40,40));
        QFont f=option.font;f.setFamily("Segoe UI");f.setHintingPreference(QFont::PreferNoHinting);f.setStyleStrategy(QFont::PreferAntialias);f.setPixelSize(14);f.setWeight(selected?QFont::DemiBold:QFont::Normal);p->setFont(f);p->setPen(QColor(selected?"#eeedee":"#bdbbbf"));
        p->drawText(QRect(r.left()+68,r.top(),r.width()-78,r.height()),Qt::AlignVCenter|Qt::AlignLeft,index.data().toString());p->restore();
    }
};

class FavoritePopup final : public QFrame {
    QEventLoop loop_;bool chosen_=false;
public:
    FavoritePopup(const QString& text,QWidget* parent):QFrame(parent,Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint){
        setObjectName("favoritePopup");setFixedSize(178,52);auto* layout=new QVBoxLayout(this);layout->setContentsMargins(7,7,7,7);
        auto* button=new QPushButton(QIcon(":/ui/star.svg"),text);button->setObjectName("toggleFavorite");button->setIconSize(QSize(14,14));button->setCursor(Qt::PointingHandCursor);button->setFocusPolicy(Qt::NoFocus);layout->addWidget(button);
        connect(button,&QPushButton::clicked,this,[this]{chosen_=true;close();});
    }
    bool execAt(QPoint position){const auto available=screen()->availableGeometry();position.setX(std::clamp(position.x(),available.left(),available.right()-width()+1));position.setY(std::clamp(position.y(),available.top(),available.bottom()-height()+1));move(position);show();raise();loop_.exec();return chosen_;}
protected:
    void hideEvent(QHideEvent* event) override {loop_.quit();QFrame::hideEvent(event);}
    void keyPressEvent(QKeyEvent* event) override {if(event->key()==Qt::Key_Escape)close();else QFrame::keyPressEvent(event);}
};

class BrandLabel final : public QLabel {
    QTimer timer_;int phase_=0;
public:
    BrandLabel(){setText("[ KING BOSS ]");setObjectName("brand");setAccessibleName("King Boss");timer_.setSingleShot(true);timer_.start(7800);
        connect(&timer_,&QTimer::timeout,this,[this]{if(phase_==0){if(isVisible()&&window()->isActiveWindow())phase_=1;}else if(++phase_>3)phase_=0;update();timer_.start(phase_?35:QRandomGenerator::global()->bounded(7000,12000));});}
protected:
    void paintEvent(QPaintEvent* event) override {
        QLabel::paintEvent(event);if(!phase_)return;
        QPainter painter(this);painter.setFont(font());painter.setPen(QColor(125,171,158,95));const int y=height()/2+(phase_-2)*4;
        painter.setClipRect(0,y,width(),2);painter.drawText(rect().translated(phase_==2?-1:1,0),alignment(),text());
    }
};

// The lists expand inside one sidebar scroll area; wheel events go to that area.
class OperatorList final : public QListWidget {
    QScrollArea* area_;
public:explicit OperatorList(QScrollArea* area):area_(area){}
protected:
    void wheelEvent(QWheelEvent* event) override {
        QWheelEvent forwarded(QPointF(area_->viewport()->mapFromGlobal(event->globalPosition().toPoint())),event->globalPosition(),event->pixelDelta(),event->angleDelta(),event->buttons(),event->modifiers(),event->phase(),event->inverted());
        QApplication::sendEvent(area_->viewport(),&forwarded);event->setAccepted(forwarded.isAccepted());
    }
};

// Paint the identity directly: nested graphics effects would disturb the panel shadow.
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
        // Several translucent contours make a soft, low shadow without an outline.
        for(int spread=11;spread>=1;--spread){
            painter.setBrush(QColor(0,0,0,spread>6?2:4));
            painter.drawRoundedRect(surface.adjusted(-spread*.65,-spread*.45,spread*.65,spread*.65).translated(0,3),18+spread*.6,18+spread*.6);
        }
        painter.setBrush(QColor("#201f20"));painter.drawRoundedRect(surface,18,18);
        QLinearGradient edge(surface.topLeft(),surface.bottomLeft());edge.setColorAt(0,QColor(255,255,255,13));edge.setColorAt(.45,QColor(255,255,255,3));edge.setColorAt(1,QColor(255,255,255,0));
        painter.setBrush(Qt::NoBrush);painter.setPen(QPen(edge,.7));painter.drawRoundedRect(surface.adjusted(.35,.35,-.35,-.35),18,18);
    }
};

class PageSnapshot final : public QWidget {
    QPixmap image_;qreal opacity_=1;
public:
    PageSnapshot(QPixmap image,QWidget* parent):QWidget(parent),image_(std::move(image)){setAttribute(Qt::WA_TransparentForMouseEvents);setObjectName("pageTransition");}
    void setOpacity(qreal opacity){opacity_=opacity;update();}
protected:
    void paintEvent(QPaintEvent*) override {QPainter painter(this);painter.setOpacity(opacity_);painter.drawPixmap(0,0,image_);}
};

class PageStack final : public QStackedWidget {
    QVariantAnimation fade_;PageSnapshot* snapshot_=nullptr;
public:
    explicit PageStack(QWidget* parent=nullptr):QStackedWidget(parent){
        fade_.setDuration(180);fade_.setStartValue(1.0);fade_.setEndValue(0.0);fade_.setEasingCurve(QEasingCurve::InOutCubic);
        connect(&fade_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){if(snapshot_)snapshot_->setOpacity(value.toReal());});
        connect(&fade_,&QVariantAnimation::finished,this,[this]{settle();});
    }
    void settle(){fade_.stop();delete snapshot_;snapshot_=nullptr;}
    bool transitioning() const{return snapshot_!=nullptr;}
    void switchTo(QWidget* page){
        if(page==currentWidget())return;
        QPixmap previous;
        if(isVisible()){
            previous=QPixmap(size()*devicePixelRatioF());previous.setDevicePixelRatio(devicePixelRatioF());previous.fill(QColor("#201f20"));
            QPainter painter(&previous);render(&painter,QPoint(),QRegion(),QWidget::DrawChildren);
        }
        settle();setCurrentWidget(page);
        if(previous.isNull())return;
        snapshot_=new PageSnapshot(std::move(previous),this);snapshot_->setGeometry(rect());snapshot_->show();snapshot_->raise();fade_.start();
    }
protected:
    void resizeEvent(QResizeEvent* event) override {settle();QStackedWidget::resizeEvent(event);}
    void hideEvent(QHideEvent* event) override {settle();QStackedWidget::hideEvent(event);}
};

class FadingScrollBar final : public QScrollBar {
    QTimer idle_;
    QVariantAnimation fade_;
    qreal opacity_=0;
    void fadeTo(qreal target){
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
        area->viewport()->installEventFilter(bar);return bar;
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
class Section final : public QFrame {
public:
    QVBoxLayout* body;QWidget* content;QPushButton* heading=nullptr;
    explicit Section(const QString& title,bool collapsible,bool expanded,QWidget* parent=nullptr):QFrame(parent) {
        setObjectName("settingCard");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
        auto* header=new QWidget;auto* row=new QHBoxLayout(header);row->setContentsMargins(0,0,0,0);header->setMinimumHeight(30);
        if(collapsible){heading=new QPushButton(title);heading->setObjectName("sectionHeading");heading->setCursor(Qt::PointingHandCursor);row->addWidget(heading,1);
            auto* arrow=new QLabel(expanded?QString::fromUtf8("⌄"):QString::fromUtf8("›"));arrow->setObjectName("sectionArrow");row->addWidget(arrow);
            layout->addWidget(header);content=new QWidget;body=new QVBoxLayout(content);body->setContentsMargins(0,0,0,0);body->setSpacing(6);content->setVisible(expanded);layout->addWidget(content);if(!expanded)layout->setContentsMargins(18,0,18,0);
            connect(heading,&QPushButton::clicked,this,[=]{const bool open=!content->isVisible();content->setVisible(open);arrow->setText(open?QString::fromUtf8("⌄"):QString::fromUtf8("›"));layout->setContentsMargins(18,0,18,open?12:0);});
        }else{auto* label=new QLabel(title);label->setObjectName("cardTitle");row->addWidget(label,1);layout->addWidget(header);content=new QWidget;body=new QVBoxLayout(content);body->setContentsMargins(0,0,0,0);body->setSpacing(6);layout->addWidget(content);}
    }
};
