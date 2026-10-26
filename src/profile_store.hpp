#pragma once
#include "core.hpp"
#include <QtCore>
#include <cmath>
#include <algorithm>
#include "diagnostics.hpp"
#include "sensitivity.hpp"
#include "profile_sets.hpp"

inline QString weaponFile(const std::filesystem::path& base){return QString::fromStdWString((base.parent_path()/L"azec_weapons.json").wstring());}
// Rapid fire is stored per weapon profile. Older sidecars carried a single
// global switch; those load by copying it onto every primary and secondary
// profile so the effective behavior does not change on upgrade. An absent or
// rejected sidecar never clears the binary's primary rapid flags: those flags
// are saved settings, and the caller quarantines the rejected file instead.
inline bool readWeaponProfiles(r6::Config& config,bool& gameFocusOnly,bool& adsOnly,const std::filesystem::path& base){
    QFile file(weaponFile(base));if(!file.exists())return true;
    if(!file.open(QIODevice::ReadOnly)){diagnostic("load weapon profiles",file.errorString());return false;}
    if(file.size()>1024*1024){diagnostic("load weapon profiles","File exceeds 1 MiB; ignored");return false;}
    const auto bytes=file.readAll();
    if(file.error()!=QFile::NoError){diagnostic("load weapon profiles",file.errorString());return false;}
    QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject()){diagnostic("load weapon profiles","Invalid JSON; ignored");return false;}
    const auto root=document.object();if(root["version"].toInt()!=1){diagnostic("load weapon profiles","Unsupported version; ignored");return false;}
    bool valid=true;
    if(const auto setup=sensitivity::fromJson(root["sensitivity"].toObject()))config.sensitivity=*setup;
    const auto validKey=[](const QJsonValue& value,int fallback){const int key=value.toInt(fallback);return r6::validHotkey(key)?key:fallback;};
    config.primary_key=validKey(root["primaryKey"],'1');config.secondary_key=validKey(root["secondaryKey"],'2');config.phone_enabled=root["phoneEnabled"].toBool(true);
    if(root.contains("gameFocusOnly")&&root["gameFocusOnly"].isBool())gameFocusOnly=root["gameFocusOnly"].toBool();
    if(root.contains("adsOnly")&&root["adsOnly"].isBool())adsOnly=root["adsOnly"].toBool();
    const auto profiles=root["operators"].toObject();
    for(int id=0;id<r6::operator_count;++id){
        const auto item=profiles[QString::fromUtf8(config.operators[id].name)].toObject();if(item.isEmpty())continue;
        const auto vertical=item["vertical"].toDouble(-1),horizontal=item["horizontal"].toDouble(-1000);const int delay=item["delay"].toInt(-1);
        if(!std::isfinite(vertical)||!std::isfinite(horizontal)||vertical<0||vertical>100||horizontal<-50||horizontal>50||delay<0||delay>60000){diagnostic("load weapon profile",QString::fromUtf8(config.operators[id].name)+": invalid values; ignored");valid=false;continue;}
        auto& op=config.secondary[id];op=config.operators[id];op.vertical=float(vertical);op.horizontal=float(horizontal);op.delay=delay;op.rapid=item["rapid"].toBool();op.humanize=item["humanize"].toBool(true);
        // Older saved secondary profiles may omit the marker. Retain
        // their edits; absent or invalid records above still initialize on enable.
        config.secondary_enabled[id]=item["enabled"].toBool();config.secondary_initialized[id]=item["initialized"].toBool(true);
    }
    const int selected=root["selected"].toInt(-1);if(selected>=0&&selected<r6::operator_count)config.selected=selected;
    const auto primaries=root["primaryProfiles"].toObject();
    for(int id=r6::legacy_operator_count;id<r6::operator_count;++id){
        const auto name=QString::fromUtf8(config.operators[id].name);const auto item=primaries[name].toObject();if(item.isEmpty())continue;
        const auto vertical=item["vertical"].toDouble(-1),horizontal=item["horizontal"].toDouble(-1000);const int delay=item["delay"].toInt(-1);
        if(!std::isfinite(vertical)||!std::isfinite(horizontal)||vertical<0||vertical>100||horizontal<-50||horizontal>50||delay<0||delay>60000){diagnostic("load extension profile",name+": invalid values; ignored");valid=false;continue;}
        auto& op=config.operators[id];op.vertical=float(vertical);op.horizontal=float(horizontal);op.delay=delay;op.rapid=item["rapid"].toBool();op.humanize=item["humanize"].toBool(true);
    }
    // Selection and shortcut writes can create a minimal sidecar. Only an
    // explicit legacy switch may replace the saved per-profile flags.
    if(!root["perProfileRapid"].toBool(false)&&root["rapidActive"].isBool()){
        const bool legacyRapid=root["rapidActive"].toBool();
        for(auto& op:config.operators)op.rapid=legacyRapid?1:0;
        for(auto& op:config.secondary)op.rapid=legacyRapid?1:0;
    }
    if(!profilesets::read(root,config,true)){diagnostic("load profile sets","Invalid profile library; active weapon values retained");valid=false;}
    return valid;
}
inline QByteArray weaponProfilesBytes(const r6::Config& config,bool gameFocusOnly,bool adsOnly){
    QJsonObject profiles,primaries;
    for(int id=0;id<r6::operator_count;++id){const auto& op=config.secondary[id];profiles[QString::fromUtf8(config.operators[id].name)]=QJsonObject{{"enabled",config.secondary_enabled[id]},{"initialized",config.secondary_initialized[id]},{"vertical",op.vertical},{"horizontal",op.horizontal},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)},{"delay",op.delay}};}
    // Extension operators keep their primary profile and the live selection here.
    for(int id=r6::legacy_operator_count;id<r6::operator_count;++id){const auto& op=config.operators[id];primaries[QString::fromUtf8(config.operators[id].name)]=QJsonObject{{"vertical",op.vertical},{"horizontal",op.horizontal},{"rapid",bool(op.rapid)},{"humanize",bool(op.humanize)},{"delay",op.delay}};}
    return QJsonDocument(QJsonObject{{"version",1},{"primaryKey",config.primary_key},{"secondaryKey",config.secondary_key},{"phoneEnabled",config.phone_enabled},
        {"perProfileRapid",true},{"gameFocusOnly",gameFocusOnly},{"adsOnly",adsOnly},{"recoilMaster",true},{"rapidMaster",bool(config.rapid_master)},
        {"selected",config.selected},{"operators",profiles},{"primaryProfiles",primaries},
        {"profileSets",profilesets::json(config)},{"activeProfileSet",QString::fromStdString(config.active_profile_set)},
        {"sensitivity",config.sensitivity.valid()?QJsonValue(sensitivity::json(config.sensitivity)):QJsonValue(QJsonValue::Null)}}).toJson(QJsonDocument::Compact);
}
// Two-phase write: the sidecar is prepared without touching the disk copy and
// only committed after the primary binary write succeeded.
inline bool stageWeaponProfiles(const r6::Config& config,bool gameFocusOnly,bool adsOnly,const std::filesystem::path& base,QSaveFile& file){
    const auto bytes=weaponProfilesBytes(config,gameFocusOnly,adsOnly);
    if(bytes.size()>1024*1024)return false;
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
// Shortcut changes persist immediately, but they are not profile edits. Replace
// only the binding fields in the saved files so every pending profile value
// stays unsaved, and stage the sidecar before the binary as a normal save does.
inline bool persistHotkeys(const std::filesystem::path& base,int menu,int recoil,int panic,int primary,int secondary){
    const QString binaryPath=QString::fromStdWString(base.wstring());
    r6::Config saved;
    if(!saved.load(base)){
        if(QFile::exists(binaryPath)){diagnostic("save shortcuts","The saved configuration could not be read");return false;}
        saved=r6::Config();
    }
    saved.menu_key=menu;saved.recoil_key=recoil;saved.panic_key=panic;
    const QString sidecarPath=weaponFile(base);QJsonObject root;
    {
        QFile existing(sidecarPath);
        if(existing.exists()){
            if(!existing.open(QIODevice::ReadOnly)||existing.size()>1024*1024)return false;
            QJsonParseError error;const auto document=QJsonDocument::fromJson(existing.readAll(),&error);
            if(error.error!=QJsonParseError::NoError||!document.isObject()||document.object()["version"].toInt()!=1)return false;
            root=document.object();
        }
    }
    root["version"]=1;root["primaryKey"]=primary;root["secondaryKey"]=secondary;
    const auto bytes=QJsonDocument(root).toJson();
    if(bytes.size()>1024*1024)return false;
    QSaveFile sidecar(sidecarPath);
    if(!sidecar.open(QIODevice::WriteOnly)||sidecar.write(bytes)!=bytes.size()){diagnostic("save shortcuts",sidecar.errorString());return false;}
    QByteArray previous;const bool existed=QFile::exists(binaryPath);
    if(existed){
        QFile file(binaryPath);
        if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)return false;
        previous=file.readAll();if(file.error()!=QFile::NoError)return false;
    }
    // Windows cannot replace a file that is still open, so both handles above
    // are closed before the binary is rewritten.
    if(!saved.save(base)){diagnostic("save shortcuts","Atomic replacement of the binary failed");return false;}
    if(!sidecar.commit()){
        QSaveFile restore(binaryPath);
        const bool rolledBack=existed?restore.open(QIODevice::WriteOnly)&&restore.write(previous)==previous.size()&&restore.commit():QFile::remove(binaryPath);
        diagnostic("save shortcuts",sidecar.errorString()+(rolledBack?"; previous binary restored":"; rollback failed"));
        return false;
    }
    return true;
}
