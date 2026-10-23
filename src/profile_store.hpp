#pragma once
#include "core.hpp"
#include <QtCore>
#include <cmath>
#include <algorithm>
#include "diagnostics.hpp"
#include "sensitivity.hpp"

inline QString weaponFile(const std::filesystem::path& base){return QString::fromStdWString((base.parent_path()/L"azec_weapons.json").wstring());}
// Rapid fire is stored per weapon profile. Older sidecars carried a single
// global switch; those load by copying it onto every primary and secondary
// profile so the effective behavior does not change on upgrade.
inline void readWeaponProfiles(r6::Config& config,bool& gameFocusOnly,bool& adsOnly,const std::filesystem::path& base){
    const auto clearRapid=[&]{for(auto& op:config.operators)op.rapid=0;for(auto& op:config.secondary)op.rapid=0;};
    QFile file(weaponFile(base));if(!file.exists()){clearRapid();return;}
    if(!file.open(QIODevice::ReadOnly)){diagnostic("load weapon profiles",file.errorString());clearRapid();return;}
    if(file.size()>1024*1024){diagnostic("load weapon profiles","File exceeds 1 MiB; ignored");clearRapid();return;}
    QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject()){diagnostic("load weapon profiles","Invalid JSON; ignored");clearRapid();return;}
    const auto root=document.object();if(root["version"].toInt()!=1){diagnostic("load weapon profiles","Unsupported version; ignored");clearRapid();return;}
    if(const auto setup=sensitivity::fromJson(root["sensitivity"].toObject()))config.sensitivity=*setup;
    const auto validKey=[](const QJsonValue& value,int fallback){const int key=value.toInt(fallback);return r6::validHotkey(key)?key:fallback;};
    config.primary_key=validKey(root["primaryKey"],'1');config.secondary_key=validKey(root["secondaryKey"],'2');config.phone_enabled=root["phoneEnabled"].toBool(true);
    bool legacyRapid=false;
    if(root.contains("rapidActive")&&root["rapidActive"].isBool())legacyRapid=root["rapidActive"].toBool();
    if(root.contains("gameFocusOnly")&&root["gameFocusOnly"].isBool())gameFocusOnly=root["gameFocusOnly"].toBool();
    if(root.contains("adsOnly")&&root["adsOnly"].isBool())adsOnly=root["adsOnly"].toBool();
    const auto profiles=root["operators"].toObject();
    for(int id=0;id<r6::operator_count;++id){
        const auto item=profiles[QString::fromUtf8(config.operators[id].name)].toObject();if(item.isEmpty())continue;
        const auto vertical=item["vertical"].toDouble(-1),horizontal=item["horizontal"].toDouble(-1000);const int delay=item["delay"].toInt(-1);
        if(!std::isfinite(vertical)||!std::isfinite(horizontal)||vertical<0||vertical>100||horizontal<-50||horizontal>50||delay<0||delay>60000){diagnostic("load weapon profile",QString::fromUtf8(config.operators[id].name)+": invalid values; ignored");continue;}
        auto& op=config.secondary[id];op=config.operators[id];op.vertical=float(vertical);op.horizontal=float(horizontal);op.delay=delay;op.rapid=item["rapid"].toBool();op.humanize=item["humanize"].toBool(true);
        config.secondary_enabled[id]=item["enabled"].toBool();config.secondary_initialized[id]=item["initialized"].toBool(true);
    }
    const int selected=root["selected"].toInt(-1);if(selected>=0&&selected<r6::operator_count)config.selected=selected;
    const auto primaries=root["primaryProfiles"].toObject();
    for(int id=r6::legacy_operator_count;id<r6::operator_count;++id){
        const auto name=QString::fromUtf8(config.operators[id].name);const auto item=primaries[name].toObject();if(item.isEmpty())continue;
        const auto vertical=item["vertical"].toDouble(-1),horizontal=item["horizontal"].toDouble(-1000);const int delay=item["delay"].toInt(-1);
        if(!std::isfinite(vertical)||!std::isfinite(horizontal)||vertical<0||vertical>100||horizontal<-50||horizontal>50||delay<0||delay>60000){diagnostic("load extension profile",name+": invalid values; ignored");continue;}
        auto& op=config.operators[id];op.vertical=float(vertical);op.horizontal=float(horizontal);op.delay=delay;op.rapid=item["rapid"].toBool();op.humanize=item["humanize"].toBool(true);
    }
    if(!root["perProfileRapid"].toBool(false)){for(auto& op:config.operators)op.rapid=legacyRapid?1:0;for(auto& op:config.secondary)op.rapid=legacyRapid?1:0;}
}
inline QByteArray weaponProfilesBytes(const r6::Config& config,bool gameFocusOnly,bool adsOnly){
    QJsonObject profiles,primaries;
    for(int id=0;id<r6::operator_count;++id){const auto& op=config.secondary[id];profiles[QString::fromUtf8(config.operators[id].name)]=QJsonObject{{"enabled",config.secondary_enabled[id]},{"initialized",config.secondary_initialized[id]},{"vertical",op.vertical},{"horizontal",op.horizontal},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)},{"delay",op.delay}};}
    // Extension operators keep their primary profile and the live selection here.
    for(int id=r6::legacy_operator_count;id<r6::operator_count;++id){const auto& op=config.operators[id];primaries[QString::fromUtf8(config.operators[id].name)]=QJsonObject{{"vertical",op.vertical},{"horizontal",op.horizontal},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)},{"delay",op.delay}};}
    return QJsonDocument(QJsonObject{{"version",1},{"primaryKey",config.primary_key},{"secondaryKey",config.secondary_key},{"phoneEnabled",config.phone_enabled},
        {"perProfileRapid",true},{"gameFocusOnly",gameFocusOnly},{"adsOnly",adsOnly},{"recoilMaster",true},{"rapidMaster",bool(config.rapid_master)},
        {"selected",config.selected},{"operators",profiles},{"primaryProfiles",primaries},
        {"sensitivity",config.sensitivity.valid()?QJsonValue(sensitivity::json(config.sensitivity)):QJsonValue(QJsonValue::Null)}}).toJson();
}
// Two-phase write: the sidecar is prepared without touching the disk copy and
// only committed after the primary binary write succeeded.
inline bool stageWeaponProfiles(const r6::Config& config,bool gameFocusOnly,bool adsOnly,const std::filesystem::path& base,QSaveFile& file){
    const auto bytes=weaponProfilesBytes(config,gameFocusOnly,adsOnly);
    file.setFileName(weaponFile(base));
    return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size();
}
// Selection changes are UI navigation, not profile edits. Persist only that
// field so browsing the roster never marks the profile dirty.
inline bool persistSelection(const std::filesystem::path& base,int selected){
    if(selected<0||selected>=r6::operator_count)return false;
    QFile existing(weaponFile(base));QJsonObject root;
    if(existing.exists()){
        if(!existing.open(QIODevice::ReadOnly)||existing.size()>1024*1024)return false;
        QJsonParseError error;const auto document=QJsonDocument::fromJson(existing.readAll(),&error);
        if(error.error!=QJsonParseError::NoError||!document.isObject()||document.object()["version"].toInt()!=1)return false;
        root=document.object();
    }
    root["version"]=1;root["selected"]=selected;
    const auto bytes=QJsonDocument(root).toJson();QSaveFile file(weaponFile(base));
    return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
}
