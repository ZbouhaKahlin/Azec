#pragma once
#include <QtGui>
#include <cmath>
#include "theme.hpp"

// Painted national flags for the language picker. Windows renders country flag
// emoji as letter pairs, so the flags are drawn directly instead.
inline void drawFlagStar(QPainter& painter,qreal x,qreal y,qreal radius){
    QPainterPath path;const qreal inner=radius*0.382;constexpr qreal pi=3.14159265358979323846;
    for(int i=0;i<10;++i){const qreal angle=-pi/2+i*pi/5;const qreal r=i%2?inner:radius;const QPointF point(x+std::cos(angle)*r,y+std::sin(angle)*r);i?path.lineTo(point):path.moveTo(point);}
    path.closeSubpath();painter.drawPath(path);
}
inline QIcon languageFlag(int language){
    constexpr int width=22,height=15;
    QPixmap pixmap(width*2,height*2);pixmap.setDevicePixelRatio(2.0);pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(QRectF(0,0,width,height));
    const QRectF area(0,0,width,height);
    switch(language){
        case 0: // Brazil
            painter.fillRect(area,QColor("#009C3B"));
            painter.setPen(Qt::NoPen);painter.setBrush(QColor("#FFDF00"));
            painter.drawPolygon(QPolygonF({QPointF(width/2.0,1.5),QPointF(width-1.5,height/2.0),QPointF(width/2.0,height-1.5),QPointF(1.5,height/2.0)}));
            painter.setBrush(QColor("#002776"));painter.drawEllipse(QPointF(width/2.0,height/2.0),3.6,3.6);
            break;
        case 2: // France
            painter.fillRect(QRectF(0,0,width/3.0,height),QColor("#0055A4"));
            painter.fillRect(QRectF(width/3.0,0,width/3.0,height),Qt::white);
            painter.fillRect(QRectF(2*width/3.0,0,width/3.0,height),QColor("#EF4135"));
            break;
        case 3: // Spain
            painter.fillRect(QRectF(0,0,width,height/4.0),QColor("#AA151B"));
            painter.fillRect(QRectF(0,height/4.0,width,height/2.0),QColor("#F1BF00"));
            painter.fillRect(QRectF(0,3*height/4.0,width,height/4.0),QColor("#AA151B"));
            painter.setPen(Qt::NoPen);painter.setBrush(QColor("#AA151B"));
            painter.drawRoundedRect(QRectF(3.4,height/2.0-2.2,3.4,4.4),1,1);
            break;
        case 4: // Germany
            painter.fillRect(QRectF(0,0,width,height/3.0),QColor("#111111"));
            painter.fillRect(QRectF(0,height/3.0,width,height/3.0),QColor("#DD0000"));
            painter.fillRect(QRectF(0,2*height/3.0,width,height/3.0),QColor("#FFCE00"));
            break;
        case 5: // Arabic: Saudi Arabia, green with a stylised script line and sword
            painter.fillRect(area,QColor("#0F7B3F"));
            painter.setPen(QPen(Qt::white,1.4,Qt::SolidLine,Qt::RoundCap));
            painter.drawLine(QPointF(4.0,5.2),QPointF(18.0,5.2));
            painter.drawLine(QPointF(5.8,7.5),QPointF(16.2,7.5));
            painter.drawLine(QPointF(6.8,10.8),QPointF(15.2,10.8));
            break;
        case 6: { // Israel: the prank bait, shown in the picker for "Hebrew"
            painter.fillRect(area,Qt::white);
            painter.fillRect(QRectF(0,1.7,width,1.8),QColor("#0038B8"));
            painter.fillRect(QRectF(0,height-3.5,width,1.8),QColor("#0038B8"));
            painter.setPen(QPen(QColor("#0038B8"),1.0));painter.setBrush(Qt::NoBrush);
            painter.drawPolygon(QPolygonF({QPointF(11.0,4.0),QPointF(8.0,9.2),QPointF(14.0,9.2)}));
            painter.drawPolygon(QPolygonF({QPointF(11.0,11.0),QPointF(8.0,5.8),QPointF(14.0,5.8)}));
            break;
        }
        case 7: // China
            painter.fillRect(area,QColor("#DE2910"));
            painter.setPen(Qt::NoPen);painter.setBrush(QColor("#FFDE00"));
            drawFlagStar(painter,4.6,4.2,3.3);
            drawFlagStar(painter,9.3,2.0,1.15);drawFlagStar(painter,11.3,4.0,1.15);drawFlagStar(painter,11.3,6.7,1.15);drawFlagStar(painter,9.3,8.6,1.15);
            break;
        case 8: // Japan
            painter.fillRect(area,Qt::white);
            painter.setPen(Qt::NoPen);painter.setBrush(QColor("#BC002D"));
            painter.drawEllipse(QPointF(width/2.0,height/2.0),3.6,3.6);
            break;
        default: { // English: United States
            painter.fillRect(area,QColor("#F4F4F4"));
            for(int stripe=0;stripe<13;++stripe)if(stripe%2==0)painter.fillRect(QRectF(0,stripe*height/13.0,width,height/13.0),QColor("#B22234"));
            painter.fillRect(QRectF(0,0,width*0.42,height*7/13.0),QColor("#3C3B6E"));
            painter.setPen(Qt::NoPen);painter.setBrush(Qt::white);
            for(int row=0;row<3;++row)for(int column=0;column<4;++column)painter.drawEllipse(QPointF(2.3+column*2.9,2.2+row*2.9),0.62,0.62);
        }
    }
    painter.setClipping(false);painter.setPen(QPen(kbtheme::ink,1.5));painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(area.adjusted(0.75,0.75,-0.75,-0.75),2.5,2.5);
    return QIcon(pixmap);
}
struct LanguageEntry{const char* label;int value;};
inline const QList<LanguageEntry>& languageEntries(){
    static const QList<LanguageEntry> entries={{"English (US)",1},{"Português (Brasil)",0},{"Français",2},{"Español",3},{"Deutsch",4},{"العربية",5},{"עברית",6},{"简体中文",7},{"日本語",8}};
    return entries;
}
// The reveal flag for the Free Palestine surprise popup.
inline QPixmap palestineFlagPixmap(int width,int height){
    QPixmap pixmap(width*2,height*2);pixmap.setDevicePixelRatio(2.0);pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(QRectF(0,0,width,height/3.0),QColor("#1B1B1B"));
    painter.fillRect(QRectF(0,height/3.0,width,height/3.0),Qt::white);
    painter.fillRect(QRectF(0,2*height/3.0,width,height/3.0),QColor("#007A3D"));
    painter.setPen(Qt::NoPen);painter.setBrush(QColor("#CE1126"));
    painter.drawPolygon(QPolygonF({QPointF(0,0),QPointF(width*0.42,height/2.0),QPointF(0,height)}));
    painter.setPen(QPen(kbtheme::ink,2.4));painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(0,0,width,height).adjusted(1.2,1.2,-1.2,-1.2),6,6);
    return pixmap;
}
