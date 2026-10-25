#pragma once
#include <QtWidgets>
#include <algorithm>
#include <cmath>

enum class InteractionIcon { None, Save, Settings, Copy, Recoil, Rapid };

inline const char* interactionIconAsset(InteractionIcon icon){
    switch(icon){
    case InteractionIcon::Save:return ":/ui/save.svg";
    case InteractionIcon::Settings:return ":/ui/settings.svg";
    case InteractionIcon::Copy:return ":/ui/copy.svg";
    case InteractionIcon::Recoil:return ":/ui/recoil.svg";
    case InteractionIcon::Rapid:return ":/ui/rapid.svg";
    default:return "";
    }
}
inline qreal interactionEase(qreal value){return 1-std::pow(1-std::clamp(value,qreal(0),qreal(1)),3);}
inline QEasingCurve controlEase(){
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(.2,0),QPointF(.2,1),QPointF(1,1));
    return curve;
}
inline QColor mixColor(const QColor& from,const QColor& to,qreal amount){
    return QColor::fromRgbF(from.redF()+(to.redF()-from.redF())*amount,from.greenF()+(to.greenF()-from.greenF())*amount,from.blueF()+(to.blueF()-from.blueF())*amount,from.alphaF()+(to.alphaF()-from.alphaF())*amount);
}
inline qreal interactionReturnBlend(qreal progress){
    const qreal value=std::clamp(progress-1,qreal(0),qreal(1));
    return value*value*(3-2*value);
}
inline void drawInteractionStroke(QPainter& painter,const QPainterPath& path,qreal progress){
    progress=std::clamp(progress,qreal(0),qreal(1));if(progress<=0)return;
    painter.save();
    if(progress<1){auto pen=painter.pen();const qreal length=path.length()/pen.widthF();pen.setDashPattern({qMax(qreal(.001),length*progress),length+1});painter.setPen(pen);}
    painter.drawPath(path);painter.restore();
}
inline QPainterPath interactionCheck(){QPainterPath path;path.moveTo(6.5,12);path.lineTo(10.2,15.8);path.lineTo(17.9,7.8);return path;}

// Resting artwork matches the bundled SVGs; native strokes animate their parts.
// No renderer or timer keeps running after an interaction has settled.
inline void paintInteractionIcon(QPainter& painter,const QRectF& area,InteractionIcon icon,const QColor& ink,qreal progress=0){
    // The second half fades the completed check back to its resting artwork.
    if((icon==InteractionIcon::Save||icon==InteractionIcon::Copy)&&progress>1){
        const qreal blend=interactionReturnBlend(progress),opacity=painter.opacity();
        painter.save();painter.setOpacity(opacity*(1-blend));paintInteractionIcon(painter,area,icon,ink,1);
        painter.setOpacity(opacity*blend);paintInteractionIcon(painter,area,icon,ink,0);painter.restore();return;
    }
    painter.save();painter.translate(area.topLeft());painter.scale(area.width()/24,area.height()/24);
    painter.setRenderHint(QPainter::Antialiasing);painter.setPen(QPen(ink,1.65,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.setBrush(Qt::NoBrush);
    const qreal opacity=painter.opacity();
    if(icon==InteractionIcon::Save){
        const qreal land=interactionEase(progress/.44),fade=std::clamp((progress-.30)/.15,qreal(0),qreal(1));
        painter.setOpacity(opacity*(1-std::clamp((progress-.45)/.20,qreal(0),qreal(1))));
        QPainterPath tray;tray.moveTo(4.5,14.5);tray.lineTo(4.5,18.5);tray.quadTo(4.5,20,6,20);tray.lineTo(18,20);tray.quadTo(19.5,20,19.5,18.5);tray.lineTo(19.5,14.5);painter.drawPath(tray);
        painter.save();painter.setOpacity(opacity*(1-fade));painter.translate(0,6*land);
        QPainterPath arrow;arrow.moveTo(12,3.5);arrow.lineTo(12,14.5);arrow.moveTo(8,10.5);arrow.lineTo(12,14.5);arrow.lineTo(16,10.5);painter.drawPath(arrow);painter.restore();
        painter.setOpacity(opacity);drawInteractionStroke(painter,interactionCheck(),interactionEase((progress-.46)/.54));
    }else if(icon==InteractionIcon::Settings){
        const qreal shift=6*std::sin(std::clamp(progress,qreal(0),qreal(1))*3.141592653589793);
        for(const auto& row:{QPointF(9+shift,7),QPointF(15-shift,17)}){
            painter.drawLine(QPointF(4,row.y()),QPointF(row.x()-2.5,row.y()));painter.drawLine(QPointF(row.x()+2.5,row.y()),QPointF(20,row.y()));painter.drawEllipse(row,2.5,2.5);
        }
    }else if(icon==InteractionIcon::Copy){
        painter.setOpacity(opacity*(1-std::clamp(progress/.32,qreal(0),qreal(1))));
        painter.drawRoundedRect(QRectF(8,8,11,12),2,2);
        QPainterPath back;back.moveTo(15.5,8);back.lineTo(15.5,5.5);back.quadTo(15.5,4,14,4);back.lineTo(5.5,4);back.quadTo(4,4,4,5.5);back.lineTo(4,14);back.quadTo(4,15.5,5.5,15.5);back.lineTo(8,15.5);painter.drawPath(back);
        painter.setOpacity(opacity);drawInteractionStroke(painter,interactionCheck(),interactionEase((progress-.25)/.75));
    }else if(icon==InteractionIcon::Recoil){
        QPainterPath circle;circle.arcMoveTo(QRectF(4,4,16,16),125);circle.arcTo(QRectF(4,4,16,16),125,290);drawInteractionStroke(painter,circle,progress);
        painter.drawLine(QPointF(12,3),QPointF(12,11));
    }else if(icon==InteractionIcon::Rapid){
        QPainterPath bolt;bolt.moveTo(13.5,3);bolt.lineTo(6.5,13);bolt.lineTo(11.5,13);bolt.lineTo(10.5,21);bolt.lineTo(17.5,10);bolt.lineTo(12.5,10);bolt.closeSubpath();drawInteractionStroke(painter,bolt,progress);
    }
    painter.restore();
}
