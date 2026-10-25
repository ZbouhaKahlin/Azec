#pragma once
#include "ui_widgets.hpp"
#include "strings.hpp"
#include <array>

// A short-lived foreground, on the main HWND. Only visible roster rows are
// captured; all snapshots and blur frames disappear when the handoff finishes.
class StartupView final : public QWidget {
public:
    static constexpr int loadingDuration=1440,expansionDuration=720;
    struct PieceSpec {QRect rect;int delay=0;int duration=540;bool header=false;};
private:
    struct Piece {PieceSpec spec;std::array<QPixmap,3> frames;};
    QVector<Piece> pieces_;
    QRect compact_,expanded_;QRectF brand_target_;
    QStringList messages_;QVariantAnimation timeline_;
    int time_=0,total_=0;std::function<void(const QRect&)> geometry_;std::function<void()> finished_;
#ifdef AZEC_UI_TESTS
    struct FrameTiming {int time;double at,work;};
    QElapsedTimer timing_clock_;QVector<FrameTiming> update_timings_,paint_timings_;
#endif
    static qreal fraction(qreal value){return std::clamp(value,qreal(0),qreal(1));}
    static qreal smooth(qreal value){const auto t=fraction(value);return t*t*(3-2*t);}
    qreal expansion() const{const auto t=fraction(qreal(time_-loadingDuration)/expansionDuration);return t*t*t*(10+t*(-15+6*t));}
    static QRectF blend(const QRectF& from,const QRectF& to,qreal t){return {from.x()+(to.x()-from.x())*t,from.y()+(to.y()-from.y())*t,from.width()+(to.width()-from.width())*t,from.height()+(to.height()-from.height())*t};}
    static QPixmap blur(const QPixmap& source,qreal radius){
        QGraphicsScene scene;auto* item=scene.addPixmap(source);auto* effect=new QGraphicsBlurEffect;
        effect->setBlurHints(QGraphicsBlurEffect::QualityHint);effect->setBlurRadius(radius);item->setGraphicsEffect(effect);
        QPixmap result(source.size());result.setDevicePixelRatio(source.devicePixelRatio());result.fill(Qt::transparent);
        QPainter painter(&result);const QRectF bounds(QPointF(),source.deviceIndependentSize());scene.render(&painter,bounds,bounds);return result;
    }
    QRectF compactBrand() const {return {compact_.width()/2.0-94,43,188,66};}
    void advance(int time){
#ifdef AZEC_UI_TESTS
        const auto began=timing_clock_.nsecsElapsed();
#endif
        time_=time;const qreal t=expansion();
        if(time>=loadingDuration)geometry_(blend(compact_,expanded_,t).toRect());
        update();
#ifdef AZEC_UI_TESTS
        update_timings_.append({time_,began/1e6,(timing_clock_.nsecsElapsed()-began)/1e6});
#endif
    }
public:
    StartupView(const QPixmap& frame,const QVector<PieceSpec>& specs,const QRect& compact,const QRect& expanded,const QRectF& brand,int language,QWidget* parent,
                std::function<void(const QRect&)> geometry,std::function<void()> finished)
        :QWidget(parent),compact_(compact),expanded_(expanded),brand_target_(brand),geometry_(std::move(geometry)),finished_(std::move(finished)){
        setObjectName("startupPresentation");setAttribute(Qt::WA_NoSystemBackground);setFocusPolicy(Qt::StrongFocus);
        for(const auto id:{UiText::StartupComponents,UiText::StartupProfiles,UiText::StartupOperators,UiText::StartupControls,UiText::StartupWorkspace,UiText::StartupReady})messages_.append(uiText(id,language));
        int revealDuration=0;
        for(const auto& spec:specs){
            const QRect crop=spec.rect.intersected(QRect(QPoint(),expanded.size()));if(crop.isEmpty())continue;
            const qreal ratio=frame.devicePixelRatio();QPixmap sharp=frame.copy(QRect(qRound(crop.x()*ratio),qRound(crop.y()*ratio),qRound(crop.width()*ratio),qRound(crop.height()*ratio)));
            if(spec.header){QPainter clear(&sharp);clear.setCompositionMode(QPainter::CompositionMode_Clear);clear.fillRect(brand_target_.translated(-crop.topLeft()),Qt::transparent);}
            Piece piece;piece.spec=spec;piece.spec.rect=crop;piece.frames={blur(sharp,12),blur(sharp,3.5),sharp};pieces_.append(std::move(piece));
            revealDuration=qMax(revealDuration,spec.delay+spec.duration);
        }
        total_=loadingDuration+expansionDuration+revealDuration;
        timeline_.setDuration(total_);timeline_.setStartValue(0);timeline_.setEndValue(total_);
        connect(&timeline_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){advance(value.toInt());});
        connect(&timeline_,&QVariantAnimation::finished,this,[this]{finished_();});
    }
    void start(){
#ifdef AZEC_UI_TESTS
        timing_clock_.start();
#endif
        timeline_.start();
    }
#ifdef AZEC_UI_TESTS
    QJsonObject frameTimings() const{
        const auto encode=[](const QVector<FrameTiming>& samples){QJsonArray result;for(const auto& sample:samples)result.append(QJsonObject{{"time",sample.time},{"atMs",sample.at},{"workMs",sample.work}});return result;};
        return {{"updates",encode(update_timings_)},{"paints",encode(paint_timings_)}};
    }
#endif
    void stop(){timeline_.stop();}
    int elapsed() const{return time_;}
    int duration() const{return total_;}
    qreal loadingProgress() const{return smooth(qreal(time_)/loadingDuration);}
    qreal cornerRadius() const{return 24-8*expansion();}
    int loadingStep() const{return qMin(5,time_*6/loadingDuration);}
    QRectF wordmarkRect() const{
        const qreal t=expansion();
        return blend(compactBrand().translated(compact_.topLeft()),brand_target_.translated(expanded_.topLeft()),t).translated(-parentWidget()->geometry().topLeft());
    }
protected:
    void paintEvent(QPaintEvent*) override{
#ifdef AZEC_UI_TESTS
        const auto began=timing_clock_.isValid()?timing_clock_.nsecsElapsed():0;
        const auto record=qScopeGuard([&]{if(timing_clock_.isValid())paint_timings_.append({time_,began/1e6,(timing_clock_.nsecsElapsed()-began)/1e6});});
#endif
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const int revealTime=time_-loadingDuration-expansionDuration;
        if(revealTime>=0){
            for(const auto& piece:pieces_){
                const auto& spec=piece.spec;const qreal t=fraction(qreal(revealTime-spec.delay)/spec.duration);if(t<=0)continue;
                const qreal settled=monoEase().valueForProgress(t),alpha=smooth(t/.65),clarity=smooth(t);
                const qreal level=clarity*2;const int first=qMin(1,int(level));const qreal mix=level-first;
                painter.save();painter.setClipRect(spec.rect.adjusted(0,-16,0,0));painter.translate(0,-12*(1-settled));
                painter.setOpacity(alpha*(1-mix));painter.drawPixmap(spec.rect.topLeft(),piece.frames[first]);
                painter.setOpacity(alpha*mix);painter.drawPixmap(spec.rect.topLeft(),piece.frames[first+1]);painter.restore();
            }
        }
        BrandLabel::draw(painter,wordmarkRect(),devicePixelRatioF());
        const qreal exit=smooth(qreal(time_-loadingDuration)/180);if(exit>=1)return;
        painter.setOpacity(1-exit);const qreal center=width()/2.0,barY=compact_.height()*.64;
        const QRectF bar(center-124,barY,248,3);painter.setPen(Qt::NoPen);painter.setBrush(kbtheme::content(.09));painter.drawRoundedRect(bar,1.5,1.5);
        painter.setBrush(kbtheme::defaultAccent);QRectF fill=bar;fill.setWidth(bar.width()*loadingProgress());painter.drawRoundedRect(fill,1.5,1.5);
        QFont font("Segoe UI");font.setPixelSize(12);painter.setFont(font);
        const qreal phase=std::fmod(qreal(qMin(time_,loadingDuration-1)),loadingDuration/6.0)/(loadingDuration/6.0);
        const qreal transition=smooth(phase/.32);const int step=loadingStep();
        const QRectF textRect(center-170,barY+17,340,22);painter.setPen(kbtheme::muted);
        if(step>0&&transition<1){painter.setOpacity((1-exit)*(1-transition));painter.drawText(textRect.translated(0,-4*transition),Qt::AlignCenter,messages_[step-1]);}
        painter.setOpacity((1-exit)*(step==0?1:transition));painter.drawText(textRect.translated(0,step==0?0:4*(1-transition)),Qt::AlignCenter,messages_[step]);
    }
};
