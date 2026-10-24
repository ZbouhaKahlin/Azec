#pragma once
#include "sensitivity.hpp"

namespace profilesets {
constexpr size_t limit=24;
inline r6::ProfileSet capture(const r6::Config& config,const std::string& id,const std::string& name){
    return {id,name,config.operators,config.secondary,config.secondary_enabled,config.secondary_initialized,config.sensitivity};
}
inline void remember(r6::Config& config){
    for(auto& set:config.profile_sets)if(set.id==config.active_profile_set){set=capture(config,set.id,set.name);return;}
}
inline void activate(r6::Config& config,const r6::ProfileSet& set){
    config.operators=set.primary;config.secondary=set.secondary;
    config.secondary_enabled=set.secondary_enabled;config.secondary_initialized=set.secondary_initialized;
    config.sensitivity=set.sensitivity;config.active_profile_set=set.id;config.weapon_slot=0;
}
inline QJsonObject profile(const r6::Operator& op){
    return {{"vertical",op.vertical},{"horizontal",op.horizontal},{"delay",op.delay},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)}};
}
inline QJsonArray json(const r6::Config& config){
    auto copy=config;remember(copy);QJsonArray sets;
    for(const auto& set:copy.profile_sets){
        QJsonArray operators;
        for(int i=0;i<r6::operator_count;++i)operators.append(QJsonObject{{"name",QString::fromUtf8(set.primary[i].name)},
            {"primary",profile(set.primary[i])},{"secondary",profile(set.secondary[i])},
            {"secondaryEnabled",set.secondary_enabled[i]},{"secondaryInitialized",set.secondary_initialized[i]}});
        auto setup=sensitivity::json(set.sensitivity);setup.remove("source");
        sets.append(QJsonObject{{"id",QString::fromStdString(set.id)},{"name",QString::fromStdString(set.name)},
            {"operators",operators},{"sensitivity",set.sensitivity.valid()?QJsonValue(setup):QJsonValue(QJsonValue::Null)}});
    }
    return sets;
}
inline bool read(const QJsonObject& root,r6::Config& config){
    config.profile_sets.clear();config.active_profile_set.clear();
    if(!root.contains("profileSets"))return true;
    if(!root["profileSets"].isArray()||root["profileSets"].toArray().size()>int(limit))return false;
    std::vector<r6::ProfileSet> sets;QSet<QString> ids,names;
    const auto number=[](QJsonValue value,double lo,double hi){return value.isDouble()&&std::isfinite(value.toDouble())&&value.toDouble()>=lo&&value.toDouble()<=hi;};
    const auto parse=[&](QJsonValue value,r6::Operator& op){
        if(!value.isObject())return false;const auto p=value.toObject();const double delay=p["delay"].toDouble(-1);
        if(!number(p["vertical"],0,100)||!number(p["horizontal"],-50,50)||!number(p["delay"],0,60000)||delay!=std::floor(delay)||!p["rapid"].isBool()||!p["humanize"].isBool())return false;
        op.vertical=float(p["vertical"].toDouble());op.horizontal=float(p["horizontal"].toDouble());op.delay=int(delay);op.rapid=p["rapid"].toBool();op.humanize=p["humanize"].toBool();return true;
    };
    for(const auto& value:root["profileSets"].toArray()){
        if(!value.isObject())return false;const auto item=value.toObject();const auto id=item["id"].toString(),name=item["name"].toString();
        if(id.isEmpty()||id.size()>64||name.trimmed().isEmpty()||name.size()>80||ids.contains(id)||names.contains(name.toCaseFolded())||!item["operators"].isArray())return false;
        ids.insert(id);names.insert(name.toCaseFolded());auto set=capture(config,id.toStdString(),name.toStdString());set.sensitivity={};
        if(!item["sensitivity"].isNull()&&!item["sensitivity"].isUndefined()){
            const auto setup=sensitivity::fromJson(item["sensitivity"].toObject());if(!setup)return false;set.sensitivity=*setup;set.sensitivity.source.clear();
        }
        const auto records=item["operators"].toArray();if(records.size()<r6::legacy_operator_count||records.size()>r6::operator_count)return false;QSet<int> seen;
        for(const auto& record:records){
            if(!record.isObject())return false;const auto entry=record.toObject();int index=-1;
            for(int i=0;i<r6::operator_count;++i)if(entry["name"].toString()==QString::fromUtf8(config.operators[i].name)){index=i;break;}
            if(index<0||seen.contains(index)||!entry["secondaryEnabled"].isBool()||!entry["secondaryInitialized"].isBool()||!parse(entry["primary"],set.primary[index])||!parse(entry["secondary"],set.secondary[index]))return false;
            seen.insert(index);set.secondary_enabled[index]=entry["secondaryEnabled"].toBool();set.secondary_initialized[index]=entry["secondaryInitialized"].toBool();
            if(set.secondary_enabled[index]&&!set.secondary_initialized[index])return false;
        }
        sets.push_back(std::move(set));
    }
    const auto active=root["activeProfileSet"].toString();if(!active.isEmpty()&&!ids.contains(active))return false;
    config.profile_sets=std::move(sets);config.active_profile_set=active.toStdString();
    // The active binary/sidecar values include edits made since the set was captured.
    remember(config);return true;
}
}
