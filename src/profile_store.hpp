#pragma once
#include "core.hpp"
#include <QtCore>
#include <cmath>
#include <algorithm>
#include "diagnostics.hpp"

inline QString weaponFile(const std::filesystem::path& base){return QString::fromStdWString((base.parent_path()/L"azec_weapons.json").wstring());}
// Files written before the Azec rename still load once; the next save uses the new name.
inline QString legacyWeaponFile(const std::filesystem::path& base){return QString::fromStdWString((base.parent_path()/L"kingboss_weapons.json").wstring());}
inline void readWeaponProfiles(r6::Config& config,const std::filesystem::path& base){
    QFile file(weaponFile(base));if(!file.exists()&&QFile::exists(legacyWeaponFile(base)))file.setFileName(legacyWeaponFile(base));if(!file.exists())return;
    if(!file.open(QIODevice::ReadOnly)){diagnostic("load weapon profiles",file.errorString());return;}
    if(file.size()>1024*1024){diagnostic("load weapon profiles","File exceeds 1 MiB; ignored");return;}
    QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject()){diagnostic("load weapon profiles","Invalid JSON; ignored");return;}
    const auto root=document.object();if(root["version"].toInt()!=1){diagnostic("load weapon profiles","Unsupported version; ignored");return;}
    const auto validKey=[](const QJsonValue& value,int fallback){const int key=value.toInt(fallback);return key>=0&&key<=255?key:fallback;};
    config.primary_key=validKey(root["primaryKey"],'1');config.secondary_key=validKey(root["secondaryKey"],'2');config.phone_enabled=root["phoneEnabled"].toBool(true);
    const auto profiles=root["operators"].toObject();
    for(int id=0;id<r6::operator_count;++id){
        const auto item=profiles[QString::fromUtf8(config.operators[id].name)].toObject();if(item.isEmpty())continue;
        const auto vertical=item["vertical"].toDouble(-1),horizontal=item["horizontal"].toDouble(-1000);const int delay=item["delay"].toInt(-1);
        if(!std::isfinite(vertical)||!std::isfinite(horizontal)||vertical<0||vertical>100||horizontal<-50||horizontal>50||delay<0||delay>60000){diagnostic("load weapon profile",QString::fromUtf8(config.operators[id].name)+": invalid values; ignored");continue;}
        auto& op=config.secondary[id];op=config.operators[id];op.vertical=float(vertical);op.horizontal=float(horizontal);op.delay=delay;op.rapid=item["rapid"].toBool();op.humanize=item["humanize"].toBool(true);
        config.secondary_enabled[id]=item["enabled"].toBool();config.secondary_initialized[id]=item["initialized"].toBool(true);
    }
}
inline bool writeWeaponProfiles(const r6::Config& config,const std::filesystem::path& base){
    QJsonObject profiles;
    for(int id=0;id<r6::operator_count;++id){const auto& op=config.secondary[id];profiles[QString::fromUtf8(config.operators[id].name)]=QJsonObject{{"enabled",config.secondary_enabled[id]},{"initialized",config.secondary_initialized[id]},{"vertical",op.vertical},{"horizontal",op.horizontal},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)},{"delay",op.delay}};}
    const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"primaryKey",config.primary_key},{"secondaryKey",config.secondary_key},{"phoneEnabled",config.phone_enabled},{"operators",profiles}}).toJson();
    QSaveFile file(weaponFile(base));const bool ok=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
    if(!ok)diagnostic("save weapon profiles",file.errorString());return ok;
}
