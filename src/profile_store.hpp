#pragma once
#include "core.hpp"
#include <QtCore>
#include <cmath>
#include <algorithm>

inline QString weaponFile(const std::filesystem::path& base){return QString::fromStdWString((base.parent_path()/L"kingboss_weapons.json").wstring());}
inline void readWeaponProfiles(r6::Config& config,const std::filesystem::path& base){
    QFile file(weaponFile(base));if(!file.open(QIODevice::ReadOnly))return;
    const auto root=QJsonDocument::fromJson(file.readAll()).object();if(root["version"].toInt()!=1)return;
    const auto validKey=[](const QJsonValue& value,int fallback){const int key=value.toInt(fallback);return key>=0&&key<=255?key:fallback;};
    config.primary_key=validKey(root["primaryKey"],'1');config.secondary_key=validKey(root["secondaryKey"],'2');config.phone_enabled=root["phoneEnabled"].toBool(true);
    const auto profiles=root["operators"].toObject();
    for(int id=0;id<r6::operator_count;++id){
        const auto item=profiles[QString::fromUtf8(config.operators[id].name)].toObject();if(item.isEmpty())continue;
        const auto vertical=item["vertical"].toDouble(-1),horizontal=item["horizontal"].toDouble(-1000);const int delay=item["delay"].toInt(-1);
        if(!std::isfinite(vertical)||!std::isfinite(horizontal)||vertical<0||vertical>100||horizontal<-50||horizontal>50||delay<30||delay>150)continue;
        auto& op=config.secondary[id];op=config.operators[id];op.vertical=float(vertical);op.horizontal=float(horizontal);op.delay=delay;op.rapid=item["rapid"].toBool();op.humanize=item["humanize"].toBool(true);
        config.secondary_enabled[id]=item["enabled"].toBool();config.secondary_initialized[id]=item["initialized"].toBool(true);
    }
}
inline bool writeWeaponProfiles(const r6::Config& config,const std::filesystem::path& base){
    QJsonObject profiles;
    for(int id=0;id<r6::operator_count;++id){const auto& op=config.secondary[id];profiles[QString::fromUtf8(config.operators[id].name)]=QJsonObject{{"enabled",config.secondary_enabled[id]},{"initialized",config.secondary_initialized[id]},{"vertical",op.vertical},{"horizontal",op.horizontal},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)},{"delay",std::clamp(op.delay,30,150)}};}
    const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"primaryKey",config.primary_key},{"secondaryKey",config.secondary_key},{"phoneEnabled",config.phone_enabled},{"operators",profiles}}).toJson();
    QSaveFile file(weaponFile(base));return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
}
