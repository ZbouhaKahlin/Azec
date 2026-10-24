#include <QCoreApplication>
#include "core_checks.hpp"
#include "../src/sensitivity.hpp"
#include "../src/profile_store.hpp"
#include "../src/profile_transfer.hpp"
#include "../src/ui_preferences.hpp"
#include "../src/engine.hpp"
#include <iostream>

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    // The caller supplies a fresh fixture; no user settings or game are opened.
    if(argc!=2)return 2;
    const QString directory=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    if(!QDir(directory).isEmpty()||!QDir::setCurrent(directory))return 2;
    const auto path=std::filesystem::path(directory.toStdWString())/L"r6_config.bin";
    bool ok=true;int checks=0;
    const auto check=[&](bool passed,const char* name){++checks;ok&=passed;std::cout<<(passed?"PASS ":"FAIL ")<<name<<'\n';};
    check(r6::self_test(path),"existing core compatibility, validation, and movement suite");
    const auto write=[](const QString& path,const QByteArray& bytes){QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size();};
    const QByteArray ini="[DISPLAY]\nMouseSensitivityVertical=99\n[INPUT]\nMouseSensitivityHorizontal=25\nMouseSensitivityVertical=50\nMouseSensitivityMultiplierUnit=0.002230\n";
    check(write("GameSettings.ini",ini),"create local INI fixture");
    const auto found=sensitivity::readFile("GameSettings.ini");
    check(found&&found->horizontal==25&&found->vertical==50&&qAbs(found->multiplier-.00223)<1e-12&&!found->source.empty(),"read INPUT values and custom multiplier without confusing other sections");
    QFile untouched("GameSettings.ini");untouched.open(QIODevice::ReadOnly);check(untouched.readAll()==ini,"game settings remain byte-for-byte unchanged");untouched.close();
    check(write("invalid.ini","[INPUT]\nMouseSensitivityHorizontal=0\nMouseSensitivityVertical=50\nMouseSensitivityMultiplierUnit=0.02\n")&&!sensitivity::readFile("invalid.ini"),"reject zero sensitivity");
    check(write("invalid.ini","[INPUT]\nMouseSensitivityHorizontal=25\nMouseSensitivityVertical=50\n")&&!sensitivity::readFile("invalid.ini"),"do not guess missing multiplier");
    check(write("invalid.ini","[INPUT]\nMouseSensitivityHorizontal=nan\nMouseSensitivityVertical=50\nMouseSensitivityMultiplierUnit=0.02\n")&&!sensitivity::readFile("invalid.ini"),"reject non-finite sensitivity");
    check(!sensitivity::readFile("missing.ini"),"missing INI leaves manual setup available");
    r6::SensitivitySetup baseline;baseline.dpi=800;baseline.horizontal=10;baseline.vertical=20;baseline.multiplier=.02;baseline.source=L"private-machine/GameSettings.ini";
    check(baseline.valid()&&sensitivity::fromJson(sensitivity::json(baseline)).has_value(),"setup JSON round trip");
    auto invalid=sensitivity::json(baseline);invalid["dpi"]=800.5;check(!sensitivity::fromJson(invalid),"reject fractional DPI");
    invalid=sensitivity::json(baseline);invalid["vertical"]=101;check(!sensitivity::fromJson(invalid),"reject out-of-range setup values");
    r6::Config config;config.sensitivity=baseline;
    config.enable_secondary(0,true);config.secondary[0].vertical=12;config.secondary[0].horizontal=3;config.enable_secondary(0,false);
    config.enable_secondary(r6::operator_count-1,true);config.secondary.back().vertical=5;
    config.operators.back().vertical=7;config.operators[0].horizontal=-4.5f;config.operators[0].rapid=1;config.selected=4;config.weapon_slot=0;
    auto dpiOnly=baseline;dpiOnly.dpi=1600;const auto dpiFactors=sensitivity::scale(config,dpiOnly);
    auto unchanged=config;sensitivity::apply(unchanged,dpiOnly);
    check(dpiFactors.fits&&dpiFactors.horizontal==1&&dpiFactors.vertical==1&&unchanged.bytes()==config.bytes()&&unchanged.sensitivity.dpi==1600,"DPI-only update does not rescale synthetic movement");
    auto next=baseline;next.dpi=1600;next.horizontal=40;next.vertical=20;next.multiplier=.01;
    const auto factors=sensitivity::scale(config,next);
    check(factors.fits&&factors.horizontal==.5&&factors.vertical==2,"separate axis factors include the game multiplier");
    const auto old=config;sensitivity::apply(config,next);
    check(config.operators[0].vertical==20&&config.operators[0].horizontal==-2.3f&&config.secondary[0].vertical==24&&config.secondary[0].horizontal==1.5f,"rescale primary and retained disabled secondary, round signed values to 0.1");
    check(config.operators.back().vertical==14&&config.secondary.back().vertical==10,"rescale extension operators and initialized secondary profiles");
    check(config.selected==old.selected&&config.weapon_slot==old.weapon_slot&&config.operators[0].rapid==old.operators[0].rapid&&config.operators[0].delay==old.operators[0].delay&&config.operators[0].humanize==old.operators[0].humanize,"preserve selection, rapid fire, timing, and humanization");
    auto overflow=config;overflow.operators.back().vertical=100;const auto overflowBefore=overflow.bytes();
    auto excessive=next;excessive.vertical=1;sensitivity::apply(overflow,excessive);
    check(!sensitivity::scale(overflow,excessive).fits&&overflow.bytes()==overflowBefore&&sensitivity::sameGame(overflow.sensitivity,next)&&overflow.operators.back().vertical==100,"out-of-range rescale is rejected without partial updates");
    bool focus=true,ads=false;r6::Config readBack;check(write(weaponFile(path),weaponProfilesBytes(config,focus,ads)),"save sidecar fixture");
    readWeaponProfiles(readBack,focus,ads,path);
    check(readBack.sensitivity.dpi==1600&&sensitivity::sameGame(readBack.sensitivity,next)&&readBack.secondary[0].vertical==24&&readBack.operators.back().vertical==14,"sidecar reload preserves calibration and both profile stores");
    const auto backup=makeBackup(config,QSet<QString>{"Ace"},true,"charcoal",false);r6::Config imported;QSet<QString> favorites;BackupOptions options;
    const auto exportedSetup=backup["settings"].toObject()["sensitivity"].toObject();
    check(!exportedSetup.contains("source")&&parseBackup(backup,imported,favorites,options)&&imported.sensitivity.source.empty()&&sensitivity::sameGame(imported.sensitivity,next)&&favorites.contains("Ace"),"backup preserves numeric reference and excludes machine paths");
    auto legacy=backup;auto legacySettings=legacy["settings"].toObject();legacySettings.remove("sensitivity");legacy["settings"]=legacySettings;
    check(parseBackup(legacy,imported,favorites,options)&&!imported.sensitivity.valid(),"older backups remain accepted without an invented reference");
    auto malformed=backup;auto settings=malformed["settings"].toObject();auto badSetup=settings["sensitivity"].toObject();badSetup["dpi"]=-1;settings["sensitivity"]=badSetup;malformed["settings"]=settings;
    check(!parseBackup(malformed,imported,favorites,options),"reject malformed reference in backups");
    r6::Engine engine(true);engine.edit([&](r6::Config& target){target=config;});check(engine.save(path),"save profiles and reference through Engine transaction");
    r6::Engine restored(true);check(restored.load(path)&&restored.snapshot().sensitivity.dpi==1600&&restored.snapshot().operators[0].horizontal==-2.3f&&restored.snapshot().secondary[0].vertical==24,"Engine restart retains scaled profiles and reference together");
    check(QFileInfo(QString::fromStdWString(path.wstring())).size()==3734,"reference never extends binary configuration");
    check(uiprefs::update(path,[](QJsonObject& root){root["favorites"]=QJsonArray{"Ace"};root["colorTheme"]="rose";root["sidebarCollapsed"]=true;}),"create isolated UI preferences");
    check(uiprefs::update(path,[](QJsonObject& root){root["colorTheme"]="charcoal";}),"update isolated appearance preferences");
    QJsonObject prefs;check(uiprefs::readDocument(uiprefs::filePath(path),prefs)&&prefs["favorites"].toArray().contains("Ace")&&prefs["colorTheme"]=="charcoal"&&prefs["sidebarCollapsed"].toBool(),"appearance changes preserve favorites and sidebar preference");
    std::cout<<checks<<" console checks: "<<(ok?"PASS":"FAIL")<<'\n';return ok?0:1;
}
