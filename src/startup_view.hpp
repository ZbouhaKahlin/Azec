#pragma once
#include "ui_widgets.hpp"
#include "strings.hpp"
#include <array>

// A short-lived foreground, on the main HWND. Only visible roster rows are
// captured; all snapshots and blur frames disappear when the handoff finishes.
class StartupView final : public QWidget {
public:
    static constexpr int loadingStepDuration=800,loadingDuration=3*loadingStepDuration,expansionDuration=720;
    // Start content during the expansion's slow tail to avoid an empty-window pause.
    static constexpr int revealStart=loadingDuration+expansionDuration-180;
    struct PieceSpec {QRect rect;int delay=0;int duration=540;bool header=false;};
private:
    struct Piece {PieceSpec spec;std::array<QPixmap,3> frames;QImage blended;};
    QVector<Piece> pieces_;
    QRect compact_,expanded_;QRectF brand_target_;
    QString loading_label_;QFont loading_font_;QVector<std::array<QPixmap,3>> words_;QImage word_blend_;
    QString previous_status_;
    qreal label_width_=0,word_width_=0;int text_height_=0;bool rtl_=false;QVariantAnimation timeline_;
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
    static void interpolate(QImage& target,const std::array<QPixmap,3>& frames,qreal clarity){
        const qreal level=fraction(clarity)*2;const int first=qMin(1,int(level));
        QPainter blend(&target);blend.setCompositionMode(QPainter::CompositionMode_Source);
        blend.drawPixmap(0,0,frames[first]);blend.setOpacity(level-first);blend.drawPixmap(0,0,frames[first+1]);
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
        loading_label_=uiText(UiText::StartupLoading,language);rtl_=language==5||language==6;
        loading_font_=QFont("Segoe UI");loading_font_.setPixelSize(12);const QFontMetrics metrics(loading_font_);
        label_width_=metrics.horizontalAdvance(loading_label_);text_height_=metrics.height();QStringList words;
        for(const auto id:{UiText::StartupComponents,UiText::StartupProfiles,UiText::StartupWorkspace}){
            words.append(uiText(id,language));word_width_=qMax(word_width_,qreal(metrics.horizontalAdvance(words.back())));
        }
        const qreal ratio=frame.devicePixelRatio();const QSize tile(qCeil((word_width_+24)*ratio),qCeil((text_height_+24)*ratio));
        for(const auto& word:words){
            QPixmap sharp(tile);sharp.setDevicePixelRatio(ratio);sharp.fill(Qt::transparent);
            {QPainter p(&sharp);p.setFont(loading_font_);p.setPen(kbtheme::muted);p.drawText(QRectF(12,12,word_width_,text_height_),Qt::AlignTop|(rtl_?Qt::AlignRight:Qt::AlignLeft),word);}
            words_.append({blur(sharp,4),blur(sharp,1.5),sharp});
        }
        word_blend_=QImage(tile,QImage::Format_ARGB32_Premultiplied);word_blend_.setDevicePixelRatio(ratio);
        int revealDuration=0;
        for(const auto& spec:specs){
            const QRect crop=spec.rect.intersected(QRect(QPoint(),expanded.size()));if(crop.isEmpty())continue;
            const qreal ratio=frame.devicePixelRatio();QPixmap sharp=frame.copy(QRect(qRound(crop.x()*ratio),qRound(crop.y()*ratio),qRound(crop.width()*ratio),qRound(crop.height()*ratio)));
            if(spec.header){QPainter clear(&sharp);clear.setCompositionMode(QPainter::CompositionMode_Clear);clear.fillRect(brand_target_.translated(-crop.topLeft()),Qt::transparent);}
            Piece piece;piece.spec=spec;piece.spec.rect=crop;piece.frames={blur(sharp,12),blur(sharp,3.5),sharp};
            piece.blended=QImage(sharp.size(),QImage::Format_ARGB32_Premultiplied);piece.blended.setDevicePixelRatio(ratio);pieces_.append(std::move(piece));
            revealDuration=qMax(revealDuration,spec.delay+spec.duration);
        }
        total_=qMax(loadingDuration+expansionDuration,revealStart+revealDuration);
        timeline_.setDuration(total_);timeline_.setStartValue(0);timeline_.setEndValue(total_);
        connect(&timeline_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){advance(value.toInt());});
        connect(&timeline_,&QVariantAnimation::finished,this,[this]{finished_();});
    }
    void start(const QString& previousStatus={}){
        previous_status_=previousStatus;
#ifdef AZEC_UI_TESTS
        timing_clock_.start();
#endif
        timeline_.start();
    }
#ifdef AZEC_UI_TESTS
    void seekForVerification(int time){advance(time);}
    QJsonObject frameTimings() const{
        const auto encode=[](const QVector<FrameTiming>& samples){QJsonArray result;for(const auto& sample:samples)result.append(QJsonObject{{"time",sample.time},{"atMs",sample.at},{"workMs",sample.work}});return result;};
        return {{"updates",encode(update_timings_)},{"paints",encode(paint_timings_)}};
    }
#endif
    void stop(){timeline_.stop();}
    int elapsed() const{return time_;}
    int duration() const{return total_;}
    qreal loadingProgress() const{return .25+.75*smooth(qreal(time_)/loadingDuration);}
    qreal cornerRadius() const{return 24-8*expansion();}
    int loadingStep() const{return qMin(int(words_.size())-1,time_/loadingStepDuration);}
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
        const int revealTime=time_-revealStart;
        if(revealTime>=0){
            for(auto& piece:pieces_){
                const auto& spec=piece.spec;const qreal t=fraction(qreal(revealTime-spec.delay)/spec.duration);if(t<=0)continue;
                if(t>=1){painter.drawPixmap(spec.rect.topLeft(),piece.frames.back());continue;}
                const qreal settled=monoEase().valueForProgress(t),alpha=smooth(t/.65),clarity=smooth(t);
                // Blend premultiplied images before fading once; separate SourceOver
                // fades attenuate each other and make the panel's opacity pulse.
                interpolate(piece.blended,piece.frames,clarity);
                painter.save();painter.setClipRect(spec.rect.adjusted(0,-16,0,0));painter.translate(0,-12*(1-settled));
                painter.setOpacity(alpha);painter.drawImage(spec.rect.topLeft(),piece.blended);painter.restore();
            }
        }
        BrandLabel::draw(painter,wordmarkRect(),devicePixelRatioF());
        const qreal exit=smooth(qreal(time_-loadingDuration)/180);if(exit>=1)return;
        painter.setOpacity(1-exit);const qreal center=width()/2.0,barY=compact_.height()*.64;
        const QRectF bar(center-124,barY,248,3);painter.setPen(Qt::NoPen);painter.setBrush(kbtheme::content(.09));painter.drawRoundedRect(bar,1.5,1.5);
        painter.setBrush(kbtheme::defaultAccent);QRectF fill=bar;fill.setWidth(bar.width()*loadingProgress());painter.drawRoundedRect(fill,1.5,1.5);
        painter.setFont(loading_font_);painter.setPen(kbtheme::muted);
        const qreal entry=previous_status_.isEmpty()?1:smooth(qreal(time_)/240);
        if(entry<1){painter.setOpacity((1-exit)*(1-entry));painter.drawText(QRectF(center-174,barY+17,348,28),Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,previous_status_);}
        painter.setOpacity((1-exit)*entry);
        // Share the update status's top alignment through the handoff and every subject.
        const qreal left=center-(label_width_+6+word_width_)/2,top=barY+17;
        const qreal wordX=rtl_?left:left+label_width_+6;
        painter.drawText(QRectF(rtl_?left+word_width_+6:left,top,label_width_,text_height_),Qt::AlignTop|(rtl_?Qt::AlignRight:Qt::AlignLeft),loading_label_);
        const qreal phase=std::fmod(qreal(qMin(time_,loadingDuration-1)),loadingStepDuration);
        const qreal transition=smooth(phase/300);const int step=loadingStep();
        const auto drawWord=[&](int index,qreal clarity,qreal opacity){
            interpolate(word_blend_,words_[index],clarity);painter.setOpacity((1-exit)*entry*opacity);
            painter.drawImage(QPointF(wordX-12,top-12),word_blend_);
        };
        if(step>0&&transition<1)drawWord(step-1,1-transition,1-transition);
        drawWord(step,step==0?1:transition,step==0?1:transition);
    }
};
