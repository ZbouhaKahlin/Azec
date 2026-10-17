#pragma once
#include <QtWidgets>
#include <QSvgRenderer>
#include <functional>
#include <algorithm>

inline constexpr int operatorRowHeight=52;
inline constexpr int operatorIconSize=44;

inline QColor mixColor(const QColor& from,const QColor& to,qreal amount){
    return QColor::fromRgbF(from.redF()+(to.redF()-from.redF())*amount,from.greenF()+(to.greenF()-from.greenF())*amount,from.blueF()+(to.blueF()-from.blueF())*amount);
}

// The hit target stays stationary; only the vector ink changes on hover and save.
class IconButton final : public QPushButton {
    QSvgRenderer renderer_;
    std::function<QColor()> accent_;
    QVariantAnimation hover_;
    qreal hoverAmount_=0;
    QPixmap mask_;
    qreal maskRatio_=0;
    void hoverTo(qreal amount){hover_.stop();hover_.setStartValue(hoverAmount_);hover_.setEndValue(amount);hover_.start();}
public:
    IconButton(const QString& path,std::function<QColor()> accent,QWidget* parent=nullptr):QPushButton(parent),renderer_(path),accent_(std::move(accent)){
        setIcon(QIcon(path));setIconSize(QSize(20,20));setFixedSize(34,34);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);
        hover_.setDuration(140);hover_.setEasingCurve(QEasingCurve::OutCubic);
        connect(&hover_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){hoverAmount_=value.toReal();update();});
    }
protected:
    void enterEvent(QEnterEvent* event) override {hoverTo(1);QPushButton::enterEvent(event);}
    void leaveEvent(QEvent* event) override {hoverTo(0);QPushButton::leaveEvent(event);}
    void paintEvent(QPaintEvent*) override {
        const qreal ratio=devicePixelRatioF();
        if(mask_.isNull()||maskRatio_!=ratio){maskRatio_=ratio;mask_=QPixmap(QSize(20,20)*ratio);mask_.setDevicePixelRatio(ratio);mask_.fill(Qt::transparent);QPainter maskPainter(&mask_);renderer_.render(&maskPainter,QRectF(0,0,20,20));}
        const auto state=property("saveState").toString();
        QColor color=state=="dirty"||property("activePage").toBool()?accent_():state=="error"?QColor("#ff8a91"):QColor("#909099");
        color=mixColor(color,objectName()=="closeControl"?QColor("#e87980"):QColor("#bdbdc7"),hoverAmount_);
        if(isDown())color=mixColor(color,QColor("#60606b"),.3);
        QPixmap ink=mask_;QPainter tint(&ink);tint.setCompositionMode(QPainter::CompositionMode_SourceIn);tint.fillRect(ink.rect(),color);tint.end();
        QPainter painter(this);if(!isEnabled())painter.setOpacity(.35);painter.drawPixmap(QPointF((width()-20)/2.0,(height()-20)/2.0),ink);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setPen(QPen(accent_(),1));painter.drawLine(12,height()-3,width()-12,height()-3);}
    }
};

class Toggle final : public QAbstractButton {
    QVariantAnimation animation_;
    qreal position_=0;
    std::function<QColor()> accent_;
public:
    explicit Toggle(std::function<QColor()> accent,QWidget* parent=nullptr):QAbstractButton(parent),accent_(std::move(accent)) {
        setCheckable(true);setCursor(Qt::PointingHandCursor);setFixedSize(42,26);setFocusPolicy(Qt::StrongFocus);
        animation_.setDuration(160);animation_.setEasingCurve(QEasingCurve::OutCubic);
        connect(&animation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){position_=value.toReal();update();});
        connect(this,&QAbstractButton::toggled,this,[this](bool on){animation_.stop();if(!isVisible()){position_=on?1:0;update();return;}animation_.setStartValue(position_);animation_.setEndValue(on?1.0:0.0);animation_.start();});
    }
    void syncChecked(bool checked){QSignalBlocker blocked(this);animation_.stop();position_=checked?1:0;setChecked(checked);update();}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const qreal position=animation_.state()==QAbstractAnimation::Running?position_:(isChecked()?1.0:0.0);
        const QColor track=mixColor(QColor("#35353c"),mixColor(QColor("#29292f"),accent_(),.24),position);
        if(!isEnabled())p.setOpacity(.38);
        p.setPen(Qt::NoPen);p.setBrush(track);p.drawRoundedRect(QRectF(4,5,34,16),8,8);
        p.setBrush(mixColor(QColor("#71717d"),accent_(),position));p.drawEllipse(QPointF(12+18*position,13),6,6);
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
inline QIcon operatorIcon(const QString& name) {QIcon icon;for(int scale:{1,2,3})icon.addPixmap(operatorPixmap(name,operatorIconSize,scale));return icon;}

class OperatorDelegate final : public QStyledItemDelegate {
    std::function<QColor()> accent_;
public:
    explicit OperatorDelegate(std::function<QColor()> accent,QObject* parent):QStyledItemDelegate(parent),accent_(std::move(accent)){}
    QSize sizeHint(const QStyleOptionViewItem&,const QModelIndex&) const override{return {200,operatorRowHeight};}
    void paint(QPainter* p,const QStyleOptionViewItem& option,const QModelIndex& index) const override {
        p->save();p->setRenderHint(QPainter::Antialiasing);p->setRenderHint(QPainter::TextAntialiasing);const QRect r=option.rect;
        const bool selected=option.state&QStyle::State_Selected,hovered=option.state&QStyle::State_MouseOver;
        if(selected||hovered){p->setPen(Qt::NoPen);p->setBrush(QColor(selected?"#26262b":"#1d1d21"));p->drawRoundedRect(r.adjusted(4,2,-4,-2),12,12);}
        if(selected){p->setPen(Qt::NoPen);p->setBrush(accent_());p->drawRoundedRect(QRectF(r.left()+4,r.top()+16,2,20),1,1);}
        qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).paint(p,QRect(r.left()+12,r.top()+4,operatorIconSize,operatorIconSize));
        QFont f=option.font;f.setFamily("Inter");f.setStyleStrategy(QFont::PreferAntialias);f.setPixelSize(16);f.setWeight(selected?QFont::Medium:QFont::Normal);p->setFont(f);p->setPen(QColor(selected?"#f2f2f5":"#bdbdc6"));
        const QRect textRect(r.left()+65,r.top(),r.width()-73,r.height());
        p->drawText(textRect,Qt::AlignVCenter|Qt::AlignLeft,QFontMetrics(f).elidedText(index.data().toString(),Qt::ElideRight,textRect.width()));p->restore();
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
    static QString artwork(){
        const QMap<QChar,QStringList> letters={
            {'K',{"#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#"}},
            {'I',{"#####","..#..","..#..","..#..","..#..","..#..","#####"}},
            {'N',{"#...#","##..#","##..#","#.#.#","#..##","#..##","#...#"}},
            {'G',{".###.","#...#","#....","#.###","#...#","#...#",".###."}},
            {'B',{"####.","#...#","#...#","####.","#...#","#...#","####."}},
            {'O',{".###.","#...#","#...#","#...#","#...#","#...#",".###."}},
            {'S',{".####","#....","#....",".###.","....#","....#","####."}}
        };
        QStringList rows;for(int row=0;row<7;++row){QString line;for(QChar letter:QString("KING BOSS")){if(letter==' '){line+="  ";continue;}const auto pattern=letters.value(letter)[row];for(int col=0;col<pattern.size();++col)line+=pattern[col]=='#'?QChar(row==0||row==6?'+':'#'):QChar(' ');line+=' ';}rows<<line.chopped(1);}return rows.join('\n');
    }
public:
    BrandLabel(){setText(artwork());setObjectName("brand");setAccessibleName("King Boss");setFixedSize(172,36);setTextFormat(Qt::PlainText);}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);painter.setRenderHint(QPainter::TextAntialiasing);QFont mono("Consolas");mono.setPixelSize(6);painter.setFont(mono);
        const auto rows=text().split('\n');const qreal advance=QFontMetricsF(mono).horizontalAdvance('#'),lineHeight=4.6;
        const qreal left=(width()-rows.front().size()*advance)/2,top=(height()-7*lineHeight)/2;
        for(int row=0;row<rows.size();++row){painter.setPen(mixColor(QColor("#ff777e"),QColor("#ff5b63"),row/6.0));painter.drawText(QPointF(left,top+(row+1)*lineHeight),rows[row]);}
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
            painter.drawRoundedRect(surface.adjusted(-spread*.65,-spread*.45,spread*.65,spread*.65).translated(0,3),24+spread*.6,24+spread*.6);
        }
        painter.setBrush(QColor("#1c1c1f"));painter.drawRoundedRect(surface,24,24);
        QLinearGradient edge(surface.topLeft(),surface.bottomLeft());edge.setColorAt(0,QColor(255,255,255,13));edge.setColorAt(.45,QColor(255,255,255,3));edge.setColorAt(1,QColor(255,255,255,0));
        painter.setBrush(Qt::NoBrush);painter.setPen(QPen(edge,.7));painter.drawRoundedRect(surface.adjusted(.35,.35,-.35,-.35),24,24);
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
            previous=QPixmap(size()*devicePixelRatioF());previous.setDevicePixelRatio(devicePixelRatioF());previous.fill(QColor("#1c1c1f"));
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
