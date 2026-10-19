#pragma once
#include "core.hpp"
#include "hotkeys.hpp"
#include <QtCore>
#include <cmath>
#include <optional>

struct BackupOptions {
    std::optional<bool> gameFocusOnly;
    std::optional<bool> adsOnly;
    std::optional<bool> sidebarCollapsed;
    QString colorTheme;
};

inline QJsonObject backupProfile(const r6::Operator& op){
    return {{"vertical",op.vertical},{"horizontal",op.horizontal},{"delay",op.delay},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)}};
}
// Version 1 keeps the six-key settings shape so older builds can still import
// new backups; the removed settings are written normalized (accent/radius) or
// preserved as raw bytes (rapid key/master).
inline QJsonObject makeBackup(const r6::Config& config,const QSet<QString>& favorites,
                              bool gameFocusOnly,
                              const QString& colorTheme,bool sidebarCollapsed,bool adsOnly=false){
    QJsonArray operators;
    for(int i=0;i<r6::operator_count;++i)operators.append(QJsonObject{{"name",QString::fromUtf8(config.operators[i].name)},
        {"primary",backupProfile(config.operators[i])},{"secondary",backupProfile(config.secondary[i])},
        {"secondaryEnabled",config.secondary_enabled[i]},{"secondaryInitialized",config.secondary_initialized[i]}});
    const int keys[]={config.menu_key,config.recoil_key,config.rapid_key,config.panic_key,config.primary_key,config.secondary_key};
    QJsonArray keyArray;for(const auto key:keys)keyArray.append(key);
    QStringList names(favorites.begin(),favorites.end());names.sort();
    return {{"application","Azec"},{"version",1},{"operators",operators},{"favorites",QJsonArray::fromStringList(names)},
        {"settings",QJsonObject{{"keys",keyArray},{"language",config.language},{"sound",bool(config.sound)},{"rapidMaster",bool(config.rapid_master)},
            {"recoilMaster",true},{"perProfileRapid",true},{"gameFocusOnly",gameFocusOnly},{"adsOnly",adsOnly},
            {"colorTheme",colorTheme},{"sidebarCollapsed",sidebarCollapsed},
            {"phoneEnabled",config.phone_enabled},{"selected",config.selected},{"radius",130.0},
            {"accent",QJsonArray{114.0/255,72.0/255,215.0/255}}}}};
}
inline bool parseBackup(const QJsonObject& root,r6::Config& result,QSet<QString>& favorites,BackupOptions& options){
    const auto number=[](const QJsonValue& value,double lo,double hi){return value.isDouble()&&std::isfinite(value.toDouble())&&value.toDouble()>=lo&&value.toDouble()<=hi;};
    const auto integer=[&](const QJsonValue& value,int lo,int hi){return number(value,lo,hi)&&std::floor(value.toDouble())==value.toDouble();};
    if(root["application"]!="Azec"||!integer(root["version"],1,1)||!root["operators"].isArray()||!root["favorites"].isArray()||!root["settings"].isObject())return false;
    const auto records=root["operators"].toArray();const auto settings=root["settings"].toObject();r6::Config next;
    // Backups made before the extension carry 71 records; both sizes import by name.
    if(records.size()!=r6::operator_count&&records.size()!=r6::legacy_operator_count)return false;
    const auto profile=[&](const QJsonValue& record,r6::Operator& op){
        if(!record.isObject())return false;const auto values=record.toObject();
        if(!number(values["vertical"],0,100)||!number(values["horizontal"],-50,50)||!integer(values["delay"],0,60000)||!values["rapid"].isBool()||!values["humanize"].isBool())return false;
        op.vertical=float(values["vertical"].toDouble());op.horizontal=float(values["horizontal"].toDouble());op.delay=values["delay"].toInt();op.rapid=values["rapid"].toBool();op.humanize=values["humanize"].toBool();return true;
    };
    QSet<QString> seen;
    for(const auto& record:records){
        if(!record.isObject())return false;const auto item=record.toObject();const auto name=item["name"].toString();int id=-1;
        for(int i=0;i<r6::operator_count;++i)if(name==QString::fromUtf8(next.operators[i].name)){id=i;break;}
        if(id<0||seen.contains(name)||!item["secondaryEnabled"].isBool()||!item["secondaryInitialized"].isBool())return false;
        if(!profile(item["primary"],next.operators[id])||!profile(item["secondary"],next.secondary[id]))return false;
        next.secondary_enabled[id]=item["secondaryEnabled"].toBool();next.secondary_initialized[id]=item["secondaryInitialized"].toBool();
        if(next.secondary_enabled[id]&&!next.secondary_initialized[id])return false;
        seen.insert(name);
    }
    if(!integer(settings["language"],0,8)||!integer(settings["selected"],0,r6::operator_count-1)||!number(settings["radius"],1,10000)||!settings["sound"].isBool()||!settings["rapidMaster"].isBool()||!settings["phoneEnabled"].isBool())return false;
    if(settings.contains("recoilMaster")&&!settings["recoilMaster"].isBool())return false;
    if(settings.contains("rapidActive")&&!settings["rapidActive"].isBool())return false;
    if(settings.contains("gameFocusOnly")&&!settings["gameFocusOnly"].isBool())return false;
    if(settings.contains("adsOnly")&&!settings["adsOnly"].isBool())return false;
    if(settings.contains("sidebarCollapsed")&&!settings["sidebarCollapsed"].isBool())return false;
    if(settings.contains("colorTheme")&&!settings["colorTheme"].isString())return false;
    const auto keys=settings["keys"].toArray();const auto accent=settings["accent"].toArray();
    if(keys.size()!=6||accent.size()!=3)return false;
    int* destinations[]={&next.menu_key,&next.recoil_key,&next.rapid_key,&next.panic_key,&next.primary_key,&next.secondary_key};
    for(int i=0;i<6;++i){if(!integer(keys[i],0,255))return false;*destinations[i]=keys[i].toInt();}
    // Only the five live bindings must be unique; the frozen rapid slot may hold
    // any legacy value without blocking an import.
    const auto bindings=hotkeys(next);
    for(size_t i=0;i<bindings.size();++i)for(size_t j=i+1;j<bindings.size();++j)if(bindings[i]&&bindings[i]==bindings[j])return false;
    next.menu_key=r6::validGlobalHotkey(next.menu_key)?next.menu_key:0;next.recoil_key=r6::validGlobalHotkey(next.recoil_key)?next.recoil_key:0;
    next.panic_key=r6::validGlobalHotkey(next.panic_key)?next.panic_key:0;
    next.primary_key=r6::validHotkey(next.primary_key)?next.primary_key:0;next.secondary_key=r6::validHotkey(next.secondary_key)?next.secondary_key:0;
    next.rapid_key=r6::validHotkey(next.rapid_key)?next.rapid_key:0;
    // Compatibility validation still requires the old fields, but the values
    // themselves are normalized to the current fixed materials.
    for(int i=0;i<3;++i)if(!number(accent[i],0,1))return false;
    next.language=settings["language"].toInt();next.selected=settings["selected"].toInt();next.radius=130;next.sound=settings["sound"].toBool();next.rapid_master=settings["rapidMaster"].toBool();next.phone_enabled=settings["phoneEnabled"].toBool();
    next.neon[0]=114.0f/255;next.neon[1]=72.0f/255;next.neon[2]=215.0f/255;
    options=BackupOptions{};
    // Backups without the per-profile marker predate independent weapon rapid
    // fire; their single global switch applies to both slots for compatibility.
    if(!settings.value("perProfileRapid").toBool(false)){
        const bool legacyRapid=settings.value("rapidActive").toBool(false);
        for(int i=0;i<r6::operator_count;++i){next.operators[i].rapid=legacyRapid?1:0;next.secondary[i].rapid=legacyRapid?1:0;}
    }
    if(settings.contains("gameFocusOnly"))options.gameFocusOnly=settings["gameFocusOnly"].toBool();
    if(settings.contains("adsOnly"))options.adsOnly=settings["adsOnly"].toBool();
    if(settings.contains("sidebarCollapsed"))options.sidebarCollapsed=settings["sidebarCollapsed"].toBool();
    if(settings.contains("colorTheme"))options.colorTheme=settings["colorTheme"].toString();
    QSet<QString> nextFavorites;
    for(const auto& name:root["favorites"].toArray()){if(!name.isString()||!seen.contains(name.toString()))return false;nextFavorites.insert(name.toString());}
    result=next;favorites=nextFavorites;return true;
}
