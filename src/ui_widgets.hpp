#pragma once
#include <QtWidgets>
#include <QSvgRenderer>
#include <functional>
#include <algorithm>
#include <cmath>

inline constexpr int operatorRowHeight=52;
inline constexpr int operatorIconSize=44;

inline QColor mixColor(const QColor& from,const QColor& to,qreal amount){
    return QColor::fromRgbF(from.redF()+(to.redF()-from.redF())*amount,from.greenF()+(to.greenF()-from.greenF())*amount,from.blueF()+(to.blueF()-from.blueF())*amount);
}

// Changing QVariantAnimation's duration/endpoints can emit valueChanged even
// while stopped. Suppress those setup values so they cannot resize the page or
// overwrite the current position before a reversal starts.
inline void retargetAnimation(QVariantAnimation& animation,qreal from,qreal to,int duration){
    {QSignalBlocker blocked(&animation);animation.stop();animation.setDuration(duration);animation.setStartValue(from);animation.setEndValue(to);animation.setCurrentTime(0);}
    animation.start();
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
        const bool unsaved=state=="dirty"||property("unsaved").toBool();
        QColor color=unsaved||property("activePage").toBool()?accent_():state=="error"?QColor("#ff6b6e"):QColor("#8a8a8a");
        const QColor hoverColor=unsaved?mixColor(accent_(),Qt::white,.12):objectName()=="closeControl"?QColor("#ff6b6e"):QColor("#b4b4b4");
        color=mixColor(color,hoverColor,hoverAmount_);
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
        // A light spring on the knob travel; colours stay clamped while painting.
        QEasingCurve spring(QEasingCurve::OutBack);spring.setOvershoot(1.05);
        animation_.setDuration(240);animation_.setEasingCurve(spring);
        connect(&animation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){position_=value.toReal();update();});
        connect(this,&QAbstractButton::toggled,this,[this](bool on){animation_.stop();if(!isVisible()){position_=on?1:0;update();return;}animation_.setStartValue(position_);animation_.setEndValue(on?1.0:0.0);animation_.start();});
    }
    void syncChecked(bool checked){if(isChecked()==checked)return;QSignalBlocker blocked(this);animation_.stop();position_=checked?1:0;setChecked(checked);update();}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const qreal position=animation_.state()==QAbstractAnimation::Running?position_:(isChecked()?1.0:0.0);
        const qreal clamped=std::clamp(position,0.0,1.0);
        if(!isEnabled())p.setOpacity(.38);
        const QColor accent=accent_();
        const QRectF track(4,5,34,16);
        // Flat recessed well when off; the accent fill fades in over it when on.
        p.setPen(QPen(QColor(255,255,255,22),1));p.setBrush(QColor(0,0,0,66));p.drawRoundedRect(track.adjusted(.5,.5,-.5,-.5),8,8);
        if(clamped>0.001){
            QColor fill=accent;fill.setAlphaF(clamped);
            p.setPen(QPen(QColor(255,255,255,qRound(153*clamped)),1));p.setBrush(fill);p.drawRoundedRect(track.adjusted(.5,.5,-.5,-.5),8,8);
        }
        // Knob: flat ink-muted off, on-solid on.
        const qreal knobX=11.5+19.0*position;
        p.setPen(Qt::NoPen);p.setBrush(mixColor(QColor("#b4b4b4"),QColor("#141414"),clamped));p.drawEllipse(QPointF(knobX,13),6.5,6.5);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setPen(QPen(accent,1));p.drawLine(12,25,30,25);}
    }
};

// Keep the content at its natural size while its viewport opens/closes. Reversals
// start at the displayed progress, and an empty viewport reserves no layout gap.
class ExpandingContent final : public QWidget {
    QWidget* content_;
    QVariantAnimation animation_;
    QPixmap snapshot_;
    qreal progress_=0;
    int fullHeight_=0,duration_;
    bool expanded_=false,initialized_=false;
    int naturalHeight() const {
        return std::max(0,content_->hasHeightForWidth()?content_->heightForWidth(width()):content_->sizeHint().height());
    }
    void apply(){
        setFixedHeight(qRound(fullHeight_*progress_));update();
    }
    void sizeContent(){fullHeight_=naturalHeight();content_->setGeometry(0,0,width(),fullHeight_);if(content_->layout())content_->layout()->activate();}
    void settle(){animation_.stop();progress_=expanded_?1:0;sizeContent();snapshot_=QPixmap();content_->setEnabled(expanded_);content_->setVisible(expanded_);apply();}
public:
    explicit ExpandingContent(QWidget* content,int duration=220,QWidget* parent=nullptr):QWidget(parent),content_(content),duration_(duration){
        content_->setParent(this);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setFixedHeight(0);
        animation_.setEasingCurve(QEasingCurve::InOutSine);
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
        // Freeze one correctly laid-out frame. Fading/resizing live child widgets
        // through an opacity effect can stretch controls and repaint clipped QR
        // content repeatedly; only this cached surface is painted during motion.
        content_->setEnabled(true);sizeContent();
        snapshot_=QPixmap(QSize(width(),fullHeight_)*devicePixelRatioF());snapshot_.setDevicePixelRatio(devicePixelRatioF());snapshot_.fill(Qt::transparent);
        {QPainter painter(&snapshot_);content_->render(&painter,QPoint(),QRegion(),QWidget::DrawChildren);}
        content_->hide();
        retargetAnimation(animation_,progress_,expanded?1.0:0.0,std::max(120,qRound(duration_*std::abs((expanded?1.0:0.0)-progress_))));
    }
    QSize sizeHint() const override{return {content_->sizeHint().width(),qRound(fullHeight_*progress_)};}
protected:
    void paintEvent(QPaintEvent*) override{if(!snapshot_.isNull()){QPainter painter(this);painter.setOpacity(std::sin(progress_*1.5707963267948966));painter.drawPixmap(0,0,snapshot_);}}
    void resizeEvent(QResizeEvent* event) override{QWidget::resizeEvent(event);if(event->oldSize().width()!=width())settle();}
    void hideEvent(QHideEvent* event) override{settle();QWidget::hideEvent(event);}
};

class WeaponButton final : public QPushButton {
    QVariantAnimation selection_;
    qreal amount_=0;
    bool chosen_=false,initialized_=false;
    std::function<QColor()> accent_;
public:
    explicit WeaponButton(std::function<QColor()> accent):accent_(std::move(accent)){
        setObjectName("weaponChoice");setCursor(Qt::PointingHandCursor);
        selection_.setEasingCurve(QEasingCurve::OutCubic);
        connect(&selection_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){amount_=value.toReal();update();});
    }
    void setChosen(bool chosen){
        if(initialized_&&chosen_==chosen)return;
        const bool animate=initialized_&&isVisible();initialized_=true;chosen_=chosen;setProperty("chosen",chosen);selection_.stop();
        if(!animate){amount_=chosen?1:0;update();return;}
        selection_.setDuration(160);selection_.setStartValue(amount_);selection_.setEndValue(chosen?1.0:0.0);selection_.start();
    }
protected:
    void paintEvent(QPaintEvent*) override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        const QRectF pill=QRectF(rect()).adjusted(.5,.5,-.5,-.5);
        // The chosen segment is the Atelier selection pill: flat accent tint,
        // accent border, one bevel line on the top edge.
        if(amount_>0.002){
            const QColor accent=accent_();
            painter.setPen(Qt::NoPen);painter.setBrush(QColor(accent.red(),accent.green(),accent.blue(),qRound(31*amount_)));painter.drawRoundedRect(pill,8,8);
            painter.setPen(QPen(QColor(accent.red(),accent.green(),accent.blue(),qRound(178*amount_)),1));painter.setBrush(Qt::NoBrush);painter.drawRoundedRect(pill,8,8);
            painter.setPen(QPen(QColor(255,255,255,qRound(31*amount_)),1));painter.drawLine(QPointF(pill.left()+9,pill.top()+1.5),QPointF(pill.right()-9,pill.top()+1.5));
        }
        QColor ink=mixColor(QColor("#8a8a8a"),QColor("#e6e6e6"),amount_);if(isDown())ink=mixColor(ink,QColor("#000000"),.15);
        painter.setFont(font());painter.setPen(ink);painter.drawText(rect(),Qt::AlignCenter,text());
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setBrush(Qt::NoBrush);painter.setPen(QColor("#8a8a8a"));painter.drawRoundedRect(pill,8,8);}
    }
    void hideEvent(QHideEvent* event) override{selection_.stop();amount_=chosen_?1:0;QPushButton::hideEvent(event);}
};

inline QPixmap operatorPixmap(const QString& name,int size=32,qreal ratio=1) {
    const QString key="kingboss/operator/"+name.toLower()+"/"+QString::number(size)+"/"+QString::number(ratio,'g',12);
    QPixmap cached;if(QPixmapCache::find(key,&cached))return cached;
    QPixmap pixmap(QSize(size,size)*ratio);pixmap.setDevicePixelRatio(ratio);pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);painter.setRenderHint(QPainter::Antialiasing);
    QSvgRenderer renderer(":/operators/"+name.toLower()+".svg");
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
        const bool selected=option.state&QStyle::State_Selected,hovered=option.state&QStyle::State_MouseOver;
        if(selected||hovered){p->setPen(Qt::NoPen);p->setBrush(QColor(255,255,255,selected?28:20));p->drawRoundedRect(r.adjusted(4,2,-4,-2),8,8);}
        if(selected){p->setPen(QPen(QColor(255,255,255,41),1));p->setBrush(Qt::NoBrush);p->drawRoundedRect(QRectF(r.adjusted(4,2,-4,-2)).adjusted(.5,.5,-.5,-.5),8,8);}
        qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).paint(p,QRect(r.left()+12,r.top()+4,operatorIconSize,operatorIconSize));
        QFont f=option.font;f.setFamily("Geist");f.setStyleStrategy(QFont::PreferAntialias);f.setPixelSize(14);f.setWeight(QFont::Medium);p->setFont(f);p->setPen(QColor(selected?"#e6e6e6":"#b4b4b4"));
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
        painter.setPen(QColor("#38ccff"));
        for(int row=0;row<rows.size();++row)painter.drawText(QPointF(left,top+(row+1)*lineHeight),rows[row]);
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
            painter.setBrush(QColor(0,0,0,spread>6?3:5));
            painter.drawRoundedRect(surface.adjusted(-spread*.65,-spread*.45,spread*.65,spread*.65).translated(0,3),24+spread*.6,24+spread*.6);
        }
        painter.setBrush(QColor(255,255,255,12));painter.drawRoundedRect(surface,24,24);
        painter.setBrush(Qt::NoBrush);painter.setPen(QPen(QColor(255,255,255,20),1));painter.drawRoundedRect(surface.adjusted(.45,.45,-.45,-.45),24,24);
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
            previous=QPixmap(size()*devicePixelRatioF());previous.setDevicePixelRatio(devicePixelRatioF());previous.fill(QColor("#1f1f1f"));
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

// A soft edge over a scroll viewport: rows dissolve into the surface instead
// of being cut, matching the reference's clipped-list fade. Mirrored variants
// cover the top edge when the list is scrolled away from its start.
class ScrollFade final : public QWidget {
    QAbstractScrollArea* area_;
    QColor color_;
    bool top_;
    int strip_=46;
public:
    ScrollFade(QAbstractScrollArea* area,const QColor& color,bool top=false,QWidget* parent=nullptr):QWidget(parent?parent:area),area_(area),color_(color),top_(top){
        setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);
        area_->viewport()->installEventFilter(this);
        if(auto* bar=area_->verticalScrollBar()){connect(bar,&QScrollBar::valueChanged,this,[this]{refresh();});connect(bar,&QScrollBar::rangeChanged,this,[this]{refresh();});}
        refresh();
    }
    void refresh(){
        auto* bar=area_->verticalScrollBar();const QRect viewport=area_->viewport()->geometry();
        const bool overflow=bar&&bar->maximum()>bar->minimum();
        const bool active=overflow&&(top_?bar->value()>bar->minimum():bar->value()<bar->maximum());
        setVisible(active);
        if(!active)return;
        if(top_){const int bottom=std::min(viewport.bottom(),viewport.top()+strip_-1);setGeometry(viewport.left(),viewport.top(),viewport.width(),bottom-viewport.top()+1);}
        else{const int top=std::max(viewport.top(),viewport.bottom()-strip_+1);setGeometry(viewport.left(),top,viewport.width(),viewport.bottom()-top+1);}
        raise();
    }
protected:
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()==QEvent::Resize&&object==area_->viewport())refresh();
        return QWidget::eventFilter(object,event);
    }
    void showEvent(QShowEvent* event) override {QWidget::showEvent(event);refresh();}
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);QLinearGradient fade(0,0,0,height());
        QColor clear=color_;clear.setAlpha(0);
        QColor soft=color_;soft.setAlpha(150);
        if(top_){fade.setColorAt(0.0,color_);fade.setColorAt(0.25,soft);fade.setColorAt(0.65,clear);fade.setColorAt(1.0,clear);}
        else{fade.setColorAt(0.0,clear);fade.setColorAt(0.35,clear);fade.setColorAt(0.75,soft);fade.setColorAt(1.0,color_);}
        painter.fillRect(rect(),fade);
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
        setObjectName("settingCard");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(12,11,12,12);layout->setSpacing(0);
        auto* header=new QWidget;auto* row=new QHBoxLayout(header);row->setContentsMargins(2,0,2,0);header->setMinimumHeight(24);
        if(collapsible){heading=new QPushButton(title);heading->setObjectName("sectionHeading");heading->setCursor(Qt::PointingHandCursor);row->addWidget(heading,1);
            auto* arrow=new QLabel(expanded?QString::fromUtf8("⌄"):QString::fromUtf8("›"));arrow->setObjectName("sectionArrow");row->addWidget(arrow);
            layout->addWidget(header);content=new QWidget;body=new QVBoxLayout(content);body->setContentsMargins(0,8,0,0);body->setSpacing(8);content->setVisible(expanded);layout->addWidget(content);if(!expanded)layout->setContentsMargins(12,4,12,4);
            connect(heading,&QPushButton::clicked,this,[=]{const bool open=!content->isVisible();content->setVisible(open);arrow->setText(open?QString::fromUtf8("⌄"):QString::fromUtf8("›"));layout->setContentsMargins(12,open?11:4,12,open?12:4);});
        }else{auto* label=new QLabel(title);label->setObjectName("panelTitle");row->addWidget(label,1);layout->addWidget(header);content=new QWidget;body=new QVBoxLayout(content);body->setContentsMargins(0,8,0,0);body->setSpacing(8);layout->addWidget(content);}
    }
};
