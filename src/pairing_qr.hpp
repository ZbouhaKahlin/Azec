#pragma once
#include <QtWidgets>
#include <QSvgRenderer>
#include <algorithm>
#include <cmath>
#include "theme.hpp"
#include "../third_party/qrcodegen/qrcodegen.hpp"

// Click/Space reveals; leaving Settings, hiding the app, or a changed link conceals it.
class PairingQr final : public QPushButton {
    QString link_,reveal_text_,hide_text_;
    QImage image_;
    QVector<QPixmap> frames_;
    QPixmap obscured_;
    quint64 generation_=0;
    QVariantAnimation reveal_;
    qreal progress_=0,raster_ratio_=0;
    bool revealed_=false,waiting_=false;
    void updateLabel(){setAccessibleName(revealed_?hide_text_:reveal_text_);setToolTip(revealed_?hide_text_:reveal_text_);update();}
    void animateToTarget(){
        const qreal from=progress_,to=revealed_?1.0:0.0;
        waiting_=false;
        {QSignalBlocker blocked(&reveal_);reveal_.stop();reveal_.setDuration(std::max(120,qRound(420*std::abs(to-from))));reveal_.setStartValue(from);reveal_.setEndValue(to);reveal_.setCurrentTime(0);}
        reveal_.start();
    }
    void setRevealed(bool revealed){
        revealed_=revealed;reveal_.stop();updateLabel();
        waiting_=frames_.isEmpty();if(waiting_&&!revealed){conceal();return;}if(!waiting_)animateToTarget();
    }
    // Prepare blur textures off the GUI thread. Painting only blends two cached
    // textures; it never filters or allocates a full QR image during a frame.
    static QImage blur(QImage source,int radius){
        if(!radius)return source;
        const int w=source.width(),h=source.height(),span=radius*2+1;
        QImage scratch(source.size(),QImage::Format_RGB32);
        auto* pixels=reinterpret_cast<QRgb*>(source.bits());auto* temporary=reinterpret_cast<QRgb*>(scratch.bits());
        const int stride=source.bytesPerLine()/int(sizeof(QRgb)),scratchStride=scratch.bytesPerLine()/int(sizeof(QRgb));
        for(int pass=0;pass<3;++pass){
            for(int y=0;y<h;++y){
                const auto* row=pixels+y*stride;auto* output=temporary+y*scratchStride;
                int red=0,green=0,blue=0;for(int offset=-radius;offset<=radius;++offset){const auto pixel=row[std::clamp(offset,0,w-1)];red+=qRed(pixel);green+=qGreen(pixel);blue+=qBlue(pixel);}
                for(int x=0;x<w;++x){output[x]=qRgb(red/span,green/span,blue/span);const auto add=row[std::min(w-1,x+radius+1)],remove=row[std::max(0,x-radius)];red+=qRed(add)-qRed(remove);green+=qGreen(add)-qGreen(remove);blue+=qBlue(add)-qBlue(remove);}
            }
            for(int x=0;x<w;++x){
                int red=0,green=0,blue=0;for(int offset=-radius;offset<=radius;++offset){const auto pixel=temporary[std::clamp(offset,0,h-1)*scratchStride+x];red+=qRed(pixel);green+=qGreen(pixel);blue+=qBlue(pixel);}
                for(int y=0;y<h;++y){pixels[y*stride+x]=qRgb(red/span,green/span,blue/span);const auto add=temporary[std::min(h-1,y+radius+1)*scratchStride+x],remove=temporary[std::max(0,y-radius)*scratchStride+x];red+=qRed(add)-qRed(remove);green+=qGreen(add)-qGreen(remove);blue+=qBlue(add)-qBlue(remove);}
            }
        }
        return source;
    }
    void prepareFrames(){
        const qreal ratio=devicePixelRatioF();if(image_.isNull()||raster_ratio_==ratio)return;
        raster_ratio_=ratio;const auto generation=++generation_;const int side=qRound(184*ratio);
        QImage raster(side,side,QImage::Format_RGB32);raster.fill(Qt::white);
        // Keep a four-module quiet zone and snap data modules to physical pixels.
        const int modules=image_.width()-8;const qreal unit=qreal(side)/image_.width();
        const auto finder=[modules](int x,int y){x-=4;y-=4;return (x>=0&&x<7&&y>=0&&y<7)||(x>=modules-7&&x<modules&&y>=0&&y<7)||(x>=0&&x<7&&y>=modules-7&&y<modules);};
        const QColor ink("#242128");QPainter painter(&raster);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);painter.setBrush(ink);
        for(int y=0;y<image_.height();++y)for(int x=0;x<image_.width();++x)if(qRed(image_.pixel(x,y))==0){
            if(finder(x,y))continue;
            const int left=qRound(qreal(x)*side/image_.width()),top=qRound(qreal(y)*side/image_.height());
            painter.drawRoundedRect(QRectF(left,top,qRound(qreal(x+1)*side/image_.width())-left,qRound(qreal(y+1)*side/image_.height())-top),unit*.12,unit*.12);
        }
        for(const auto origin:{QPoint(4,4),QPoint(modules-3,4),QPoint(4,modules-3)}){
            const QRectF outer(origin.x()*unit,origin.y()*unit,7*unit,7*unit);
            painter.setBrush(ink);painter.drawRoundedRect(outer,unit*.35,unit*.35);
            painter.setBrush(Qt::white);painter.drawRoundedRect(outer.adjusted(unit,unit,-unit,-unit),unit*.4,unit*.4);
            // Finder centers must stay dark on white, even with a light UI accent.
            painter.setBrush(QColor("#7248D7"));painter.drawRoundedRect(outer.adjusted(2*unit,2*unit,-2*unit,-2*unit),unit*.3,unit*.3);
        }
        // A small logo window consumes less than 6% of the symbol's modules.
        // The highest ECC level protects the obscured data. The finder and
        // timing patterns stay clear of the logo window.
        const qreal center=side/2.0,logoModules=std::min(8.0,modules*.2);
        const qreal half=(logoModules+1)*unit/2;
        painter.setBrush(Qt::white);painter.drawRoundedRect(QRectF(center-half,center-half,half*2,half*2),unit*.7,unit*.7);
        QSvgRenderer logo(QStringLiteral(":/ui/logo.svg"));const qreal logoSide=logoModules*unit;
        logo.render(&painter,QRectF(center-logoSide/2,center-logoSide/2,logoSide,logoSide));
        painter.end();
        auto placeholder=raster.scaled(9,9,Qt::IgnoreAspectRatio,Qt::SmoothTransformation).scaled(side,side,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        obscured_=QPixmap::fromImage(placeholder);obscured_.setDevicePixelRatio(ratio);
        QPointer<PairingQr> receiver(this);
        QThreadPool::globalInstance()->start([receiver,generation,ratio,raster=std::move(raster)]{
            QVector<QImage> images;const int maximum=qCeil(12*ratio);images.reserve(maximum+1);images.append(raster);
            for(int radius=1;radius<=maximum;++radius)images.append(blur(raster,radius));
            QMetaObject::invokeMethod(QCoreApplication::instance(),[receiver,generation,ratio,images=std::move(images)]{
                if(!receiver||receiver->generation_!=generation)return;
                receiver->frames_.clear();receiver->frames_.reserve(images.size());
                for(const auto& image:images){auto frame=QPixmap::fromImage(image);frame.setDevicePixelRatio(ratio);receiver->frames_.append(std::move(frame));}
                receiver->update();if(receiver->waiting_&&receiver->revealed_)receiver->animateToTarget();
            },Qt::QueuedConnection);
        });
    }
public:
    PairingQr(const QString& reveal,const QString& conceal,QWidget* parent=nullptr):QPushButton(parent),reveal_text_(reveal),hide_text_(conceal){
        setObjectName("pairingQr");setFixedSize(184,208);setCursor(Qt::PointingHandCursor);setFocusPolicy(Qt::StrongFocus);
        reveal_.setEasingCurve(QEasingCurve::InOutSine);
        connect(&reveal_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){progress_=value.toReal();update();});
        connect(this,&QPushButton::clicked,this,[this]{setRevealed(!revealed_);});updateLabel();
    }
    bool revealed() const{return revealed_;}
    qreal revealProgress() const{return progress_;}
    bool transitioning() const{return waiting_||reveal_.state()==QAbstractAnimation::Running;}
    bool framesReady() const{return !frames_.isEmpty();}
    void conceal(){reveal_.stop();waiting_=false;revealed_=false;progress_=0;updateLabel();}
    void setPairingLink(const QString& link){
        if(link==link_)return;conceal();link_=link;image_=QImage();frames_.clear();obscured_=QPixmap();raster_ratio_=0;++generation_;
        if(!link.isEmpty()){
            const auto qr=qrcodegen::QrCode::encodeText(link.toUtf8().constData(),qrcodegen::QrCode::Ecc::HIGH);
            const int extent=qr.getSize()+8;image_=QImage(extent,extent,QImage::Format_RGB32);image_.fill(Qt::white);
            for(int y=0;y<qr.getSize();++y)for(int x=0;x<qr.getSize();++x)if(qr.getModule(x,y))image_.setPixelColor(x+4,y+4,Qt::black);
        }
        prepareFrames();updateLabel();
    }
protected:
    void hideEvent(QHideEvent* event) override{conceal();QPushButton::hideEvent(event);}
    void showEvent(QShowEvent* event) override{prepareFrames();QPushButton::showEvent(event);}
    void paintEvent(QPaintEvent*) override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);painter.setBrush(Qt::white);painter.drawRoundedRect(QRectF(0,0,184,184),12,12);
        if(!image_.isNull()){
            if(raster_ratio_!=devicePixelRatioF())QTimer::singleShot(0,this,[this]{prepareFrames();});
            const QRectF target(0,0,184,184);
            painter.save();QPainterPath clip;clip.addRoundedRect(target,12,12);painter.setClipPath(clip);
            painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
            if(frames_.isEmpty())painter.drawPixmap(target,obscured_,QRectF(obscured_.rect()));
            else{
                // A logarithmic radius makes detail emerge gradually. A linear
                // radius spends most of the reveal looking equally blurry and
                // brings all the fine detail in during the final few frames.
                const qreal radius=std::expm1(std::log1p(frames_.size()-1)*(1-progress_));
                const int low=std::clamp(int(std::floor(radius)),0,int(frames_.size())-1);
                const qreal fraction=radius-low;
                painter.drawPixmap(target,frames_[low],QRectF(frames_[low].rect()));
                if(fraction>0&&low+1<frames_.size()){painter.setOpacity(fraction);painter.drawPixmap(target,frames_[low+1],QRectF(frames_[low+1].rect()));}
                // Use the same destroyed-detail texture at the concealed end
                // before and after asynchronous preparation. Blend into the
                // blur chain gently so neither startup nor section settling pops.
                if(progress_<.15){painter.setOpacity(1-progress_/.15);painter.drawPixmap(target,obscured_,QRectF(obscured_.rect()));}
            }
            painter.setOpacity(std::max(0.0,1-progress_*2));painter.setPen(QColor("#31204C"));painter.drawText(QRect(12,62,160,60),Qt::AlignCenter|Qt::TextWordWrap,reveal_text_);
            painter.restore();
        }
        painter.setBrush(Qt::NoBrush);painter.setPen(QPen(kbtheme::outline,2));painter.drawRoundedRect(QRectF(1,1,182,182),12,12);
        painter.setPen(kbtheme::muted);QFont font=painter.font();font.setPixelSize(12);painter.setFont(font);
        painter.drawText(QRect(0,187,184,21),Qt::AlignCenter,revealed_?hide_text_:reveal_text_);
        if(hasFocus()&&window()->testAttribute(Qt::WA_KeyboardFocusChange)){painter.setBrush(Qt::NoBrush);painter.setPen(QPen(kbtheme::purple,2));painter.drawRoundedRect(QRectF(2,2,180,180),12,12);}
    }
};
