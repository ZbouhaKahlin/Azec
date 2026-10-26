#pragma once
#include "core.hpp"
#include <QtCore>
#include <optional>
#include <cmath>
#include <algorithm>

namespace sensitivity {
inline QJsonObject json(const r6::SensitivitySetup& setup){
    return {{"dpi",setup.dpi},{"horizontal",setup.horizontal},{"vertical",setup.vertical},
        {"multiplier",setup.multiplier},{"source",QString::fromStdWString(setup.source)}};
}
inline std::optional<r6::SensitivitySetup> fromJson(const QJsonObject& object){
    r6::SensitivitySetup setup;
    const double dpi=object["dpi"].toDouble(-1);
    if(!std::isfinite(dpi)||dpi!=std::floor(dpi)||dpi<50||dpi>100000)return {};
    setup.dpi=int(dpi);setup.horizontal=object["horizontal"].toDouble();setup.vertical=object["vertical"].toDouble();
    setup.multiplier=object["multiplier"].toDouble();const auto source=object["source"].toString();
    if(source.size()>4096||!setup.valid())return {};
    setup.source=source.toStdWString();return setup;
}
// Read saved settings only. Never open the game, capture its screen, or write its INI.
inline std::optional<r6::SensitivitySetup> readFile(const QString& path){
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)return {};
    QTextStream stream(&file);QString section;QHash<QString,QString> values;
    while(!stream.atEnd()){
        const auto line=stream.readLine().trimmed();
        if(line.startsWith('[')&&line.endsWith(']')){section=line.mid(1,line.size()-2).trimmed();continue;}
        if(section.compare("INPUT",Qt::CaseInsensitive)!=0||line.startsWith(';')||line.startsWith('#'))continue;
        const int equals=line.indexOf('=');if(equals<=0)continue;
        values.insert(line.left(equals).trimmed().toLower(),line.mid(equals+1).section(';',0,0).trimmed());
    }
    r6::SensitivitySetup setup;bool h=false,v=false,m=false;
    setup.horizontal=values.value("mousesensitivityhorizontal").toDouble(&h);
    setup.vertical=values.value("mousesensitivityvertical").toDouble(&v);
    setup.multiplier=values.value("mousesensitivitymultiplierunit").toDouble(&m);
    if(!h||!v||!m||!setup.validGame())return {};
    setup.source=QFileInfo(path).absoluteFilePath().toStdWString();return setup;
}
inline bool sameGame(const r6::SensitivitySetup& a,const r6::SensitivitySetup& b){
    return qAbs(a.horizontal-b.horizontal)<0.000001&&qAbs(a.vertical-b.vertical)<0.000001&&qAbs(a.multiplier-b.multiplier)<0.000000001;
}
struct Scale {double horizontal=1,vertical=1;bool fits=true;};
inline Scale scale(const r6::Config& config,const r6::SensitivitySetup& next){
    Scale result;if(!config.sensitivity.valid()||!next.valid()){result.fits=false;return result;}
    // Synthetic movement bypasses the mouse sensor. DPI is reference metadata,
    // not another multiplier on the movement Azec generates.
    result.horizontal=(config.sensitivity.horizontal*config.sensitivity.multiplier)/(next.horizontal*next.multiplier);
    result.vertical=(config.sensitivity.vertical*config.sensitivity.multiplier)/(next.vertical*next.multiplier);
    const auto fits=[&](const r6::Operator& op){return op.vertical*result.vertical<=100.000001&&qAbs(op.horizontal*result.horizontal)<=50.000001;};
    for(int i=0;i<r6::operator_count;++i){
        if(!fits(config.operators[i])||(config.secondary_initialized[i]&&!fits(config.secondary[i])))result.fits=false;
    }
    return result;
}
inline void apply(r6::Config& config,const r6::SensitivitySetup& next){
    const auto factors=scale(config,next);if(!factors.fits)return;
    const auto adjust=[&](r6::Operator& op){op.vertical=float(std::round(op.vertical*factors.vertical*10)/10);op.horizontal=float(std::round(op.horizontal*factors.horizontal*10)/10);};
    for(int i=0;i<r6::operator_count;++i){adjust(config.operators[i]);if(config.secondary_initialized[i])adjust(config.secondary[i]);}
    config.sensitivity=next;
}
}
