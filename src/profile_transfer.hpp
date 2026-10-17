#pragma once
#include "core.hpp"
#include "hotkeys.hpp"
#include <QtCore>
#include <cmath>

inline QJsonObject backupProfile(const r6::Operator& op){
    return {{"vertical",op.vertical},{"horizontal",op.horizontal},{"delay",op.delay},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)}};
}
inline QJsonObject makeBackup(const r6::Config& config,const QSet<QString>& favorites){
    QJsonArray operators;
    for(int i=0;i<r6::operator_count;++i)operators.append(QJsonObject{{"name",QString::fromUtf8(config.operators[i].name)},
        {"primary",backupProfile(config.operators[i])},{"secondary",backupProfile(config.secondary[i])},
        {"secondaryEnabled",config.secondary_enabled[i]},{"secondaryInitialized",config.secondary_initialized[i]}});
    QJsonArray keys;for(const auto key:hotkeys(config))keys.append(key);
    QStringList names(favorites.begin(),favorites.end());names.sort();
    return {{"application","Azec"},{"version",1},{"operators",operators},{"favorites",QJsonArray::fromStringList(names)},
        {"settings",QJsonObject{{"keys",keys},{"language",config.language},{"sound",bool(config.sound)},{"rapidMaster",bool(config.rapid_master)},
            {"phoneEnabled",config.phone_enabled},{"selected",config.selected},{"radius",config.radius},
            {"accent",QJsonArray{config.neon[0],config.neon[1],config.neon[2]}}}}};
}
inline bool parseBackup(const QJsonObject& root,r6::Config& result,QSet<QString>& favorites){
    const auto number=[](const QJsonValue& value,double lo,double hi){return value.isDouble()&&std::isfinite(value.toDouble())&&value.toDouble()>=lo&&value.toDouble()<=hi;};
    const auto integer=[&](const QJsonValue& value,int lo,int hi){return number(value,lo,hi)&&std::floor(value.toDouble())==value.toDouble();};
    if((root["application"]!="Azec"&&root["application"]!="KingBoss2004")||!integer(root["version"],1,1)||!root["operators"].isArray()||!root["favorites"].isArray()||!root["settings"].isObject())return false;
    const auto records=root["operators"].toArray();const auto settings=root["settings"].toObject();r6::Config next;
    if(records.size()!=r6::operator_count)return false;
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
    const auto keys=settings["keys"].toArray();const auto accent=settings["accent"].toArray();
    if(keys.size()!=6||accent.size()!=3)return false;
    int* destinations[]={&next.menu_key,&next.recoil_key,&next.rapid_key,&next.panic_key,&next.primary_key,&next.secondary_key};
    for(int i=0;i<6;++i){if(!integer(keys[i],0,255))return false;*destinations[i]=keys[i].toInt();}
    if(!uniqueHotkeys(next))return false;
    for(int i=0;i<3;++i){if(!number(accent[i],0,1))return false;next.neon[i]=float(accent[i].toDouble());}
    next.language=settings["language"].toInt();next.selected=settings["selected"].toInt();next.radius=float(settings["radius"].toDouble());next.sound=settings["sound"].toBool();next.rapid_master=settings["rapidMaster"].toBool();next.phone_enabled=settings["phoneEnabled"].toBool();
    QSet<QString> nextFavorites;
    for(const auto& name:root["favorites"].toArray()){if(!name.isString()||!seen.contains(name.toString()))return false;nextFavorites.insert(name.toString());}
    result=next;favorites=nextFavorites;return true;
}
